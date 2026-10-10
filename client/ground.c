
/// The terrain's colors go into a texture of one texel per tile. Each pixel
/// looks at the 16 tiles around the point it shows, weighted by a cubic
/// B-spline, which smooths the steps of tiles painted in a staircase into
/// curves. Each terrain type there gets its share of the weight, and the
/// shares, raised to a high power, decide the color, so the type holding
/// most of the weight wins and a border between two types is a smooth
/// curve with a narrow soft band, not the grid's steps. Noise moves the
/// point read, so borders waver, and a broader noise varies the ground's
/// brightness, like paint. The shader finds the
/// point of the map under each pixel by turning the camera's projection
/// back (client/camera.h), and how far inside sight it lies from the
/// player's troops and how far each sees, handed to it each frame, as
/// client/game.c measures it: fully inside sight to 1 tile before a
/// troop's radius, fading to none at it.

#include "client/ground.h"

#include <raylib.h>
#include <rlgl.h>

// The most troops whose sight the shader takes: a centaine.
#define _vasal_client_ground_eyes_max                                         \
	(vasal_engine_world_groups * vasal_engine_world_troops)

// The ground beyond the map's edge.
#define _vasal_client_ground_beyond 0x202020ffU

// GLSL ES 1.00, for WebGL. The noise is a hash of the cell corners,
// blended smoothly.
static const char* const _vasal_client_ground_shader =
	"#version 100\n"
	"precision highp float;\n"
	"uniform sampler2D texture0;\n"
	"uniform vec2 map_size;\n"
	"uniform vec2 window;\n"
	"uniform vec2 render;\n"
	"uniform vec3 camera;\n"
	"uniform float tilt;\n"
	"uniform int eye_count;\n"
	"uniform vec3 eyes[120];\n"
	"uniform vec3 beyond;\n"
	"float hash(vec2 p) {\n"
	"	return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);\n"
	"}\n"
	"float noise(vec2 p) {\n"
	"	vec2 i = floor(p);\n"
	"	vec2 f = fract(p);\n"
	"	vec2 u = f * f * (3.0 - 2.0 * f);\n"
	"	return mix(mix(hash(i), hash(i + vec2(1.0, 0.0)), u.x),\n"
	"		mix(hash(i + vec2(0.0, 1.0)), hash(i + vec2(1.0, 1.0)), u.x),\n"
	"		u.y);\n"
	"}\n"
	"vec3 tile(vec2 p) {\n"
	"	vec2 cell = floor(clamp(p, vec2(0.0), map_size - 1.0)) + 0.5;\n"
	"	return texture2D(texture0, cell / map_size).rgb;\n"
	"}\n"
	"vec4 spline(float f) {\n"
	"	float g = 1.0 - f;\n"
	"	return vec4(g * g * g, 3.0 * f * f * f - 6.0 * f * f + 4.0,\n"
	"		-3.0 * f * f * f + 3.0 * f * f + 3.0 * f + 1.0, f * f * f)\n"
	"		/ 6.0;\n"
	"}\n"
	"void main() {\n"
	"	vec2 pixel = vec2(gl_FragCoord.x, render.y - gl_FragCoord.y)\n"
	"		* (window / render);\n"
	"	float across = (pixel.x - 0.5 * window.x) / camera.z;\n"
	"	float down = (pixel.y - 0.5 * window.y) / (camera.z * tilt);\n"
	"	vec2 at = vec2(across + down, down - across) * 0.70710678\n"
	"		+ camera.xy;\n"
	"	if (at.x < 0.0 || at.y < 0.0 || at.x >= map_size.x\n"
	"			|| at.y >= map_size.y) {\n"
	"		gl_FragColor = vec4(beyond, 1.0);\n"
	"		return;\n"
	"	}\n"
	"	vec2 wobble = vec2(noise(at * 0.4), noise(at * 0.4 + 17.0)) * 0.7\n"
	"		+ vec2(noise(at * 1.3 + 5.0), noise(at * 1.3 + 31.0)) * 0.3\n"
	"		- 0.5;\n"
	"	vec2 look = at + wobble * 0.8;\n"
	"	vec2 corner = floor(look - 0.5);\n"
	"	vec4 wx = spline(look.x - 0.5 - corner.x);\n"
	"	vec4 wy = spline(look.y - 0.5 - corner.y);\n"
	"	vec3 colors[16];\n"
	"	float weights[16];\n"
	"	for (int j = 0; j < 4; j++) {\n"
	"		for (int i = 0; i < 4; i++) {\n"
	"			colors[j * 4 + i] = tile(corner\n"
	"				+ vec2(float(i) - 0.5, float(j) - 0.5));\n"
	"			weights[j * 4 + i] = wx[i] * wy[j];\n"
	"		}\n"
	"	}\n"
	"	vec3 color = vec3(0.0);\n"
	"	float total = 0.0;\n"
	"	for (int a = 0; a < 16; a++) {\n"
	"		float share = 0.0;\n"
	"		for (int b = 0; b < 16; b++) {\n"
	"			vec3 d = colors[a] - colors[b];\n"
	"			share += dot(d, d) < 0.0001 ? weights[b] : 0.0;\n"
	"		}\n"
	"		float v = weights[a] * pow(share, 10.0);\n"
	"		color += colors[a] * v;\n"
	"		total += v;\n"
	"	}\n"
	"	color /= total;\n"
	"	color *= 0.93 + 0.14 * noise(at * 0.09);\n"
	"	float seen = 0.0;\n"
	"	for (int i = 0; i < 120; i++) {\n"
	"		if (i >= eye_count) {\n"
	"			break;\n"
	"		}\n"
	"		seen = max(seen, eyes[i].z - distance(at, eyes[i].xy));\n"
	"	}\n"
	"	seen = clamp(seen, 0.0, 1.0);\n"
	"	float gray = dot(color, vec3(0.30, 0.59, 0.11)) * 0.6;\n"
	"	gl_FragColor = vec4(mix(vec3(gray), color, seen), 1.0);\n"
	"}\n";

// The shader and the map's terrain texture, made once per map.
typedef struct _vasal_client_ground_s {
	Shader                    shader;
	Texture2D                 terrain;
	const vasal_engine_map_s* map;
	bool_t                    tried;
	bool_t                    working;
	int                       map_size;
	int                       window;
	int                       render;
	int                       camera;
	int                       tilt;
	int                       eye_count;
	int                       eyes;
	int                       beyond;
} _vasal_client_ground_s;

static _vasal_client_ground_s _vasal_client_ground;

// Compiles the shader, once. A shader that fails to compile comes back as
// raylib's default one.
static void _vasal_client_ground_compile(
		_vasal_client_ground_s* const ground) {
	ground->tried = true;
	ground->shader = LoadShaderFromMemory(NULL, _vasal_client_ground_shader);
	ground->working = IsShaderValid(ground->shader)
		&& (ground->shader.id != rlGetShaderIdDefault());
	if (!ground->working) {
		return;
	}
	const Shader shader = ground->shader;
	ground->map_size = GetShaderLocation(shader, "map_size");
	ground->window = GetShaderLocation(shader, "window");
	ground->render = GetShaderLocation(shader, "render");
	ground->camera = GetShaderLocation(shader, "camera");
	ground->tilt = GetShaderLocation(shader, "tilt");
	ground->eye_count = GetShaderLocation(shader, "eye_count");
	ground->eyes = GetShaderLocation(shader, "eyes");
	ground->beyond = GetShaderLocation(shader, "beyond");
}

// Paints the map's terrain colors into a texture, one texel per tile.
// spec: client.render#6
static void _vasal_client_ground_paint(_vasal_client_ground_s* const ground,
		const vasal_engine_map_s* const map) {
	if (NULL != ground->map) {
		UnloadTexture(ground->terrain);
	}
	Image image = GenImageColor((int)map->width, (int)map->height, BLANK);
	for (uint16_t y = 0U; y < map->height; y++) {
		for (uint16_t x = 0U; x < map->width; x++) {
			ImageDrawPixel(&image, (int)x, (int)y,
				GetColor(vasal_engine_map_terrain(map, x, y)->color));
		}
	}
	ground->terrain = LoadTextureFromImage(image);
	UnloadImage(image);
	SetTextureFilter(ground->terrain, TEXTURE_FILTER_POINT);
	SetTextureWrap(ground->terrain, TEXTURE_WRAP_CLAMP);
	ground->map = map;
}

// Hands the shader the player's troops, where they are drawn now.
// spec: map.sight#1 map.sight#3
static void _vasal_client_ground_eyes(const _vasal_client_ground_s* const
		ground, const vasal_client_game_s* const game) {
	static float eyes[_vasal_client_ground_eyes_max * 3U];
	int count = 0;
	for (size_t g = 0U; g < vasal_engine_world_groups; g++) {
		const vasal_engine_world_group_s* const group =
			vasal_client_game_group(game, g);
		for (size_t t = 0U; t < group->troop_count; t++) {
			eyes[(count * 3) + 2] = vasal_client_game_eye(game, g, t,
				&eyes[count * 3], &eyes[(count * 3) + 1]);
			count++;
		}
	}
	SetShaderValue(ground->shader, ground->eye_count, &count,
		SHADER_UNIFORM_INT);
	if (0 < count) {
		SetShaderValueV(ground->shader, ground->eyes, eyes,
			SHADER_UNIFORM_VEC3, count);
	}
}

// spec: client.render#6 map.sight#3
bool_t vasal_client_ground_draw(const vasal_client_game_s* const game,
	const vasal_client_camera_s* const camera, const float width,
	const float height) {
	_vasal_client_ground_s* const ground = &_vasal_client_ground;
	if (!ground->tried) {
		_vasal_client_ground_compile(ground);
	}
	if (!ground->working) {
		return false;
	}
	const vasal_engine_map_s* const map = game->world->map;
	if (map != ground->map) {
		_vasal_client_ground_paint(ground, map);
	}
	const Shader shader = ground->shader;
	const float map_size[2] = { (float)map->width, (float)map->height };
	const float window[2] = { width, height };
	const float render[2] = {
		(float)GetRenderWidth(), (float)GetRenderHeight()
	};
	const float view[3] = { camera->x, camera->y, camera->scale };
	const float tilt = vasal_client_camera_tilt;
	const Color beyond = GetColor(_vasal_client_ground_beyond);
	const float beyond_color[3] = {
		(float)beyond.r / 255.0f, (float)beyond.g / 255.0f,
		(float)beyond.b / 255.0f
	};
	SetShaderValue(shader, ground->map_size, map_size, SHADER_UNIFORM_VEC2);
	SetShaderValue(shader, ground->window, window, SHADER_UNIFORM_VEC2);
	SetShaderValue(shader, ground->render, render, SHADER_UNIFORM_VEC2);
	SetShaderValue(shader, ground->camera, view, SHADER_UNIFORM_VEC3);
	SetShaderValue(shader, ground->tilt, &tilt, SHADER_UNIFORM_FLOAT);
	SetShaderValue(shader, ground->beyond, beyond_color,
		SHADER_UNIFORM_VEC3);
	_vasal_client_ground_eyes(ground, game);
	BeginShaderMode(shader);
	// The terrain texture goes to the shader as the texture drawn, over
	// the whole window.
	const Rectangle source = {
		0.0f, 0.0f, (float)map->width, (float)map->height
	};
	const Rectangle target = { 0.0f, 0.0f, width, height };
	const Vector2 origin = { 0.0f, 0.0f };
	DrawTexturePro(ground->terrain, source, target, origin, 0.0f, WHITE);
	EndShaderMode();
	return true;
}
