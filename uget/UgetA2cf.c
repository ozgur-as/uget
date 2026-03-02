/*
 *
 *   Copyright (C) 2011-2020 by C.H. Huang
 *   plushuang.tw@gmail.com
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *  ---
 *
 *  In addition, as a special exception, the copyright holders give
 *  permission to link the code of portions of this program with the
 *  OpenSSL library under certain conditions as described in each
 *  individual source file, and distribute linked combinations
 *  including the two.
 *  You must obey the GNU Lesser General Public License in all respects
 *  for all of the code used other than OpenSSL.  If you modify
 *  file(s) with this exception, you may extend this exception to your
 *  version of the file(s), but you are not obligated to do so.  If you
 *  do not wish to do so, delete this exception statement from your
 *  version.  If you delete this exception statement from all source
 *  files in the program, then also delete it here.
 *
 */

#include <stdlib.h>
#include <memory.h>
#include <UgDefine.h>
#include <UgStdio.h>
#include <UgetA2cf.h>

#define INFO_HASH_LEN_MAX    8192

enum {
	ENDIAN_UNKNOWN,
	ENDIAN_BE,
	ENDIAN_LE,
};

union un_int16
{
	uint16_t  integer;
	uint8_t   bytes[2];
};

union un_int32
{
	uint32_t  integer;
	uint8_t   bytes[4];
};

union un_int64
{
	uint64_t  integer;
	uint8_t   bytes[8];
};

static int  endian_type = ENDIAN_UNKNOWN;

int   init_endian_type (void)
{
	union un_int16  value;

	if (endian_type == ENDIAN_UNKNOWN) {
		value.integer = 1;
		if (value.bytes[0] == 0)
			endian_type = ENDIAN_BE;
		else
			endian_type = ENDIAN_LE;
	}
	return endian_type;
}

void  swap_le_be (uint8_t* dest, const uint8_t* src, int length)
{
	int    index;

	for (index = 0;  index < length;  index++) {
		length--;
		dest[index]  = src[length];
		dest[length] = src[index];
	}
}

uint16_t uint16_to_be (uint16_t value)
{
	union un_int16  value16;

	if (endian_type != ENDIAN_BE) {
		swap_le_be (value16.bytes, (uint8_t*)&value, 2);
		return value16.integer;
	}
	return value;
}

uint32_t uint32_to_be (uint32_t value)
{
	union un_int32  value32;

	if (endian_type != ENDIAN_BE) {
		swap_le_be (value32.bytes, (uint8_t*)&value, 4);
		return value32.integer;
	}
	return value;
}

uint64_t uint64_to_be (uint64_t value)
{
	union un_int64  value64;

	if (endian_type != ENDIAN_BE) {
		swap_le_be (value64.bytes, (uint8_t*)&value, 8);
		return value64.integer;
	}
	return value;
}

#define uint16_from_be    uint16_to_be
#define uint32_from_be    uint32_to_be
#define uint64_from_be    uint64_to_be

static int   find_bit0 (uint8_t* bytes_beg, uint32_t bytes_len, uint32_t* beg_bit);
static int   find_bit1 (uint8_t* bytes_beg, uint32_t bytes_len, uint32_t* beg_bit);
static void  fill_bits (uint8_t* bytes, uint32_t nth_bit, uint32_t n_bits);
static int   test_bit (uint8_t* bytes, uint32_t nth_bit);
static void  set_bit (uint8_t* bytes, uint32_t nth_bit);

// ----------------------------------------------------------------------------

#define A2CF_LAST_PIECE_LEN(a2cf)  ((a2cf)->total_size % (a2cf)->piece_len)

static UgetA2cfPiece*  a2cf_piece_new (uint32_t length)
{
	UgetA2cfPiece* piece;
	uint32_t bitfield_len;

//	bitfield_len = length / (16384*8);
	bitfield_len  = length >> (14+3);
	bitfield_len += (length & (16384*8-1)) ? 1 : 0;
	piece = ug_malloc0 (sizeof(UgetA2cfPiece) + bitfield_len);
//	piece->self = piece;
	piece->length = length;
	piece->bitfield_len = bitfield_len;
	return piece;
}

static int  a2cf_piece_read (UgetA2cfPiece* piece, FILE* file)
{
	uint32_t  bitfield_len = piece->bitfield_len;

	(void) ug_fread (file, (char*)&piece->index, 4);
	(void) ug_fread (file, (char*)&piece->length, 4);
	(void) ug_fread (file, (char*)&piece->bitfield_len, 4);
	piece->index = uint32_from_be (piece->index);
	piece->length = uint32_from_be (piece->length);
	piece->bitfield_len = uint32_from_be (piece->bitfield_len);

	if (bitfield_len < piece->bitfield_len)
		return FALSE;
//	piece->bitfield = ug_malloc (piece->bitfield_len);
	(void) ug_fread (file, piece->bitfield, piece->bitfield_len);
	return TRUE;
}

static void a2cf_piece_write (UgetA2cfPiece* piece, FILE* file)
{
	union un_int32  value32;

	value32.integer = uint32_to_be (piece->index);
	ug_fwrite (file, value32.bytes, 4);
	value32.integer = uint32_to_be (piece->length);
	ug_fwrite (file, value32.bytes, 4);
	value32.integer = uint32_to_be (piece->bitfield_len);
	ug_fwrite (file, value32.bytes, 4);

	if (piece->bitfield_len)
		ug_fwrite (file, piece->bitfield, piece->bitfield_len);
}

static void a2cf_piece_truncate (UgetA2cfPiece* piece, uint32_t length)
{
	piece->length = length;
//	piece->bitfield_len  =  piece->length / (16384*8);
	piece->bitfield_len  =  piece->length >> (14+3);
//	piece->bitfield_len += (piece->length % (16384*8)) ? 1 : 0;
	piece->bitfield_len += (piece->length & (16384*8-1)) ? 1 : 0;
}

static int  a2cf_piece_filled (UgetA2cfPiece* piece)
{
	uint8_t  mask;
	uint8_t* bytes, *bytes_end;
	uint32_t n_bits;

	bytes = piece->bitfield;
	bytes_end = bytes + piece->bitfield_len;
//	n_bits = (piece->length / 16384) % 8;
	n_bits = (piece->length >> 14) & 7;
//	if (piece->length & 16384-1) {
	if (piece->length & 16383) {
		if (n_bits == 7)
			n_bits = 0;
		else
			bytes_end--;
	}

	for (;  bytes < bytes_end;  bytes++) {
		if (bytes[0] != 0xFF)
			return FALSE;
	}

	for (mask = 0x80;  n_bits > 0;  n_bits--, mask >>= 1) {
		if (bytes[0] & mask)
			continue;
		return FALSE;
	}
	return TRUE;
}

static int  a2cf_piece_lack (UgetA2cfPiece* piece, uint32_t* beg, uint32_t* end)
{
	uint32_t  bit_beg, bit_end;
	uint32_t  bit_limit;

//	bit_limit = (piece->length / 16384) + ((piece->length % 16384) ? 1 : 0);
	bit_limit = (piece->length >> 14) + ((piece->length & 16383) ? 1 : 0);
//	bit_beg = beg[0] / 16384;
	bit_beg = beg[0] >> 14;

	if (find_bit0 (piece->bitfield, piece->bitfield_len, &bit_beg) == FALSE)
		return FALSE;
	if (bit_beg >= bit_limit)
		return FALSE;

//	beg[0] = bit_beg * 16384;
	beg[0] = bit_beg << 14;
	bit_end = bit_beg + 1;
	if (find_bit1 (piece->bitfield, piece->bitfield_len, &bit_end) == FALSE)
		end[0] = piece->length;
	else {
		if (bit_end >= bit_limit)
			end[0] = piece->length;
		else
			end[0] = bit_end << 14;
//			end[0] = bit_end * 16384;
	}
	return TRUE;
}

static uint64_t  a2cf_piece_completed (UgetA2cfPiece* piece)
{
	uint32_t  bit_beg;
	uint32_t  bit_limit;
	uint32_t  last_bit_len;
	uint64_t  completed;

//	bit_limit = (piece->length / 16384) + ((piece->length % 16384) ? 1 : 0);
	bit_limit = (piece->length >> 14) + ((piece->length & 16383) ? 1 : 0);
	bit_beg = 0;
	completed = 0;

	while (find_bit1 (piece->bitfield, piece->bitfield_len, &bit_beg)) {
		if (bit_beg >= bit_limit)
			break;
		if (bit_beg == bit_limit - 1) {
			last_bit_len = piece->length & 16383;
			if (last_bit_len) {
				completed += last_bit_len;
				break;
			}
		}
		completed += 16384;
		bit_beg++;
	}
	return completed;
}

// ----------------------------------------------------------------------------

static const uint64_t size_piece[14] = {
	(uint64_t) 1    * 8 * 16384,    // index, shift = 0
	(uint64_t) 2    * 8 * 16384,
	(uint64_t) 4    * 8 * 16384,
	(uint64_t) 8    * 8 * 16384,    // index, shift = 3,  20 [default]
	(uint64_t) 16   * 8 * 16384,
	(uint64_t) 32   * 8 * 16384,
	(uint64_t) 64   * 8 * 16384,    // index, shift = 6,  23
	(uint64_t) 128  * 8 * 16384,
	(uint64_t) 256  * 8 * 16384,
	(uint64_t) 512  * 8 * 16384,    // index, shift = 9,  26
	(uint64_t) 1024 * 8 * 16384,
	(uint64_t) 2048 * 8 * 16384,
	(uint64_t) 4096 * 8 * 16384,    // index, shift = 12, 29
	(uint64_t) 8192 * 8 * 16384,
};

void  uget_a2cf_init (UgetA2cf* a2cf, uint64_t size)
{
	int       index;
	uint64_t  piece_size;

	memset (a2cf, 0, sizeof (UgetA2cf));
	a2cf->ver = 1;
	a2cf->total_len = size;
	// piece size
	for (index = 0;  index < 14;  index++) {
		piece_size = size_piece[index];
		if (index < 3) {
			if (size < piece_size)
				break;
		}
		else if (size < piece_size * UINT32_MAX)
			break;
	}
//	a2cf->piece_len = (uint32_t) (8 * 16384) << index;
	a2cf->piece_len = (uint32_t) 1 << (3 + 14 + index);

//	a2cf->bitfield_len  = (uint32_t)(size / (8 * a2cf->piece_len))
//	a2cf->bitfield_len += (uint32_t)(size % (8 * a2cf->piece_len)) ? 1 : 0;
	a2cf->bitfield_len  = (uint32_t)(size >> (3+3+14+index));
	a2cf->bitfield_len += (uint32_t)(size & (8*a2cf->piece_len-1)) ? 1 : 0;
	a2cf->bitfield = (uint8_t*) ug_malloc0 (a2cf->bitfield_len);
	// piece
//	a2cf->piece.index_end  = (uint32_t) (size / a2cf->piece_len);
//	a2cf->piece.index_end += (uint32_t) (size % a2cf->piece_len) ? 1 : 0;
	a2cf->piece.index_end  = (uint32_t) (size >> (3+14+index));
	a2cf->piece.index_end += (uint32_t) (size & (a2cf->piece_len-1)) ? 1 : 0;
	ug_list_init (&a2cf->piece.list);
}

void  uget_a2cf_clear (UgetA2cf* a2cf)
{
	ug_free (a2cf->info_hash);
	ug_free (a2cf->bitfield);
	a2cf->info_hash = NULL;
	a2cf->bitfield = NULL;
	a2cf->info_hash_len = 0;
	a2cf->bitfield_len = 0;
	// piece
	ug_list_foreach (&a2cf->piece.list, (UgForeachFunc) ug_free, NULL);
	ug_list_clear (&a2cf->piece.list, FALSE);
}

int  uget_a2cf_load (UgetA2cf* a2cf, const char* filename)
{
	UgetA2cfPiece*  piece;
	FILE*    file;
	uint32_t index;
	uint32_t n_pieces;
	uint32_t bitfield_len;

	init_endian_type ();

	file = ug_fopen (filename, "rb");
	if (file == NULL)
		return FALSE;
	// version
	(void) ug_fread (file, (char*)&a2cf->ver, 2);
	(void) ug_fread (file, (char*)&a2cf->ext, 4);
	a2cf->ver = uint16_from_be (a2cf->ver);
	a2cf->ext = uint32_from_be (a2cf->ext);
	// check file version - uGet only support version 1
	if (a2cf->ver != 1)
		goto failed;

	// info hash
	a2cf->info_hash_len = 0;
	(void) ug_fread (file, (char*)&a2cf->info_hash_len, 4);
	a2cf->info_hash_len = uint32_from_be (a2cf->info_hash_len);
	if (a2cf->info_hash_len > INFO_HASH_LEN_MAX)
		goto failed;
	else if (a2cf->info_hash_len == 0)
		a2cf->info_hash = NULL;
	else {
		a2cf->info_hash = ug_malloc (a2cf->info_hash_len);
		(void) ug_fread (file, a2cf->info_hash, a2cf->info_hash_len);
	}

	(void) ug_fread (file, (char*)&a2cf->piece_len, 4);
	(void) ug_fread (file, (char*)&a2cf->total_len, 8);
	(void) ug_fread (file, (char*)&a2cf->upload_len, 8);
	a2cf->piece_len = uint32_from_be (a2cf->piece_len);
	a2cf->total_len = uint64_from_be (a2cf->total_len);
	a2cf->upload_len = uint64_from_be (a2cf->upload_len);

	// bitfield
	if (ug_fread (file, (char*)&a2cf->bitfield_len, 4) != 4)
		goto failed;
	a2cf->bitfield_len = uint32_from_be (a2cf->bitfield_len);
	// check bitfield_len
	bitfield_len  = (uint32_t)(a2cf->total_len / (8 * a2cf->piece_len));
	bitfield_len += (uint32_t)(a2cf->total_len % (8 * a2cf->piece_len)) ? 1 : 0;
	if (a2cf->bitfield_len == 0 || a2cf->bitfield_len != bitfield_len) {
		a2cf->bitfield = NULL;
		goto failed;
	}
	else {
		a2cf->bitfield = ug_malloc (a2cf->bitfield_len);
		if (ug_fread (file, a2cf->bitfield, a2cf->bitfield_len) != a2cf->bitfield_len)
			goto failed;
	}

	// The number of in-flight pieces.
	n_pieces = 0;
	(void) ug_fread (file, (char*)&n_pieces, 4);
	n_pieces = uint32_from_be (n_pieces);

	// piece.index_end - calculate number of the last piece
	a2cf->piece.index_end = (uint32_t) (a2cf->total_len / a2cf->piece_len) +
			( (a2cf->total_len % a2cf->piece_len) ? 1 : 0 );

	// load in-flight pieces
	for (index = 0;  index < n_pieces;  index++) {
		piece = a2cf_piece_new (a2cf->piece_len);
		if (a2cf_piece_read (piece, file) == FALSE) {
			ug_free (piece);
			break;
		}
		ug_list_append (&a2cf->piece.list, (UgLink*) piece);
	}

	fclose (file);
	return TRUE;

failed:
	fclose(file);
	return FALSE;
}

int   uget_a2cf_save (UgetA2cf* a2cf, const char* filename)
{
	UgetA2cfPiece*  piece;
	FILE*    file;
	uint32_t n_pieces;
	union {
		union un_int16  value16;
		union un_int32  value32;
		union un_int64  value64;
	} temp;

	init_endian_type ();

	// try to update existing file.
	file = ug_fopen (filename, "rb+");
	if (file == NULL)
		file = ug_fopen (filename, "wb");
	if (file == NULL)
		return FALSE;

	temp.value16.integer = uint16_to_be (a2cf->ver);
	ug_fwrite (file, temp.value16.bytes, 2);
	temp.value32.integer = uint32_to_be (a2cf->ext);
	ug_fwrite (file, temp.value32.bytes, 4);
	temp.value32.integer = uint32_to_be (a2cf->info_hash_len);
	ug_fwrite (file, temp.value32.bytes, 4);

	if (a2cf->info_hash)
		ug_fwrite (file, a2cf->info_hash, a2cf->info_hash_len);

	temp.value32.integer = uint32_to_be (a2cf->piece_len);
	ug_fwrite (file, temp.value32.bytes, 4);
	temp.value64.integer = uint64_to_be (a2cf->total_len);
	ug_fwrite (file, temp.value64.bytes, 8);
	temp.value64.integer = uint64_to_be (a2cf->upload_len);
	ug_fwrite (file, temp.value64.bytes, 8);
	temp.value32.integer = uint32_to_be (a2cf->bitfield_len);
	ug_fwrite (file, temp.value32.bytes, 4);

	if (a2cf->bitfield_len)
		ug_fwrite (file, a2cf->bitfield, a2cf->bitfield_len);

	n_pieces = a2cf->piece.list.size;
	temp.value32.integer = uint32_to_be (n_pieces);
	ug_fwrite (file, temp.value32.bytes, 4);

	for (piece = (void*)a2cf->piece.list.head;  piece;  piece = piece->next)
		a2cf_piece_write (piece, file);

#ifndef __ANDROID__
//	ug_ftruncate (file, ug_ftell (file));  // for updating existing file.
#endif
	fclose (file);
	return TRUE;
}

int   uget_a2cf_lack (UgetA2cf* a2cf, uint64_t* beg, uint64_t* end)
{
	UgetA2cfPiece*  piece;
	uint32_t  index;
	uint32_t  piece_beg;
	uint32_t  piece_end;

	// check
	if (beg[0] == a2cf->total_len)
		return FALSE;

	index     = (uint32_t) (beg[0] / a2cf->piece_len);
	piece_beg = (uint32_t) (beg[0] % a2cf->piece_len);

	// find begin
	for (;  index < a2cf->piece.index_end;  index++) {
		// test a2cf->bitfield
		if (test_bit (a2cf->bitfield, index) == TRUE) {
			piece_beg = 0;
			continue;
		}
		// find begin in piece
		piece = uget_a2cf_find (a2cf, index);
		if (piece) {
			if (a2cf_piece_lack (piece, &piece_beg, &piece_end)) {
				if (piece_end != piece->length) {
					beg[0] = (uint64_t)index * a2cf->piece_len;
					end[0] = beg[0] + piece_end;
					beg[0] = beg[0] + piece_beg;
					return TRUE;
				}
			}
			else {
				piece_beg = 0;
				continue;
			}
		}
		beg[0] = (uint64_t)index * a2cf->piece_len + piece_beg;
		// if current piece is the last piece
		if (index == a2cf->piece.index_end - 1) {
			end[0] = a2cf->total_len;
			return TRUE;
		}
		break;
	}
	if (index >= a2cf->piece.index_end)
		return FALSE;

	// find end
	for (index += 1;  index < a2cf->piece.index_end;  index++) {
		// test a2cf->bitfield
		if (test_bit (a2cf->bitfield, index) == TRUE) {
			end[0] = (uint64_t)index * a2cf->piece_len;
			return TRUE;
		}
		// find end in piece
		piece = uget_a2cf_find (a2cf, index);
		if (piece) {
			piece_beg = 0;
			piece_end = piece->length;
			a2cf_piece_lack (piece, &piece_beg, &piece_end);
			if (piece_beg != 0) {
				end[0] = (uint64_t)index * a2cf->piece_len;
				return TRUE;
			}
			if (piece_end != piece->length) {
				end[0] = (uint64_t)index * a2cf->piece_len + piece_end;
				return TRUE;
			}
		}
	}

	end[0] = a2cf->total_len;
	return TRUE;
}

static void uget_a2cf_fill_piece (UgetA2cf* a2cf, uint32_t index, uint32_t beg, uint32_t end)
{
	UgetA2cfPiece*  piece;
	uint32_t        bit_beg, bit_end;

	if (test_bit (a2cf->bitfield, index) == FALSE) {
		if (beg == end)
			return;
		piece = uget_a2cf_realloc (a2cf, index);
		if (end == 0)
			end = piece->length;

		//	bit_beg = beg / 16384;
		bit_beg = beg >> 14;
		bit_end = end >> 14;
		// piece->bitfield_len * 8 == piece->bitfield_len << 3
		if (piece->length == end) {
//			if (piece->length & (16384-1))
			if (piece->length & 16383)
				bit_end++;
		}
		fill_bits (piece->bitfield, bit_beg, bit_end - bit_beg);

		if (a2cf_piece_filled (piece)) {
			set_bit (a2cf->bitfield, index);
			// delete piece
			ug_list_remove (&a2cf->piece.list, (UgLink*)piece);
			ug_free (piece);
		}
	}
}

uint64_t  uget_a2cf_fill (UgetA2cf* a2cf, uint64_t beg, uint64_t end)
{
	UgetA2cfPiece*  piece;
	uint32_t        index;
	uint32_t        index_beg, index_end;
	uint32_t        piece_beg, piece_end;

	index_beg = (uint32_t) (beg / a2cf->piece_len);
	index_end = (uint32_t) (end / a2cf->piece_len);
	piece_beg = (uint32_t) (beg % a2cf->piece_len);
	piece_end = (uint32_t) (end % a2cf->piece_len);

	// first piece or only 1 piece
	if (index_beg == index_end) {
		uget_a2cf_fill_piece (a2cf, index_beg, piece_beg, piece_end);
		goto exit;
	}
	else if (piece_beg > 0) {
		uget_a2cf_fill_piece (a2cf, index_beg, piece_beg, 0);
		piece_beg = 0;
		index_beg++;
	}
	// last piece
	if (piece_end > 0) {
		uget_a2cf_fill_piece (a2cf, index_end, 0, piece_end);
//		piece_end = 0;
	}

	// middle
	for (index = index_beg;  index < index_end;  index++) {
		if (test_bit (a2cf->bitfield, index) == TRUE)
			continue;
		// delete piece
		piece = uget_a2cf_find (a2cf, index);
		if (piece) {
			ug_list_remove (&a2cf->piece.list, (UgLink*)piece);
			ug_free (piece);
		}
	}
	fill_bits (a2cf->bitfield, index_beg, index_end - index_beg);

exit:
	if (end == a2cf->total_len)
		return end;
	return end & ~16383;
}

uint64_t  uget_a2cf_completed (UgetA2cf* a2cf)
{
	UgetA2cfPiece*  piece;
	uint32_t        index;
	uint32_t        last_piece_len;
	uint64_t        completed;

	completed = 0;
	piece = (UgetA2cfPiece*) a2cf->piece.list.head;

	for (index = 0;  index < a2cf->piece.index_end;  index++) {
		if (test_bit (a2cf->bitfield, index) == TRUE) {
			if (index == a2cf->piece.index_end - 1) {
				last_piece_len = a2cf->total_len % a2cf->piece_len;
				if (last_piece_len) {
					completed += last_piece_len;
					break;
				}
			}
			completed += a2cf->piece_len;
			continue;
		}
		if (piece && piece->index == index) {
			completed += a2cf_piece_completed (piece);
			piece = piece->next;
		}
	}

	return completed;
}

void  uget_a2cf_insert (UgetA2cf* a2cf, UgetA2cfPiece* newpiece)
{
	UgetA2cfPiece*  piece;

	if (a2cf->piece.list.head == NULL) {
		ug_list_prepend (&a2cf->piece.list, (UgLink*)newpiece);
		return;
	}

	for (piece = (void*)a2cf->piece.list.head;  piece;  piece = piece->next) {
		if (piece->index > newpiece->index) {
			ug_list_insert (&a2cf->piece.list, (void*)piece, (void*)newpiece);
			return;
		}
	}
	ug_list_append (&a2cf->piece.list, (void*)newpiece);
}

UgetA2cfPiece*  uget_a2cf_find (UgetA2cf* a2cf, uint32_t piece_index)
{
	UgetA2cfPiece*  piece;

	for (piece = (void*)a2cf->piece.list.head;  piece;  piece = piece->next) {
		if (piece->index == piece_index)
			return piece;
		else if (piece->index > piece_index)
			return NULL;
	}
	return NULL;
}

UgetA2cfPiece*  uget_a2cf_realloc (UgetA2cf* a2cf, uint32_t piece_index)
{
	UgetA2cfPiece*  piece;

	piece = uget_a2cf_find (a2cf, piece_index);
	if (piece == NULL) {
		piece = a2cf_piece_new (a2cf->piece_len);
		piece->index = piece_index;
		if (piece_index == a2cf->piece.index_end - 1)
			a2cf_piece_truncate (piece, a2cf->total_len % a2cf->piece_len);
		uget_a2cf_insert (a2cf, piece);
	}
	return piece;
}

// ----------------------------------------------------------------------------

// beg_bit: [in, out]
static int find_bit0 (uint8_t* bytes_beg, uint32_t bytes_len, uint32_t* beg_bit)
{
	int       counts;
	uint8_t*  bytes;
	uint8_t*  bytes_end;
	uint8_t   mask;

	bytes_end = bytes_beg + bytes_len;
	bytes = bytes_beg + (beg_bit[0] >> 3);
	counts = beg_bit[0] & 7;
	if (bytes >= bytes_end)
		return FALSE;

	if (counts != 0) {
		for (mask = 0x80 >> counts;  counts < 8;  counts++, mask >>= 1) {
			if ((bytes[0] & mask) == 0) {
				beg_bit[0] = ((bytes - bytes_beg) << 3) + counts;
				return TRUE;
			}
		}
		bytes++;
	}

	for (;  bytes < bytes_end;  bytes++) {
		if (bytes[0] != 0xFF)
			break;
	}

	if (bytes < bytes_end) {
		for (mask = 0x80, counts = 0;  counts < 8;  counts++, mask >>= 1) {
			if ((bytes[0] & mask) == 0) {
				beg_bit[0] = ((bytes - bytes_beg) << 3) + counts;
				return TRUE;
			}
		}
	}

	return FALSE;
}

// beg_bit: [in, out]
static int find_bit1 (uint8_t* bytes_beg, uint32_t bytes_len, uint32_t* beg_bit)
{
	int       counts;
	uint8_t*  bytes;
	uint8_t*  bytes_end;
	uint8_t   mask;

	bytes_end = bytes_beg + bytes_len;
	bytes = bytes_beg + (beg_bit[0] >> 3);
	counts = beg_bit[0] & 7;
	if (bytes >= bytes_end)
		return FALSE;

	if (counts != 0) {
		for (mask = 0x80 >> counts;  counts < 8;  counts++, mask >>= 1) {
			if (bytes[0] & mask) {
				beg_bit[0] = ((bytes - bytes_beg) << 3) + counts;
				return TRUE;
			}
		}
		bytes++;
	}

	for (;  bytes < bytes_end;  bytes++) {
		if (bytes[0] != 0)
			break;
	}

	if (bytes < bytes_end) {
		for (mask = 0x80, counts = 0;  counts < 8;  counts++, mask >>= 1) {
			if (bytes[0] & mask) {
				beg_bit[0] = ((bytes - bytes_beg) << 3) + counts;
				return TRUE;
			}
		}
	}

	return FALSE;
}

static void  set_bit (uint8_t* bytes, uint32_t nth_bit)
{
	uint8_t  cur_bit;

//	bytes += nth_bit / 8;
	bytes += nth_bit >> 3;
//	nth_bit_beg = beg % 8;
	cur_bit = nth_bit & 7;

	bytes[0] |= (0x80 >> cur_bit);
}

static int   test_bit (uint8_t* bytes, uint32_t nth_bit)
{
	uint8_t  cur_bit;

//	bytes += nth_bit / 8;
	bytes += nth_bit >> 3;
//	cur_bit = nth_bit % 8;
	cur_bit = nth_bit & 7;

	if (bytes[0] & (0x80 >> cur_bit))
		return TRUE;
	else
		return FALSE;
}

static void  fill_bits (uint8_t* bytes, uint32_t nth_bit, uint32_t n_bits)
{
	uint8_t  mask;
	uint8_t  counts;

//	bytes += nth_bit / 8;
	bytes += nth_bit >> 3;

//	nth_bit %= 8;
	nth_bit &= 7;

	if (nth_bit != 0) {
		mask = 0x80;
		if (nth_bit + n_bits >= 8)
			counts = 8 - nth_bit;
		else
			counts = n_bits;

		for (mask >>= nth_bit;  counts > 0;  counts--, mask >>= 1) {
			bytes[0] |= mask;
			nth_bit++;
			n_bits--;
		}
		bytes++;
	}

	for (; n_bits >= 8; n_bits -= 8)
		*bytes++ = 0xFF;

	for (mask = 0x80;  n_bits > 0;  n_bits--, mask >>= 1)
		bytes[0] |= mask;
}

