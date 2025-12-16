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

#if defined(_MSC_VER)
#define _CRT_SECURE_NO_WARNINGS
#if _MSC_VER < 1800
#define strtoll _strtoi64
#endif
#if _MSC_VER < 1900
#define snprintf _snprintf
#endif
#endif

#include <UgJson-custom.h>
#include <limits.h> // INT_MIN, INT_MAX, UINT_MAX
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> // time_t

// ----------------------------------------------------------------------------

UgJsonError ug_json_parse_int_string(UgJson *json, const char *name,
                                     const char *value, void *dest,
                                     void *data) {
  //	if (json->type >= UG_JSON_OBJECT) {
  //		ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
  //		return UG_JSON_ERROR_TYPE_NOT_MATCH;
  //	}

  if (json->type == UG_JSON_STRING || json->type == UG_JSON_NUMBER)
    *(int *)dest = strtol(value, NULL, 10);
  else
    return UG_JSON_ERROR_TYPE_NOT_MATCH;
  return UG_JSON_ERROR_NONE;
}

UgJsonError ug_json_parse_uint_string(UgJson *json, const char *name,
                                      const char *value, void *dest,
                                      void *data) {
  //	if (json->type >= UG_JSON_OBJECT) {
  //		ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
  //		return UG_JSON_ERROR_TYPE_NOT_MATCH;
  //	}

  if (json->type == UG_JSON_STRING || json->type == UG_JSON_NUMBER)
    *(unsigned int *)dest = strtoul(value, NULL, 10);
  else
    return UG_JSON_ERROR_TYPE_NOT_MATCH;
  return UG_JSON_ERROR_NONE;
}

UgJsonError ug_json_parse_int64_string(UgJson *json, const char *name,
                                       const char *value, void *dest,
                                       void *data) {
  //	if (json->type >= UG_JSON_OBJECT) {
  //		ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
  //		return UG_JSON_ERROR_TYPE_NOT_MATCH;
  //	}

  if (json->type == UG_JSON_STRING || json->type == UG_JSON_NUMBER)
    *(int64_t *)dest = strtoll(value, NULL, 10);
  else
    return UG_JSON_ERROR_TYPE_NOT_MATCH;
  return UG_JSON_ERROR_NONE;
}

UgJsonError ug_json_parse_double_string(UgJson *json, const char *name,
                                        const char *value, void *dest,
                                        void *data) {
  //	if (json->type >= UG_JSON_OBJECT) {
  //		ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
  //		return UG_JSON_ERROR_TYPE_NOT_MATCH;
  //	}

  if (json->type == UG_JSON_STRING || json->type == UG_JSON_NUMBER)
    *(double *)dest = strtod(value, NULL);
  else
    return UG_JSON_ERROR_TYPE_NOT_MATCH;
  return UG_JSON_ERROR_NONE;
}

void ug_json_write_int_string(UgJson *json, int *value) {
  int len;
  char *buf;

  len = snprintf(NULL, 0, "%d", *value) + 1;
  buf = ug_malloc(len);
  snprintf(buf, len, "%d", *value);
  ug_json_write_string(json, buf);
  ug_free(buf);
}

void ug_json_write_uint_string(UgJson *json, unsigned int *value) {
  int len;
  char *buf;

  len = snprintf(NULL, 0, "%u", *value) + 1;
  buf = ug_malloc(len);
  snprintf(buf, len, "%u", *value);
  ug_json_write_string(json, buf);
  ug_free(buf);
}

void ug_json_write_int64_string(UgJson *json, int64_t *value) {
  int len;
  char *buf;

#if defined(_MSC_VER) || defined(__MINGW32__)
  len = snprintf(NULL, 0, "%I64d", *value) + 1;
  buf = ug_malloc(len);
  snprintf(buf, len, "%I64d", *value);
#else
  len = snprintf(NULL, 0, "%lld", (long long)*value) + 1;
  buf = ug_malloc(len);
  snprintf(buf, len, "%lld", (long long)*value);
#endif
  ug_json_write_string(json, buf);
  ug_free(buf);
}

void ug_json_write_double_string(UgJson *json, double *value) {
  int len;
  char *buf;

  len = snprintf(NULL, 0, "%f", *value) + 1;
  buf = ug_malloc(len);
  snprintf(buf, len, "%f", *value);
  ug_json_write_string(json, buf);
  ug_free(buf);
}

// ----------------------------------------------------------------------------
// parse and write JSON number for C types - uint8_t, int16_t, and int32_t

UgJsonError ug_json_parse_uint8(UgJson *json, const char *name,
                                const char *value, void *dest, void *data) {
  //	if (json->type >= UG_JSON_OBJECT) {
  //		ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
  //		return UG_JSON_ERROR_TYPE_NOT_MATCH;
  //	}

  if (json->type != UG_JSON_NUMBER)
    return UG_JSON_ERROR_TYPE_NOT_MATCH;
  *(uint8_t *)dest = strtol(value, NULL, 10);
  return UG_JSON_ERROR_NONE;
}

UgJsonError ug_json_parse_int16(UgJson *json, const char *name,
                                const char *value, void *dest, void *data) {
  //	if (json->type >= UG_JSON_OBJECT) {
  //		ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
  //		return UG_JSON_ERROR_TYPE_NOT_MATCH;
  //	}

  if (json->type != UG_JSON_NUMBER)
    return UG_JSON_ERROR_TYPE_NOT_MATCH;
  *(int16_t *)dest = strtol(value, NULL, 10);
  return UG_JSON_ERROR_NONE;
}

UgJsonError ug_json_parse_int32(UgJson *json, const char *name,
                                const char *value, void *dest, void *data) {
  //	if (json->type >= UG_JSON_OBJECT) {
  //		ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
  //		return UG_JSON_ERROR_TYPE_NOT_MATCH;
  //	}

  if (json->type != UG_JSON_NUMBER)
    return UG_JSON_ERROR_TYPE_NOT_MATCH;
  *(int32_t *)dest = strtol(value, NULL, 10);
  return UG_JSON_ERROR_NONE;
}

void ug_json_write_uint8(UgJson *json, uint8_t *value) {
  ug_json_write_int(json, *(uint8_t *)value);
}

void ug_json_write_int16(UgJson *json, int16_t *value) {
  ug_json_write_int(json, *(int16_t *)value);
}

void ug_json_write_int32(UgJson *json, int32_t *value) {
  ug_json_write_int(json, *(int32_t *)value);
}

// ----------------------------------------------------------------------------
// parse string "true" and "false" to integer (boolean).
// write integer (boolean) to string "true" and "false".

UgJsonError ug_json_parse_bool_string(UgJson *json, const char *name,
                                      const char *value, void *dest,
                                      void *data) {
  //	if (json->type >= UG_JSON_OBJECT) {
  //		ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
  //		return UG_JSON_ERROR_TYPE_NOT_MATCH;
  //	}

  switch (json->type) {
  case UG_JSON_TRUE:
    *(int *)dest = TRUE;
    break;

  case UG_JSON_FALSE:
    *(int *)dest = FALSE;
    break;

    //	case UG_JSON_NUMBER:
    //		if (strtol(value, NULL, 10) == 0)
    //			*(int*) dest = FALSE;
    //		else
    //			*(int*) dest = TRUE;
    //		break;

  case UG_JSON_STRING:
    if (strcmp(value, "true") == 0)
      *(int *)dest = TRUE;
    else if (strcmp(value, "false") == 0)
      *(int *)dest = FALSE;
    break;

  default:
    return UG_JSON_ERROR_TYPE_NOT_MATCH;
  }
  return UG_JSON_ERROR_NONE;
}

void ug_json_write_bool_string(UgJson *json, int *value) {
  if (*value == FALSE)
    ug_json_write_string(json, "false");
  else
    ug_json_write_string(json, "true");
}

// ----------------------------------------------------------------------------
// UgJsonParseFunc and UgJsonWriteFunc for time_t

UgJsonError ug_json_parse_time_t(UgJson *json, const char *name,
                                 const char *value, void *dest, void *none) {
#if 1
  if (json->type != UG_JSON_NUMBER)
    return UG_JSON_ERROR_TYPE_NOT_MATCH;
  *(time_t *)dest = strtoll(value, NULL, 10);
  return UG_JSON_ERROR_NONE;
#else
  struct tm timem;

  timem.tm_year = year - 1900;
  timem.tm_mon = month - 1;
  timem.tm_mday = day;
  timem.tm_hour = hour;
  timem.tm_min = minute;
        timem.tm_sec) = second;
        timem.tm_isdst = 0;
        *(time_t *)dest = mktime(timem);

//	if (json->type >= UG_JSON_OBJECT) {
//		ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
//		return UG_JSON_ERROR_TYPE_NOT_MATCH;
//	}
#endif
}

void ug_json_write_time_t(UgJson *json, void *data) {
#if 1
  ug_json_write_int64(json, *(time_t *)data);
#else
  struct tm *timem;
  char *timestr;

  timem = gmtime((time_t *)dest); // localtime((time_t*) dest);
  timestr = ug_malloc(32);
  // output format : "2013-02-05 21:25:15"
  snprintf(timestr, 32, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", timem->tm_year + 1900,
           timem->tm_mon + 1, timem->tm_mday, timem->tm_hour, timem->tm_min,
           timem->tm_sec);
  ug_json_write_string(json, timestr);
  ug_free(timestr);
#endif
}

// ----------------------------------------------------------------------------
// UgJson reparse UgValue: this can convert UgValue to UgEntry
// e.g.
// ug_json_begin_parse(json);
// ug_json_push(json, ug_json_parse_entry, entry, dest, data);
// ug_json_reparse_value(json, value);
// ug_json_end_parse(json);

enum {
  PARSER_STACK_BASE,                     // 0
  PARSER_STACK_TYPE,                     // 1
  PARSER_STACK_DATA2,                    // 2
  PARSER_STACK_DATA1,                    // 3
  PARSER_STACK_FUNC,                     // 4
  PARSER_STACK_UNIT = PARSER_STACK_FUNC, // equal with STACK_MULTIPLY_VALUE
};

// static function
static void ug_json_call_parser_value(UgJson *json, UgValue *uvalue,
                                      const char *value) {
  UgJsonParseFunc parser;
  void **stack;
  char error;
  int stackLen;

  // get parser and data from stack
  if (json->stack.length && json->type < UG_JSON_N_TYPE) {
    stackLen = json->stack.length;
    stack = json->stack.at + json->stack.length;
    parser = *(stack - PARSER_STACK_FUNC);
    error = parser(json, uvalue->name, value, *(stack - PARSER_STACK_DATA1),
                   *(stack - PARSER_STACK_DATA2));
    if (error)
      json->error = error;
    // It must push parser when getting UG_JSON_OBJECT or UG_JSON_ARRAY.
    // If callback function does NOT push any parser, push default one.
    if (json->type >= UG_JSON_OBJECT && stackLen == json->stack.length)
      ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
  }

  // reset state
  json->type = 0; // UG_JSON_VALUE
  // reset buffer
  json->buf.at[0] = 0;
  json->buf.length = 1;
}

void ug_json_parse_by_value(UgJson *json, UgValue *value) {
  UgValue *end;

  switch (value->type) {
  case UG_VALUE_NONE:
  default:
    // skip this value
    break;

  case UG_VALUE_OBJECT:
    json->type = UG_JSON_OBJECT;
    ug_json_call_parser_value(json, value, NULL);
    json->scope = UG_JSON_OBJECT;
    end = value->c.object->at + value->c.object->length;
    for (value = value->c.object->at; value < end; value++)
      ug_json_parse_by_value(json, value);
    ug_json_pop(json);
    break;

  case UG_VALUE_ARRAY:
    json->type = UG_JSON_ARRAY;
    ug_json_call_parser_value(json, value, NULL);
    json->scope = UG_JSON_ARRAY;
    end = value->c.array->at + value->c.array->length;
    for (value = value->c.array->at; value < end; value++)
      ug_json_parse_by_value(json, value);
    ug_json_pop(json);
    break;

  case UG_VALUE_BOOL:
    if (value->c.boolean == FALSE) {
      json->type = UG_JSON_FALSE;
      ug_json_call_parser_value(json, value, "false");
    } else {
      json->type = UG_JSON_TRUE;
      ug_json_call_parser_value(json, value, "true");
    }
    break;

  case UG_VALUE_INT:
    json->type = UG_JSON_NUMBER;
    snprintf(json->buf.at, json->buf.allocated, "%d", value->c.integer);
    ug_json_call_parser_value(json, value, json->buf.at);
    break;

  case UG_VALUE_UINT:
    json->type = UG_JSON_NUMBER;
    snprintf(json->buf.at, json->buf.allocated, "%u", value->c.uinteger);
    ug_json_call_parser_value(json, value, json->buf.at);
    break;

  case UG_VALUE_INT64:
    json->type = UG_JSON_NUMBER;
#if defined(_MSC_VER) || defined(__MINGW32__)
    snprintf(json->buf.at, json->buf.allocated, "%I64d", value->c.integer64);
#else
    snprintf(json->buf.at, json->buf.allocated, "%lld",
             (long long int)value->c.integer64);
#endif
    ug_json_call_parser_value(json, value, json->buf.at);
    break;

  case UG_VALUE_UINT64:
    json->type = UG_JSON_NUMBER;
#if defined(_MSC_VER) || defined(__MINGW32__)
    snprintf(json->buf.at, json->buf.allocated, "%I64u", value->c.uinteger64);
#else
    snprintf(json->buf.at, json->buf.allocated, "%llu",
             (long long int)value->c.uinteger64);
#endif
    ug_json_call_parser_value(json, value, json->buf.at);
    break;

  case UG_VALUE_DOUBLE:
    json->type = UG_JSON_NUMBER;
    snprintf(json->buf.at, json->buf.allocated, "%f", value->c.fraction);
    ug_json_call_parser_value(json, value, json->buf.at);
    break;

  case UG_VALUE_STRING:
    if (value->c.string == NULL) {
      json->type = UG_JSON_NULL;
      ug_json_call_parser_value(json, value, NULL);
    } else {
      json->type = UG_JSON_STRING;
      ug_json_call_parser_value(json, value, value->c.string);
    }
    break;
  }
}
