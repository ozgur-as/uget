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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <UgJson.h>
#include <UgList.h>
#include <UgArray.h>
#include <UgEntry.h>
#include <UgValue.h>
#include <UgJson-custom.h>

// ----------------------------------------------------------------------------
// WorkedId

typedef struct
{
	int     worked;
	char*   id;
} WorkedId;

UgJsonError worked_id_custom_parse (UgJson* json,
                                    const char* name, const char* value,
                                    void* dest, void* none);
void        worked_id_custom_write (UgJson* json, void* data);

UgEntry	WorkedIdEntry[] =
{
	{"worked", offsetof (WorkedId, worked), UG_ENTRY_BOOL,   NULL, NULL},
	{"id",     offsetof (WorkedId, id),     UG_ENTRY_STRING, NULL, NULL},
	{NULL},
};
// used by test_json_object_custom()
UgEntry	WorkedIdCustomEntry[] = {
	{NULL, 0, UG_ENTRY_CUSTOM,
			(UgJsonParseFunc) worked_id_custom_parse,
			(UgJsonWriteFunc) worked_id_custom_write },
	{NULL}
};

// ------------------------------------
// WorkedId functions

void  worked_id_init (WorkedId* wid)
{
	wid->worked = 0;
	wid->id = NULL;
}

void  worked_id_final (WorkedId* wid)
{
	ug_free (wid->id);
}

WorkedId*  worked_id_new (void)
{
	WorkedId*	wid;

	wid = ug_malloc (sizeof (WorkedId));
	worked_id_init(wid);
	return wid;
}

void  worked_id_free (WorkedId* wid)
{
	worked_id_final (wid);
}

// UgEntry.type   = UG_ENTRY_CUSTOM
// UgEntry.param1 = (UgJsonParseFunc) worked_id_custom_parse
UgJsonError worked_id_custom_parse (UgJson* json,
                                    const char* name, const char* value,
                                    void* dest, void* none)
{
	// WorkedId's type is UG_JSON_OBJECT
	if (json->type != UG_JSON_OBJECT) {
//		if (json->type == UG_JSON_ARRAY)
//			ug_json_push (json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	}

	// initialize WorkedId. If you have initialized it, don't initialize again.
//	worked_id_init (dest);

	// push JSON parser, only UG_JSON_OBJECT or UG_JSON_ARRAY need to push parser.
	ug_json_push (json, ug_json_parse_entry, dest, WorkedIdEntry);
	// return error code
	return UG_JSON_ERROR_NONE;
}

// UgEntry.type   = UG_ENTRY_CUSTOM
// UgEntry.param2 = (UgJsonWriteFunc) worked_id_custom_write
void  worked_id_custom_write (UgJson* json, void* data)
{
	ug_json_write_object_head (json);
	ug_json_write_entry (json, data, WorkedIdEntry);
	ug_json_write_object_tail (json);
}

// ----------------------------------------------------------------------------
// WorkedIdArray

typedef UG_ARRAY (WorkedId)    WorkedIdArray;

UgJsonError worked_id_array_parse (UgJson* json,
                                   const char* name, const char* value,
                                   void* array, void* none)
{
	WorkedId* wid;

	// WorkedId's type is UG_JSON_OBJECT
	if (json->type != UG_JSON_OBJECT) {
//		if (json->type == UG_JSON_ARRAY)
//			ug_json_push (json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	}

	wid = ug_array_alloc (array, 1);
	worked_id_init (wid);
	// push JSON parser, only UG_JSON_OBJECT or UG_JSON_ARRAY need to push parser.
	ug_json_push (json, ug_json_parse_entry, wid, WorkedIdEntry);
	// return error code
	return UG_JSON_ERROR_NONE;
}

void  worked_id_array_write (UgJson* json, void* array)
{
	WorkedId* cur;
	WorkedId* end;

	cur = ((WorkedIdArray*)array)->at;
	end = ((WorkedIdArray*)array)->at + ((WorkedIdArray*)array)->length;
	for (;  cur < end;  cur++)
		worked_id_custom_write (json, cur);
}

// ----------------------------------------------------------------------------
// SampleJs

typedef struct
{
	char*  name;

	struct Info {
		struct FtpInfo {
			char*   user;
			char*   password;
			int     active_mode;
		} ftp;

		struct HttpInfo {
			char*   user_agent;
			char*   referrer;
		} http;

		UgArrayStr          sarray;
		UgArrayInt          iarray;
		UG_ARRAY(WorkedId)  oarray;
	} info;
} SampleJs;

// SampleJs.Info.FtpInfo
UgEntry	FtpInfoEntry[] =
{
	{"user",       offsetof (struct FtpInfo, user),       UG_ENTRY_STRING, NULL, NULL},
	{"password",   offsetof (struct FtpInfo, password),   UG_ENTRY_STRING, NULL, NULL},
	{"active-mode",offsetof (struct FtpInfo, active_mode),UG_ENTRY_BOOL,   NULL, NULL},
	{NULL},
};

// SampleJs.Info.HttpInfo
UgEntry	HttpInfoEntry[] =
{
	{"user-agent", offsetof (struct HttpInfo, user_agent), UG_ENTRY_STRING, NULL, NULL},
	{"referrer",   offsetof (struct HttpInfo, referrer),   UG_ENTRY_STRING, NULL, NULL},
	{NULL},
};

// SampleJs.Info
UgEntry	InfoEntry[] =
{
	{"ftp",    offsetof (struct Info, ftp),    UG_ENTRY_OBJECT,
			FtpInfoEntry,  (UgInitFunc) NULL},
	{"http",   offsetof (struct Info, http),   UG_ENTRY_OBJECT,
			HttpInfoEntry, (UgInitFunc) NULL},
	{"sarray", offsetof (struct Info, sarray), UG_ENTRY_ARRAY,
			ug_json_parse_array_string, ug_json_write_array_string},
	{"iarray", offsetof (struct Info, iarray), UG_ENTRY_ARRAY,
			ug_json_parse_array_int, ug_json_write_array_int},
	{"oarray", offsetof (struct Info, oarray), UG_ENTRY_ARRAY,
			worked_id_array_parse, worked_id_array_write},
	{NULL},
};

// SampleJs
UgEntry	 SampleJsEntry[] =
{
	{"name", offsetof (SampleJs, name), UG_ENTRY_STRING,
			NULL,                   NULL},
	{"info", offsetof (SampleJs, info), UG_ENTRY_OBJECT,
			InfoEntry, (UgInitFunc) NULL},
	{NULL},
};
// used by sample_js_parse()
UgEntry  SampleJsObjectEntry[] = {
	{NULL, 0, UG_ENTRY_OBJECT, SampleJsEntry, (UgInitFunc) NULL},
	{NULL}
};

// ------------------------------------
// SampleJs functions

SampleJs*  sample_js_new (void)
{
	SampleJs*  samplejs;

	samplejs = calloc (1, sizeof (SampleJs));
	ug_array_init (&samplejs->info.sarray, sizeof (char*), 0);
	ug_array_init (&samplejs->info.iarray, sizeof (int), 0);
	ug_array_init (&samplejs->info.oarray, sizeof (WorkedId), 0);
	return samplejs;
}

void  sample_js_free (SampleJs* samplejs)
{
	int index;

	if (samplejs == NULL)
		return;

	ug_free (samplejs->name);
	samplejs->name = NULL;

	ug_free (samplejs->info.ftp.user);
	samplejs->info.ftp.user = NULL;
	ug_free (samplejs->info.ftp.password);
	samplejs->info.ftp.password = NULL;

	ug_free (samplejs->info.http.user_agent);
	samplejs->info.http.user_agent = NULL;
	ug_free (samplejs->info.http.referrer);
	samplejs->info.http.referrer = NULL;

	for (index = 0; index < samplejs->info.sarray.length; index++)
		ug_free (samplejs->info.sarray.at[index]);
	ug_array_clear (&samplejs->info.sarray);

	ug_array_clear (&samplejs->info.iarray);

	for (index = 0; index < samplejs->info.oarray.length; index++)
		worked_id_final (&samplejs->info.oarray.at[index]);
	ug_array_clear (&samplejs->info.oarray);

	free (samplejs);
}

// UgJsonParseFunc
UgJsonError json_debug_parser (UgJson* json, const char* name, const char* value, void* dest, void* none)
{
	printf ("scope : %s\n", (json->scope == UG_JSON_ARRAY) ? "A" : "O");
	if (json->type == UG_JSON_ARRAY || json->type == UG_JSON_OBJECT)
		ug_json_push (json, json_debug_parser, NULL, NULL);

	// UgJson.index[0] : index of name
	// UgJson.index[1] : index of value

	// debug
	if (json->index[0])
		printf ("'%s' : ", json->buf.at + json->index[0]);

	switch (json->type) {
	case UG_JSON_NULL:		// null
		printf ("NULL '%s'", json->buf.at + json->index[1]);
		break;
	case UG_JSON_TRUE:		// true
		printf ("TRUE '%s'", json->buf.at + json->index[1]);
		break;
	case UG_JSON_FALSE:		// false
		printf ("FALSE '%s'", json->buf.at + json->index[1]);
		break;
	case UG_JSON_NUMBER:	// 1234, 0.567
		printf ("NUMBER '%s'", json->buf.at + json->index[1]);
		break;
	case UG_JSON_STRING:	// "string"
		printf ("STRING '%s'", json->buf.at + json->index[1]);
		break;
	case UG_JSON_OBJECT:	// {
		printf ("OBJECT");
		break;
	case UG_JSON_ARRAY:		// [
		printf ("ARRAY");
		break;
	}

	printf ("\n");
	return 0;
}

static const char* sample_js_string =
{
	"{"
	"  \"name\": \"\\u5927\\u4E2Dfile.torrent\","
	"  \"info\":"
	"  {"
	"    \"ftp\":"
	"    {"
	"      \"user\": \"anonymous\","
	"      \"password\": \"guest@unknown\","
	"      \"active-mode\": false"
	"    },"
	"    \"http\":"
	"    {"
	"      \"user-agent\": \"Browser\\/version\","
	"      \"referrer\": \"http:\\/\\/127.0.0.1\\/\" "
	"    },"
	"    \"sarray\":"
	"    ["
	"      \"element1\", \"element2\","
	"      \"element3\", \"element\\n4\" "
	"    ],"
	"    \"iarray\":"
	"    ["
	"      1234, 2012,"
	"      4567, 2011 "
	"    ],"
	"    \"oarray\":"
	"    ["
	"      { \"worked\" : false, \"id\": \"KMan\" },"
	"      { \"worked\" : true , \"id\": \"Kimi\" },"
	"      { \"worked\" : false, \"id\": \"XMan\" },"
	"      { \"worked\" : true , \"id\": \"WMan\" }"
	"    ]"
	"  }"
	"}"
};

void  sample_js_parse (SampleJs* samplejs)
{
	UgJson  json;
	int     code;
	// JSON string for SampleJs
	const char*	json_string = sample_js_string;

	memset (&json, 0, sizeof (json));
	ug_json_init (&json);
	ug_json_begin_parse (&json);
#if 1
	// method 1: use UgEntry to parse start of object
	ug_json_push (&json, ug_json_parse_entry, samplejs, SampleJsObjectEntry);
#elif 1
	// method 2: push ug_json_parse_object() to parse start of object
	ug_json_push (&json, ug_json_parse_entry, samplejs, SampleJsEntry);
	ug_json_push (&json, ug_json_parse_object, NULL, NULL);
#else
	// debug
	ug_json_push (&json, json_debug_parser, NULL, NULL);
	ug_json_push (&json, ug_json_parse_object, NULL, NULL);
#endif
	code = ug_json_parse (&json, json_string, -1);
	printf ("ug_json_parse response %d\n", code);
	code = ug_json_end_parse (&json);
	printf ("ug_json_end_parse response %d\n", code);
	ug_json_final (&json);
}

void  sample_js_reparse (SampleJs* samplejs)
{
	UgJson   json;
	UgValue  value;

	ug_value_init (&value);

	memset (&json, 0, sizeof (json));
	ug_json_init (&json);
	ug_json_begin_parse (&json);
	ug_json_push (&json, ug_json_parse_value, &value, NULL);
	ug_json_parse (&json, sample_js_string, -1);
	ug_json_end_parse (&json);

	// convert UgValue to UgEntry
	ug_json_begin_parse (&json);
	ug_json_push (&json, ug_json_parse_entry, samplejs, SampleJsObjectEntry);
	ug_json_parse_by_value (&json, &value);
	ug_json_end_parse (&json);
	ug_json_final (&json);

	ug_value_clear (&value);
}

void  sample_js_print (SampleJs* samplejs)
{
	UgJson   json;
	UgBuffer buffer;

	printf ("\n"
			"obj.info.ftp.user: %s\n"
			"obj.info.ftp.password: %s\n"
			"obj.info.ftp.active_mode: %d\n"
			"obj.info.http.user_agent: %s\n"
			"obj.info.http.referrer: %s\n",
			samplejs->info.ftp.user,
			samplejs->info.ftp.password,
			samplejs->info.ftp.active_mode,
			samplejs->info.http.user_agent,
			samplejs->info.http.referrer
			);
	printf ("obj.info.sarray[0]: %s\n"
			"obj.info.sarray[1]: %s\n"
			"obj.info.sarray[2]: %s\n"
			"obj.info.sarray[3]: %s\n",
			samplejs->info.sarray.at[0],
			samplejs->info.sarray.at[1],
			samplejs->info.sarray.at[2],
			samplejs->info.sarray.at[3]);
	printf ("obj.info.iarray[0]: %d\n"
			"obj.info.iarray[1]: %d\n"
			"obj.info.iarray[2]: %d\n"
			"obj.info.iarray[3]: %d\n",
			samplejs->info.iarray.at[0],
			samplejs->info.iarray.at[1],
			samplejs->info.iarray.at[2],
			samplejs->info.iarray.at[3]);
	printf ("obj.info.oarray[0].worked: %d\n"
			"obj.info.oarray[0].id: %s\n"
			"obj.info.oarray[1].worked: %d\n"
			"obj.info.oarray[1].id: %s\n"
			"obj.info.oarray[2].worked: %d\n"
			"obj.info.oarray[2].id: %s\n"
			"obj.info.oarray[3].worked: %d\n"
			"obj.info.oarray[3].id: %s\n",
			samplejs->info.oarray.at[0].worked,
			samplejs->info.oarray.at[0].id,
			samplejs->info.oarray.at[1].worked,
			samplejs->info.oarray.at[1].id,
			samplejs->info.oarray.at[2].worked,
			samplejs->info.oarray.at[2].id,
			samplejs->info.oarray.at[3].worked,
			samplejs->info.oarray.at[3].id);

	ug_json_init (&json);
	ug_buffer_init (&buffer, 1024);

	ug_json_begin_write (&json, UG_JSON_FORMAT_INDENT, &buffer);
#if 1
	// method 1: use UgEntry to write start of object
	ug_json_write_entry (&json, samplejs, SampleJsObjectEntry);
#else
	// method 2: call ug_json_write_object_head() to write start of object
	ug_json_write_object_head (&json);
	ug_json_write_entry (&json, samplejs, SampleJsEntry);
	ug_json_write_object_tail (&json);
#endif
	ug_json_end_write (&json);

	ug_buffer_write_char (&buffer, '\0');
	puts ((char*)buffer.beg);

	ug_buffer_clear (&buffer, 1);
	ug_json_final (&json);
}

// UgBufferFunc
static int  buffer_to_file (UgBuffer* buffer)
{
	FILE*   file = buffer->data;

	printf ("write %d bytes to file\n", ug_buffer_length (buffer));
	fwrite (buffer->beg, 1, ug_buffer_length(buffer), file);
	buffer->cur = buffer->beg;
	return 0;
}

void  sample_js_to_file (SampleJs* samplejs, const char* filename)
{
	UgBuffer buffer;
	UgJson   json;
	FILE*    file;

	file = fopen (filename, "w");
	ug_buffer_init (&buffer, 128);
	buffer.more = buffer_to_file;
	buffer.data = file;

	ug_json_init (&json);
	ug_json_begin_write (&json, UG_JSON_FORMAT_INDENT, &buffer);
#if 1
	// method 1: use UgEntry to write start of object
	ug_json_write_entry (&json, samplejs, SampleJsObjectEntry);
#else
	// method 2: call ug_json_write_object_head() to write start of object
	ug_json_write_object_head (&json);
	ug_json_write_entry (&json, samplejs, SampleJsEntry);
	ug_json_write_object_tail (&json);
#endif
	ug_json_end_write (&json);
	ug_json_final (&json);

	ug_buffer_clear (&buffer, 1);
	fclose (file);
}

// ----------------------------------------------------------------------------
// JSON object sample

void  test_json_object_custom (void)
{
	UgBuffer    buffer;
	WorkedId    worked;
	int         code;
	UgJson      json;
	const char* json_string = {
		"{ \"worked\" : true , \"id\": \"C.H. Huang\" }"
	};

	puts ("\n--- test_json_object_custom:");

	worked_id_init (&worked);
	ug_json_init (&json);
	ug_buffer_init (&buffer, 128);

	// parse
	ug_json_begin_parse (&json);
#if 1
	// method 1: use UgEntry to parse start of object
	ug_json_push (&json, ug_json_parse_entry, &worked, WorkedIdCustomEntry);
#else
	// method 2: push ug_json_parse_object() to parse start of object
	ug_json_push (&json, ug_json_parse_entry, &worked, WorkedIdEntry);
	ug_json_push (&json, ug_json_parse_object, NULL, NULL);
#endif
	code = ug_json_parse (&json, json_string, -1);
	printf ("ug_json_parse response %d\n", code);
	code = ug_json_end_parse (&json);
	printf ("ug_json_end_parse response %d\n", code);

	// write
	ug_json_begin_write (&json, UG_JSON_FORMAT_INDENT, &buffer);
#if 1
	// method 1: use UgEntry to write start of object
	ug_json_write_entry (&json, &worked, WorkedIdCustomEntry);
#else
	// method 2: call ug_json_write_object_head() to write start of object
	ug_json_write_object_head (&json);
	ug_json_write_entry (&json, &worked, WorkedIdEntry);
	ug_json_write_object_tail (&json);
#endif
	ug_json_end_write (&json);

	ug_buffer_write_char (&buffer, '\0');
	puts (buffer.beg);

	ug_buffer_clear (&buffer, 1);
	ug_json_final (&json);
	worked_id_final (&worked);
}

// ----------------------------------------------------------------------------
// JSON array sample

// use UgArray to store JSON number array
void  test_json_int_array (void)
{
	// method 1
	const UgEntry  intArrayEntry[] = {
		{NULL,    0,  UG_ENTRY_ARRAY,
				ug_json_parse_array_int, ug_json_write_array_int},
		{NULL}
	};
	// common
	UgJson      json;
	UgArrayInt  array;
	UgJsonError code;
	char*   teststr = "[12,22,34,56]";
	int     index;

	puts ("\n--- test_json_int_array:");

	memset (&json, 0, sizeof (json));
	ug_array_init (&array, sizeof (int), 0);

	// JSON begin
	ug_json_init (&json);
	// JSON parse
	ug_json_begin_parse (&json);
#if 1
	// method 1: use UgEntry to parse start of array
	ug_json_push (&json, ug_json_parse_entry, &array, (void*)intArrayEntry);
#else
	// method 2: push ug_json_parse_array() to parse start of array
	ug_json_push (&json, ug_json_parse_array_int, &array, NULL);
	ug_json_push (&json, ug_json_parse_array, NULL, NULL);
#endif
	code = ug_json_parse (&json, teststr, -1);
	printf ("ug_json_parse response %d\n", code);
	code = ug_json_end_parse (&json);
	printf ("ug_json_end_parse response %d\n", code);

	// JSON end
	ug_json_final (&json);

	printf ("array.length = '%d'\n"
			"array.allocated = '%d'\n\n",
			array.length, array.allocated);

	for (index = 0;  index < array.length;  index++)
		printf ("array.at[%d] = %d\n", index, array.at[index]);
	ug_array_clear (&array);
}

// use UgList to store JSON string and number array
void  test_json_array_by_list (void)
{
	// method 1
	const UgEntry  stringListEntry[] = {
		{NULL,   0, UG_ENTRY_ARRAY,
				ug_json_parse_list_string, ug_json_write_list_string},
		{NULL}
	};
	const UgEntry  intListEntry[] = {
		{NULL, 0, UG_ENTRY_ARRAY,
				ug_json_parse_list_int, ug_json_write_list_int},
		{NULL}
	};
	// common
	UgJson		json;
	UgList		list;
	UgLink*		link;
	UgJsonError	code;
	UgBuffer    buffer;
	char*	jarraystr = "[\"Str1\",\"Str2\",\"Str3\",\"Str4\"]";
	char*	jarrayint = "[21,32,43,54]";

	puts ("\n--- test_json_array_by_list:");

	memset (&json, 0, sizeof (json));
	ug_list_init (&list);

	// JSON begin
	ug_json_init (&json);
	ug_buffer_init (&buffer, 4096);

	puts ("\n  parse JSON string array:");
	// JSON parse - string array
	ug_json_begin_parse (&json);
#if 1
	// method 1: use UgEntry to parse start of array
	ug_json_push (&json, ug_json_parse_entry, &list, (void*)stringListEntry);
#else
	// method 2: push ug_json_parse_array() to parse start of array
	ug_json_push (&json, ug_json_parse_list_string, &list, NULL);
	ug_json_push (&json, ug_json_parse_array, NULL, NULL);
#endif
	code = ug_json_parse (&json, jarraystr, -1);
	printf ("ug_json_parse response %d\n", code);
	code = ug_json_end_parse (&json);
	printf ("ug_json_end_parse response %d\n", code);

	printf ("list.size = '%d'\n", (int)list.size);
	for (link = list.head;  link;  link = link->next)
		printf ("link->data = \"%s\"\n", (char*)link->data);
	// JSON write - string array
	ug_json_begin_write (&json, 0, &buffer);
#if 1
	// method 1: use UgEntry to write start of array
	ug_json_write_entry (&json, &list, (void*)stringListEntry);
#else
	// method 2: call ug_json_write_array_head() to write start of array
	ug_json_write_array_head (&json);
	ug_json_write_list_string (&json, &list);
	ug_json_write_array_tail (&json);
#endif
	ug_json_end_write (&json);

	puts ("ug_json_write_list_string:");
	ug_buffer_write_char (&buffer, '\0');
	puts (buffer.beg);
	// free strings in list
	ug_list_foreach (&list, (UgForeachFunc) ug_free, NULL);
	ug_list_clear (&list, TRUE);

	puts ("\n  parse JSON int array:");
	// JSON parse - int array
	ug_json_begin_parse (&json);
#if 1
	// method 1: use UgEntry to parse start of array
	ug_json_push (&json, ug_json_parse_entry, &list, (void*)intListEntry);
#else
	// method 2: push ug_json_parse_array() to parse start of array
	ug_json_push (&json, ug_json_parse_list_int, &list, NULL);
	ug_json_push (&json, ug_json_parse_array, NULL, NULL);
#endif
	code = ug_json_parse (&json, jarrayint, -1);
	printf ("ug_json_parse response %d\n", code);
	code = ug_json_end_parse (&json);
	printf ("ug_json_end_parse response %d\n", code);

	printf ("list.size = '%d'\n", (int)list.size);
	for (link = list.head;  link;  link = link->next)
		printf ("link->data = %d\n", (int)(intptr_t)link->data);
	// JSON write - int array
	buffer.cur = buffer.beg;
	ug_json_begin_write (&json, 0, &buffer);
#if 1
	// method 1: use UgEntry to write start of array
	ug_json_write_entry (&json, &list, (void*)intListEntry);
#else
	// method 2: call ug_json_write_array_head() to write start of array
	ug_json_write_array_head (&json);
	ug_json_write_list_int (&json, &list);
	ug_json_write_array_tail (&json);
#endif
	ug_json_end_write (&json);

	puts ("ug_json_write_list_int:");
	ug_buffer_write_char (&buffer, '\0');
	puts (buffer.beg);
	// free list
	ug_list_clear (&list, TRUE);

	ug_buffer_clear (&buffer, 1);
	// JSON end
	ug_json_final (&json);
}

// ----------------------------------------------------------------------------
// JSON value sample

void  test_json_value (void)
{
	UgJson  json;
	int     code;
#if 0
	const char* json_string = "1234";
#elif 0
	const char* json_string = "-100";
#elif 0
	const char* json_string = "\"This is string.\"";
#elif 0
	const char* json_string = "\"This is";  // uncompleted string
#elif 0
	const char* json_string = "true";
#elif 0
	const char* json_string = "tr";         // uncompleted true
#elif 0
	const char* json_string = "{";          // uncompleted object
#else
	// "Sam a tűzoltó - Világcsúcs kísérletek"
	const char* json_string = "\"Sam a t\\u0171zolt\\u00f3 - Vil\\u00e1gcs\\u00facs k\\u00eds\\u00e9rletek\"";
#endif

	puts ("\n--- test_json_value:");

	ug_json_init (&json);
	ug_json_begin_parse (&json);
#if 1
	// debug
	ug_json_push (&json, json_debug_parser, NULL, NULL);
#endif
	code = ug_json_parse (&json, json_string, -1);
	printf ("ug_json_parse response %d\n", code);
	code = ug_json_end_parse (&json);
	printf ("ug_json_end_parse response %d\n", code);
	ug_json_final (&json);
}

// ----------------------------------------------------------------------------
// test JSON writer

void  test_json_writer (void)
{
	UgJson   json;
	UgBuffer buffer;

	puts ("\n--- test_json_writer:");

	ug_json_init (&json);
	ug_buffer_init (&buffer, 4096);

	ug_json_begin_write (&json, UG_JSON_FORMAT_INDENT, &buffer);

	// --- object1 begin
	ug_json_write_object_head (&json);

	// --- array1 begin
	ug_json_write_array_head (&json);
	ug_json_write_array_tail (&json);
	// --- array1 end

	// --- object2 begin
	ug_json_write_object_head (&json);
	ug_json_write_string (&json, "name1");
	// --- array2 begin
	ug_json_write_array_head (&json);
	ug_json_write_int (&json, 1205);
	ug_json_write_int (&json, 2011);
	ug_json_write_int (&json, 1102);
	ug_json_write_array_tail (&json);
	// --- array2 end
	ug_json_write_string (&json, "name2");
//	ug_json_write_string (&json, "value2");
	ug_json_write_string (&json, "Sam a tűzoltó - Világcsúcs kísérletek");
	ug_json_write_object_tail (&json);
	// --- object2 end

	ug_json_write_object_tail (&json);
	// --- object1 end

	ug_json_end_write (&json);

	ug_buffer_write_char (&buffer, '\0');
	puts (buffer.beg);

	ug_buffer_clear (&buffer, 1);
	ug_json_final (&json);
}

// ----------------------------------------------------------------------------
// test UgArray

void  test_array (void)
{
	UgArrayChar array;
	char*   str = "This one is test string.";

	puts ("\n--- test_array:");

	ug_array_init (&array, sizeof (char), 0);
	ug_array_append (&array, str, strlen (str));
	ug_array_end0 (&array);
	printf ("\n"
			"array.at = '%s'\n"
			"array.length = '%d'\n"
			"array.allocated = '%d'\n",
			array.at, array.length, array.allocated);

	ug_array_clear (&array);
}

// ----------------------------------------------------------------------------
// test main function

int   main (void)
{
	SampleJs*   samplejs;
//	g_mem_set_vtable (glib_mem_profiler_table);

	// test UgArray
	test_array ();
	// test JSON writer
	test_json_writer ();
	// test simple JSON value
	test_json_value ();
	// use Custom type (WorkedId) to store JSON object
	test_json_object_custom ();
	// use UgArray(int) to store JSON number array
	test_json_int_array ();
	// use UgList to store JSON string and number array
	test_json_array_by_list ();

	puts ("\n--- SampleJs functions:");
	// C struct sample code: parse, print, and save file.
	samplejs = sample_js_new ();
	sample_js_parse (samplejs);
	sample_js_print (samplejs);
	sample_js_to_file (samplejs, "test-SampleJs.json");
	sample_js_free (samplejs);

	samplejs = sample_js_new ();
	sample_js_reparse (samplejs);
	sample_js_print (samplejs);
	sample_js_free (samplejs);

//	g_mem_profile ();

	return 0;
}

