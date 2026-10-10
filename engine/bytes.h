
/// Little-endian values in byte buffers, for files and network messages.
/// Writers and readers check every bound: an access past the end does
/// nothing but mark the writer or reader as failed, so a sequence of calls
/// needs only one check at its end.

#ifndef vasal__engine__bytes_h
#define vasal__engine__bytes_h

#include "engine/types.h"

/// Writes into a buffer of fixed capacity.
typedef struct vasal_engine_bytes_writer_s {
	uint8_t* data;
	size_t   capacity;
	/// Bytes written so far.
	size_t   size;
	/// Whether a write did not fit; nothing is written after that.
	bool_t   failed;
} vasal_engine_bytes_writer_s;

/// Reads from a buffer of fixed size.
typedef struct vasal_engine_bytes_reader_s {
	const uint8_t* data;
	size_t         size;
	/// Bytes read so far.
	size_t         at;
	/// Whether a read went past the end; reads return 0 after that.
	bool_t         failed;
} vasal_engine_bytes_reader_s;

/// A writer at the start of the buffer.
vasal_engine_bytes_writer_s vasal_engine_bytes_writer(uint8_t* const data,
	const size_t capacity);

void vasal_engine_bytes_write_u8(vasal_engine_bytes_writer_s* const writer,
	const uint8_t value);
void vasal_engine_bytes_write_u16(vasal_engine_bytes_writer_s* const writer,
	const uint16_t value);
void vasal_engine_bytes_write_u32(vasal_engine_bytes_writer_s* const writer,
	const uint32_t value);

/// Writes count bytes as they are.
void vasal_engine_bytes_write(vasal_engine_bytes_writer_s* const writer,
	const uint8_t* const bytes, const size_t count);

/// A reader at the start of the buffer.
vasal_engine_bytes_reader_s vasal_engine_bytes_reader(
	const uint8_t* const data, const size_t size);

uint8_t vasal_engine_bytes_read_u8(vasal_engine_bytes_reader_s* const reader);
uint16_t vasal_engine_bytes_read_u16(
	vasal_engine_bytes_reader_s* const reader);
uint32_t vasal_engine_bytes_read_u32(
	vasal_engine_bytes_reader_s* const reader);

/// Reads count bytes into bytes, or zeros when they are not there.
void vasal_engine_bytes_read(vasal_engine_bytes_reader_s* const reader,
	uint8_t* const bytes, const size_t count);

#endif
