/*
 *
 *   Copyright (C) 2012-2020 by C.H. Huang
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

#ifndef UG_JSON_H
#define UG_JSON_H

#include <stdint.h>     // int64_t
#include <inttypes.h>   // PRId64, PRIu64
#include <UgBuffer.h>

#ifdef __cplusplus
extern "C" {
#endif

// ----------------------------------------------------------------------------
// UgJsonType & UgJsonError

typedef enum {
	UG_JSON_NULL,       // null
	UG_JSON_TRUE,       // true
	UG_JSON_FALSE,      // false
	UG_JSON_NUMBER,     // 1234, 0.567
	UG_JSON_STRING,     // "string"
	UG_JSON_OBJECT,     // {
	UG_JSON_ARRAY,      // [

	UG_JSON_N_TYPE,     // numbers of UgJsonType
} UgJsonType;

typedef enum {
	UG_JSON_ERROR_NONE              =  0,

	// error code < 0, JSON format error
	UG_JSON_ERROR_UNKNOWN           = -1,
	UG_JSON_ERROR_INVALID_NAME      = -2,
	UG_JSON_ERROR_EXCESS_NAME       = -3,   // get 2 names in 1 element
	UG_JSON_ERROR_BEFORE_COLON      = -4,   // no name before colon in object
	UG_JSON_ERROR_EXCESS_COLON      = -5,   // get 2 colons in 1 element
//	UG_JSON_ERROR_NO_VALUE          = -6,
	UG_JSON_ERROR_INVALID_VALUE     = -7,   // not true, false, null, string, and number.
	UG_JSON_ERROR_EXCESS_VALUE      = -8,   // get 2 values in 1 element
	UG_JSON_ERROR_INVALID_CONTROL   = -9,   // unknown control character after '\'
	UG_JSON_ERROR_INVALID_UNICODE   = -10,
	UG_JSON_ERROR_INVALID_NUMBER    = -11,

	UG_JSON_ERROR_IN_OBJECT         = -12,
	UG_JSON_ERROR_IN_ARRAY          = -13,
	UG_JSON_ERROR_IN_SEPARATOR      = -14,  // ','

	UG_JSON_ERROR_UNCOMPLETED       = -15,

	// --------------------------------
	// JSON-RPC error code
	UG_JSON_ERROR_RPC_INVALID       = -32,

	// --------------------------------
	// error code > 0, user (application) error
	// used by user parser (e.g. UgArray, UgList, UgEntry)
	UG_JSON_ERROR_TYPE_NOT_MATCH    = 1,
	UG_JSON_ERROR_NO_DESTINATION    = 2,
	UG_JSON_ERROR_OUT_OF_MEMORY     = 3,
	UG_JSON_ERROR_CUSTOM            = 4,
} UgJsonError;

// ----------------------------------------------------------------------------
// UgJson: a simple JSON parser & writer

typedef struct	UgJson          UgJson;

// UgJsonParseFunc : a callback function to parse data
typedef UgJsonError  (*UgJsonParseFunc) (UgJson* json,
                                         const char* name, const char* value,
                                         void* dest, void* data);
typedef void         (*UgJsonWriteFunc) (UgJson* json, void* src, void* data);

// ----------------------------------------------------------------------------
// JSON initialize & finalize functions

void  ug_json_init (UgJson* json);
void  ug_json_final (UgJson* json);

// ----------------------------------------------------------------------------
// JSON Parser functions

void         ug_json_begin_parse (UgJson* json);
UgJsonError  ug_json_end_parse   (UgJson* json);

UgJsonError  ug_json_parse (UgJson* json, const char* string, int len);

// Don't call ug_json_pop() directly.
void         ug_json_set  (UgJson* json, UgJsonParseFunc func, void* dest, void* data);
void         ug_json_push (UgJson* json, UgJsonParseFunc func, void* dest, void* data);
void         ug_json_pop  (UgJson* json);

// UgJsonParseFunc for JSON that starting with array.
// This parser will be popped if JSON start with array, otherwise it pop parser twice.
// User must push it after UserParseFunc. Below is sample code.
// ug_json_push (json, UserParseFunc, UserData1, UserData2);
// ug_json_push (json, ug_json_parse_array, NULL, NULL);
UgJsonError  ug_json_parse_array (UgJson* json,
                                  const char* name, const char* value,
                                  void* dest, void* data);
// UgJsonParseFunc for JSON that starting with object.
// This parser will be popped if JSON start with object, otherwise it pop parser twice.
// User must push it after UserParseFunc. Below is sample code.
// ug_json_push (json, UserParseFunc, UserData1, UserData2);
// ug_json_push (json, ug_json_parse_object, NULL, NULL);
UgJsonError  ug_json_parse_object (UgJson* json,
                                   const char* name, const char* value,
                                   void* dest, void* data);
// UgJsonParseFunc for unknown object or array.
// User must push this function in UserParseFunc if you get unknown object or array.
// ug_json_push (json, ug_json_parse_unknown, NULL, NULL);
UgJsonError  ug_json_parse_unknown (UgJson* json,
                                    const char* name, const char* value,
                                    void* dest, void* data);

// ----------------------------------------------------------------------------
// JSON Writer functions

// UgJson.state = UgJsonFormat
// UgJson.index[0] = level
// UgJson.stack.at[0] = UgBuffer

typedef enum
{
	UG_JSON_FORMAT_INDENT = 1,
	UG_JSON_FORMAT_UTF8   = 2,
	UG_JSON_FORMAT_ALL    = UG_JSON_FORMAT_INDENT | UG_JSON_FORMAT_UTF8
} UgJsonFormat;

// UgJson Writer functions
void    ug_json_begin_write (UgJson* json, UgJsonFormat format, UgBuffer* buffer);
void    ug_json_end_write   (UgJson* json);

void    ug_json_write_head (UgJson* json, char ch);
void    ug_json_write_tail (UgJson* json, char ch);

// void ug_json_write_object_head (UgJson* json)
// void ug_json_write_object_tail (UgJson* json)
// void ug_json_write_array_head (UgJson* json)
// void ug_json_write_array_tail (UgJson* json)
#define ug_json_write_object_head(json)		ug_json_write_head (json, '{')
#define ug_json_write_object_tail(json)		ug_json_write_tail (json, '}')
#define ug_json_write_array_head(json)		ug_json_write_head (json, '[')
#define ug_json_write_array_tail(json)		ug_json_write_tail (json, ']')

void    ug_json_write_null   (UgJson* json);
void    ug_json_write_bool   (UgJson* json, int value);

void    ug_json_write_number (UgJson* json, const char* format, ...);
void    ug_json_write_string (UgJson* json, const char* Cstring);

// void ug_json_write_int    (UgJson* json, int value);
// void ug_json_write_uint   (UgJson* json, unsigned int value);
// void ug_json_write_int64  (UgJson* json, int64_t  value);
// void ug_json_write_uint64 (UgJson* json, uint64_t value);
// void ug_json_write_double (UgJson* json, double   value);
#define ug_json_write_int(json, value)      ug_json_write_number (json, "%d", value)
#define ug_json_write_uint(json, value)     ug_json_write_number (json, "%u", value)
#define ug_json_write_double(json, value)   ug_json_write_number (json, "%f", value)
// PRId64/PRIu64 rather than the MSVC-only "%I64d": MinGW's ANSI stdio does
// not understand the %I64 prefix, so it would serialize int64 values wrongly.
#define ug_json_write_int64(json, value)    ug_json_write_number (json, "%" PRId64, (int64_t) (value))
#define ug_json_write_uint64(json, value)   ug_json_write_number (json, "%" PRIu64, (uint64_t) (value))

#ifdef __cplusplus
}
#endif

// ----------------------------------------------------------------------------
// UgJson structure

struct UgJson
{
	// buffer is used by parser & writer
	struct {
		char*  at;
		int    length;
		int    allocated;
	} buf;

	// stack can store state, function, and data.
	struct {
		void** at;
		int    length;
		int    allocated;
	} stack;

	// User's parser can read type and scope only.
	// used by parser & writer
	uint8_t  scope;                 // UG_JSON_OBJECT or UG_JSON_ARRAY
	uint8_t  state;                 // Parser state, Writer UgJsonFormat
	int8_t   error;
	// temporary state
	uint8_t  type;                  // UgJsonType
	uint8_t  count;                 // count value (true, false, null, unicode) length
	uint8_t  colon:1;               // boolean, has ':' in object.
	uint8_t  numberPoint:1;         // boolean, has '.' in number.
	uint8_t  numberEe:1;            // boolean, has 'E' or 'e' in number.
	uint8_t  numberPm:1;            // boolean, has '+' or '-' in number.

	// parser index[0] = index of name
	//        index[1] = index of value
	// writer index[0] = level
	int       index[2];

#ifdef __cplusplus
// C++11 standard-layout
	inline UgJson (void)
		{ ug_json_init (this); }
	inline ~UgJson (void)
		{ ug_json_final (this); }

	inline void init (void)
		{ ug_json_init (this); }
	inline void final (void)
		{ ug_json_final (this); }

	// parser
	inline void  beginParse(void)
		{ ug_json_begin_parse (this); }
	inline void  endParse (void)
		{ ug_json_end_parse (this); }
	inline void  parse (const char* string, int length)
		{ ug_json_parse (this, string, length); }
	inline void  push (UgJsonParseFunc func, void* dest, void* data)
		{ ug_json_push (this, func, dest, data); }
	inline void  pop (void)
		{ ug_json_pop (this); }

	// writer
	inline void  beginWrite (UgJsonFormat format, UgBuffer* buffer)
		{ ug_json_begin_write (this, format, buffer); }
	inline void  endWrite (void)
		{ ug_json_end_write (this); }

//	inline void  writeHead (char ch)
//		{ ug_json_write_head (this, ch); }
//	inline void  writeTail (char ch)
//		{ ug_json_write_tail (this, ch); }
	inline void  writeObjectHead (void)
		{ ug_json_write_object_head (this); }
	inline void  writeObjectTail (void)
		{ ug_json_write_object_tail (this); }
	inline void  writeArrayHead (void)
		{ ug_json_write_array_head (this); }
	inline void  writeArrayTail (void)
		{ ug_json_write_array_tail (this); }

	inline void  writeNull (void)
		{ ug_json_write_null (this); }
	inline void  writeBool (int value)
		{ ug_json_write_bool (this, value); }
//	void  writeNumber (const char* format, ...)
	inline void  writeString (const char* Cstring)
		{ ug_json_write_string (this, Cstring); }

	inline void  writeInt (int value)
		{ ug_json_write_int (this, value); }
	inline void  writeUint (unsigned int value)
		{ ug_json_write_uint (this, value); }
	inline void  writeInt64 (int64_t value)
		{ ug_json_write_int64 (this, value); }
	inline void  writeDouble (double value)
		{ ug_json_write_double (this, value); }
#endif // __cplusplus
};

// ----------------------------------------------------------------------------
// C++11 standard-layout

#ifdef __cplusplus

namespace Ug
{
// This one is for directly use only. You can NOT derived it.
typedef struct UgJson    Json;
};  // namespace Ug

#endif  // __cplusplus


#endif  // UG_JSON_H

