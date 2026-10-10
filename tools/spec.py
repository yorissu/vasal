
#
# spec.py
#
# Checks the specification (docs/spec.txt) and the project's files against
# it: the specification's own format, every reference to it in code and
# tests, that every agreed criterion due by the current milestone has a
# test, and the file rules of every file given.
#
# The check target runs it:
#	python3 tools/spec.py --spec docs/spec.txt --milestone 0 <files...>
# It prints one line per problem and exits with 1 when there is any.
#
# References are comments holding "spec:" and ids, after // in C and # in
# Python. Files in a tests folder are tests: their references mark criteria
# as tested, and every test (cairo_test_new in C, a test_ method in Python)
# must be preceded by one.
#

import argparse
import dataclasses
import pathlib
import re
import sys

ID = re.compile(r"[a-z][a-z0-9_]*(\.[a-z][a-z0-9_]*)+")
NAME = re.compile(r"[a-z][a-z0-9_]*")
TESTS = ("unit", "scenario", "statistics", "fuzz", "lint", "review")
STATUSES = ("proposed", "open")
UNITS = ("percent", "ticks", "distance", "gold", "troops", "count")
OPERATORS = ("=", "!=", "<", "<=", ">", ">=", "in", "~")
CRITERION_FIELDS = ("given", "when", "then", "expect", "why")
STORY_FIELDS = ("title", "milestone", "links", "description", "why")
LATER = sys.maxsize

# The marker is split, so this file holds no reference itself.
REFERENCE = re.compile(r"(?://|#) " + "spec" + r": ?(.*)")
C_TEST = re.compile(r"\bcairo_test_new\(")
PYTHON_TEST = re.compile(r"^\s*def test_\w*\(")


@dataclasses.dataclass
class Problem:
	file: str
	line: int
	message: str
	detail: str = ""

	def __str__(self):
		text = f"{self.file}:{self.line}: {self.message}"
		return f"{text}: {self.detail}" if self.detail else text


@dataclasses.dataclass
class Criterion:
	story: str
	number: int
	line: int
	test: str
	status: str
	has_then: bool = False
	tested: bool = False


@dataclasses.dataclass
class Story:
	id: str
	line: int
	milestone: int = LATER
	fields: set = dataclasses.field(default_factory=set)
	criteria: list = dataclasses.field(default_factory=list)
	values: set = dataclasses.field(default_factory=set)


@dataclasses.dataclass
class Document:
	file: str
	stories: dict = dataclasses.field(default_factory=dict)

	def criterion(self, story, number):
		for criterion in self.stories[story].criteria:
			if criterion.number == number:
				return criterion
		return None


#
# The file rules: an empty first line, exactly one newline at the end, LF
# only, printable ASCII and tabs only, no trailing spaces or tabs, and at most
# 80 columns with tabs 4 columns wide. An empty file is fine.
#
def check_format(file, text):
	problems = []
	if not text:
		return problems
	if not text.startswith("\n"):
		problems.append(Problem(file, 1, "the first line is not empty"))
	if not text.endswith("\n"):
		problems.append(Problem(file, 0, "no newline at the end"))
	elif text.endswith("\n\n"):
		problems.append(Problem(file, 0, "more than one newline at the end"))
	for number, line in enumerate(text.split("\n"), 1):
		if any(not (" " <= character <= "~" or character == "\t")
				for character in line):
			problems.append(Problem(file, number,
				"a character that is not printable ASCII (or CR)"))
		if len(line.expandtabs(4)) > 80:
			problems.append(Problem(file, number, "longer than 80 columns"))
		if line.endswith((" ", "\t")):
			problems.append(Problem(file, number, "trailing spaces or tabs"))
	return problems


#
# Reads the stories of the specification. A line's depth is its number of
# leading tabs: 0 starts a story, 1 is a story field, a criterion or a value,
# 2 is a criterion field or continues a story field, 3 continues a criterion
# field. Links and value references may point forward, so they are checked
# once the whole file is read.
#
def read_document(file, text):
	document = Document(file)
	problems = []
	references = []
	story = None
	criterion = None
	field = None

	def problem(number, message, detail=""):
		problems.append(Problem(file, number, message, detail))

	for number, line in enumerate(text.split("\n"), 1):
		if not line:
			story, criterion, field = None, None, None
			continue
		if line.startswith("#"):
			continue
		for key in re.findall(r"\{([^}]*)\}", line):
			if ID.fullmatch(key):
				references.append((number, key, True))
			else:
				problem(number, "malformed value reference", key)

		body = line.lstrip("\t")
		depth = len(line) - len(body)
		if depth == 0:
			criterion, field = None, None
			match = re.fullmatch(r"story (\S+)", line)
			if not match or not ID.fullmatch(match.group(1)):
				story = None
				problem(number, "malformed story line", line)
			elif match.group(1) in document.stories:
				story = None
				problem(number, "duplicate story", match.group(1))
			else:
				story = Story(match.group(1), number)
				document.stories[story.id] = story
		elif story is None:
			problem(number, "line outside a story", body)
		elif depth == 1:
			criterion, field = None, None
			if body.startswith("criterion "):
				criterion = read_criterion(story, number, body, problem)
			elif body.startswith("value "):
				read_value(story, number, body, problem)
			else:
				field = read_field(story, number, body, references, problem)
		elif depth == 2 and criterion is not None:
			name, _, value = body.partition(": ")
			if name not in CRITERION_FIELDS or not value:
				problem(number, "unknown criterion field", body)
			if name == "then":
				criterion.has_then = True
			if name == "expect" and not well_formed_expect(value):
				problem(number, "malformed expect", body)
			field = name
		elif depth == 2 and field is not None:
			if field == "links":
				references.extend((number, id, False) for id in body.split())
		elif depth == 3 and criterion is not None and field is not None:
			pass
		else:
			problem(number, "wrong indentation", body)

	for story in document.stories.values():
		if not {"title", "milestone", "description"} <= story.fields:
			problem(story.line,
				"story lacks a title, milestone or description", story.id)
		for criterion in story.criteria:
			if not criterion.has_then:
				problem(criterion.line, "criterion lacks then",
					f"{story.id}#{criterion.number}")
	for number, key, value in references:
		if not value:
			if key not in document.stories:
				problem(number, "link to no story", key)
			continue
		story_id, _, name = key.rpartition(".")
		if story_id not in document.stories \
				or name not in document.stories[story_id].values:
			problem(number, "reference to no value", key)
	return document, problems


# "criterion <n>: <test> [proposed | open]"
def read_criterion(story, number, body, problem):
	match = re.fullmatch(r"criterion (\d+): (\w+)(?: (\w+))?", body)
	if not match or match.group(2) not in TESTS \
			or (match.group(3) and match.group(3) not in STATUSES):
		problem(number, "malformed criterion line", body)
		return None
	criterion = Criterion(story.id, int(match.group(1)), number,
		match.group(2), match.group(3) or "agreed")
	if criterion.number != len(story.criteria) + 1:
		problem(number, "criterion out of order", body)
	story.criteria.append(criterion)
	return criterion


# "value <name>: <number or open> <unit> [<min>..<max>] [fixed]"
def read_value(story, number, body, problem):
	match = re.fullmatch(
		r"value ([a-z][a-z0-9_]*): (open|\d+) (\w+)"
		r"(?: (\d+)\.\.(\d+))?( fixed)?", body)
	if not match or match.group(3) not in UNITS \
			or (match.group(4) and int(match.group(4)) > int(match.group(5))):
		problem(number, "malformed value line", body)
	elif match.group(1) in story.values:
		problem(number, "duplicate value", match.group(1))
	else:
		story.values.add(match.group(1))


# A story field: title, milestone, links, description or why.
def read_field(story, number, body, references, problem):
	name, separator, value = body.partition(": ")
	if not separator or name not in STORY_FIELDS:
		problem(number, "unknown field", body)
		return None
	story.fields.add(name)
	if name == "milestone":
		if value == "later":
			story.milestone = LATER
		elif re.fullmatch(r"[0-7]", value):
			story.milestone = int(value)
		else:
			problem(number, "malformed milestone", value)
	if name == "links":
		references.extend((number, id, False) for id in value.split())
	return name


# "expect: <quantity> <operator> <value>"
def well_formed_expect(value):
	words = value.split()
	return len(words) == 3 and re.fullmatch(r"[a-z0-9_.]+", words[0]) \
		and words[1] in OPERATORS


#
# Checks the references in a source file and, in tests, marks the criteria
# they name as tested and makes sure every test is preceded by one.
#
def check_references(document, file, text, test):
	problems = []
	referenced = False
	test_start = PYTHON_TEST if file.endswith(".py") else C_TEST
	for number, line in enumerate(text.split("\n"), 1):
		match = REFERENCE.search(line)
		if match:
			ids = match.group(1).split()
			if not ids:
				problems.append(Problem(file, number, "reference without ids"))
			for id in ids:
				problems.extend(check_id(document, file, number, id, test))
			referenced = True
		elif test and test_start.search(line):
			if not referenced:
				problems.append(Problem(file, number, "test names no criteria",
					line.strip()))
			referenced = False
		elif line.strip():
			referenced = False
	return problems


def check_id(document, file, number, id, test):
	story_id, hash, criterion_number = id.partition("#")
	if story_id not in document.stories:
		return [Problem(file, number, "reference to no story", id)]
	if not hash:
		return []
	criterion = None
	if criterion_number.isdigit():
		criterion = document.criterion(story_id, int(criterion_number))
	if criterion is None:
		return [Problem(file, number, "reference to no criterion", id)]
	if test:
		criterion.tested = True
	return []


#
# Every agreed criterion due by the milestone that needs a test (unit,
# scenario, statistics or fuzz) must have one. A fuzz criterion is due no
# earlier than the story that brings the fuzz build (build.fuzz).
#
def check_coverage(document, milestone):
	problems = []
	fuzz = document.stories.get("build.fuzz")
	fuzz_milestone = fuzz.milestone if fuzz else 0
	for story in document.stories.values():
		for criterion in story.criteria:
			due = story.milestone
			if criterion.test == "fuzz":
				due = max(due, fuzz_milestone)
			if due > milestone:
				continue
			if criterion.status == "agreed" \
					and criterion.test not in ("lint", "review") \
					and not criterion.tested:
				problems.append(Problem(document.file, criterion.line,
					"agreed criterion has no test",
					f"{story.id}#{criterion.number}"))
	return problems


def read(file, problems):
	try:
		with open(file, encoding="ascii", errors="replace",
				newline="") as stream:
			return stream.read()
	except OSError as error:
		problems.append(Problem(file, 0, "can't read the file",
			error.strerror or ""))
		return None


def main(arguments):
	parser = argparse.ArgumentParser(description="Checks the specification.")
	parser.add_argument("--spec", required=True)
	parser.add_argument("--milestone", type=int, required=True)
	parser.add_argument("files", nargs="*")
	options = parser.parse_args(arguments)

	problems = []
	text = read(options.spec, problems)
	if text is not None:
		problems.extend(check_format(options.spec, text))
		document, found = read_document(options.spec, text)
		problems.extend(found)
		for file in options.files:
			text = read(file, problems)
			if text is None:
				continue
			problems.extend(check_format(file, text))
			if file.endswith((".c", ".h", ".py")):
				test = "tests" in pathlib.PurePath(file).parts
				problems.extend(check_references(document, file, text, test))
		problems.extend(check_coverage(document, options.milestone))

	for problem in problems:
		print(problem)
	return 1 if problems else 0


if __name__ == "__main__":
	sys.exit(main(sys.argv[1:]))
