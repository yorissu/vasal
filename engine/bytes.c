
/// Bounds-checked little-endian reading and writing.

#include "engine/bytes.h"

// Whether count more bytes fit after offset in a buffer of size bytes.
static bool_t _vasal_engine_bytes_fits(const size_t offset,
		const size_t count, const size_t size) {
	return (offset <= size) && (count <= (size - offset));
}

vasal_engine_bytes_writer_s vasal_engine_bytes_writer(uint8_t* const data,
	const size_t capacity) {
	const vasal_engine_bytes_writer_s writer = {
		.data     = data,
		.capacity = capacity,
		.size     = 0U,
		.failed   = false
	};
	return writer;
}

void vasal_engine_bytes_write(vasal_engine_bytes_writer_s* const writer,
	const uint8_t* const bytes, const size_t count) {
	if (writer->failed
			|| !_vasal_engine_bytes_fits(writer->size, count,
				writer->capacity)) {
		writer->failed = true;
		return;
	}
	for (size_t i = 0U; i < count; i++) {
		writer->data[writer->size + i] = bytes[i];
	}
	writer->size += count;
}

void vasal_engine_bytes_write_u8(vasal_engine_bytes_writer_s* const writer,
	const uint8_t value) {
	vasal_engine_bytes_write(writer, &value, 1U);
}

void vasal_engine_bytes_write_u16(vasal_engine_bytes_writer_s* const writer,
	const uint16_t value) {
	const uint8_t bytes[2] = {
		(uint8_t)(value & 0xffU),
		(uint8_t)(value >> 8U)
	};
	vasal_engine_bytes_write(writer, bytes, sizeof(bytes));
}

void vasal_engine_bytes_write_u32(vasal_engine_bytes_writer_s* const writer,
	const uint32_t value) {
	const uint8_t bytes[4] = {
		(uint8_t)(value & 0xffU),
		(uint8_t)((value >> 8U) & 0xffU),
		(uint8_t)((value >> 16U) & 0xffU),
		(uint8_t)(value >> 24U)
	};
	vasal_engine_bytes_write(writer, bytes, sizeof(bytes));
}

vasal_engine_bytes_reader_s vasal_engine_bytes_reader(
	const uint8_t* const data, const size_t size) {
	const vasal_engine_bytes_reader_s reader = {
		.data   = data,
		.size   = size,
		.at     = 0U,
		.failed = false
	};
	return reader;
}

void vasal_engine_bytes_read(vasal_engine_bytes_reader_s* const reader,
	uint8_t* const bytes, const size_t count) {
	const bool_t fits = !reader->failed
		&& _vasal_engine_bytes_fits(reader->at, count, reader->size);
	for (size_t i = 0U; i < count; i++) {
		bytes[i] = fits ? reader->data[reader->at + i] : 0U;
	}
	if (fits) {
		reader->at += count;
	} else {
		reader->failed = true;
	}
}

uint8_t vasal_engine_bytes_read_u8(vasal_engine_bytes_reader_s* const reader) {
	uint8_t value = 0U;
	vasal_engine_bytes_read(reader, &value, 1U);
	return value;
}

uint16_t vasal_engine_bytes_read_u16(
	vasal_engine_bytes_reader_s* const reader) {
	uint8_t bytes[2] = { 0U, 0U };
	vasal_engine_bytes_read(reader, bytes, sizeof(bytes));
	const uint32_t value = (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U);
	return (uint16_t)value;
}

uint32_t vasal_engine_bytes_read_u32(
	vasal_engine_bytes_reader_s* const reader) {
	uint8_t bytes[4] = { 0U, 0U, 0U, 0U };
	vasal_engine_bytes_read(reader, bytes, sizeof(bytes));
	return (uint32_t)bytes[0] | ((uint32_t)bytes[1] << 8U)
		| ((uint32_t)bytes[2] << 16U) | ((uint32_t)bytes[3] << 24U);
}
