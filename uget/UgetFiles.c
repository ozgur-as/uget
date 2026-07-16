/*
 *
 *   Copyright (C) 2018-2020 by C.H. Huang
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_GLIB
#include <glib.h>    // g_malloc/g_free
#endif // HAVE_GLIB

#include <stdlib.h>
#include <UgString.h>
#include <UgJson.h>
#include <UgetFiles.h>
#include <UgJson-custom.h>

// ----------------------------------------------------------------------------
// UgetFile

UgetFile*  uget_file_new(void)
{
#ifdef HAVE_GLIB
	return g_malloc0(sizeof(UgetFile));
#else
	return ug_malloc0(sizeof(UgetFile));
#endif // HAVE_GLIB
}

void  uget_file_free(UgetFile* file)
{
#ifdef HAVE_GLIB
	g_free(file);
#else
	ug_free(file);
#endif
}

// ----------------------------------------------------------------------------
// UgetFiles

static void uget_files_init(UgetFiles* files);
static void uget_files_final(UgetFiles* files);
static void uget_files_copy(UgetFiles* files, UgetFiles* src);

static void        ug_json_write_list(UgJson* json, void* collection);
static UgJsonError ug_json_parse_list(UgJson* json,
                                      const char* name, const char* value,
                                      void* list,       void* none);

static const UgEntry  UgetFilesEntry[] =
{
	{"list",         offsetof(UgetFiles, list),   UG_ENTRY_ARRAY,
			ug_json_parse_list, ug_json_write_list},

	// deprecated
	{"collection",   offsetof(UgetFiles, list),   UG_ENTRY_ARRAY,
			ug_json_parse_list, NULL},
	{NULL}    // null-terminated
};

static const UgDataInfo  UgetFilesInfoStatic =
{
	.name   = "files",
	.size   = sizeof(UgetFiles),
	.init   = (UgInitFunc)   uget_files_init,
	.final  = (UgFinalFunc)  uget_files_final,
	.assign = (UgAssignFunc) uget_files_assign,
	.entry  = UgetFilesEntry,
};
// extern
const UgDataInfo*  UgetFilesInfo = &UgetFilesInfoStatic;

static void uget_files_init(UgetFiles* files)
{
	ug_list_init(&files->list);
	files->sync_count = 0;
}

static void uget_files_final(UgetFiles* files)
{
	// free UgetFile.path in list
	ug_list_foreach(&files->list, (UgForeachFunc)ug_free, NULL);
	ug_list_clear(&files->list, TRUE);
}

int  uget_files_assign(UgetFiles* files, UgetFiles* src)
{
	// free UgetFile.path in list
	ug_list_foreach(&files->list, (UgForeachFunc)ug_free, NULL);
	ug_list_clear(&files->list, TRUE);

	uget_files_copy(files, src);
	files->sync_count = src->sync_count;
	return TRUE;
}

void  uget_files_clear(UgetFiles* files)
{
	ug_list_foreach(&files->list, (UgForeachFunc)ug_free, NULL);
	ug_list_clear(&files->list, TRUE);
}

// sync UgetFile from 'src' to 'files.
// 1. all UgetFile in 'src' will insert/replace into 'files'.
// 2. remove deleted (state == UGET_FILE_STATE_DELETED) UgetFile in 'src'.
// return TRUE if 'files' have added or removed UgetFile.
int  uget_files_sync(UgetFiles* files, UgetFiles* src)
{
	UgetFile* sibling;
	UgetFile* file1;
	UgetFile* file1_src;
	UgetFile* src_next;

	if (files->sync_count == src->sync_count)
		return FALSE;

	// sync UgetFile from 'src'
	for (file1_src = (UgetFile*)src->list.head;  file1_src;  file1_src = src_next) {
		src_next = file1_src->next;
		file1 = uget_files_find(files, file1_src->path, &sibling);
		// add new UgetFile in files
		if (file1 == NULL) {
			file1 = uget_file_new();
			ug_list_insert(&files->list, (UgLink*)sibling, (UgLink*)file1);
			if (file1_src->path)
				file1->path = ug_strdup(file1_src->path);
			else
				file1->path = NULL;
		}
		file1->type  = file1_src->type;
		file1->state = file1_src->state;
//		file1->order = file1_src->order;
		file1->total = file1_src->total;
		file1->complete = file1_src->complete;

		// remove deleted UgetFile in 'src'
		if (file1_src->type & UGET_FILE_STATE_DELETED) {
			// delete file from src
			ug_free(file1_src->path);
			ug_list_remove(&src->list, (UgLink*)file1_src);
			uget_file_free(file1_src);
		}
	}
	files->sync_count = src->sync_count;
	return TRUE;
}

UgetFile* uget_files_find(UgetFiles* files, const char* path, UgetFile** sibling)
{
	UgetFile* file1;
	int       diff;

	for (file1 = (UgetFile*)files->list.head;  file1;  file1 = file1->next) {
		diff = strcmp(file1->path, path);
		if (diff > 0) {
			if (sibling)
				sibling[0] = file1;
			return NULL;
		}
		if (diff == 0)
			break;
	}

	if (sibling)
		sibling[0] = file1;
	return file1;
}

UgetFile* uget_files_realloc(UgetFiles* files, const char* path)
{
	UgetFile* file1;
	UgetFile* sibling;

	file1 = uget_files_find(files, path, &sibling);
    if (file1 == NULL) {
		file1 = uget_file_new();
		ug_list_insert(&files->list, (UgLink*)sibling, (UgLink*)file1);
		file1->path  = ug_strdup(path);
		file1->type  = 0;
		file1->state = 0;
//		file1->order = 0;
		file1->total = 0;
		file1->complete = 0;
		files->sync_count++;
    }
	return file1;
}

UgetFile* uget_files_replace(UgetFiles* files, const char* path,
                             int type, int state)
{
	UgetFile* file1;

	file1 = uget_files_realloc(files, path);
	file1->type  = type;
	file1->state = state;
	files->sync_count++;
	return file1;
}

void uget_files_apply(UgetFiles* files, int type, int state)
{
	UgetFile* file1;

	for (file1 = (UgetFile*)files->list.head;  file1;  file1 = file1->next) {
        if (file1->type == type || type == UGET_FILE_ALL)
			file1->state |= state;
	}
	files->sync_count++;
}

void uget_files_erase(UgetFiles* files, int type, int state)
{
	UgetFile* file1;
	UgetFile* next;

	for (file1 = (UgetFile*)files->list.head;  file1;  file1 = next) {
		next = file1->next;
        if (file1->type != type && type != UGET_FILE_ALL)
			continue;
		if (file1->state & state) {
			// delete file from src
			ug_free(file1->path);
			ug_list_remove(&files->list, (UgLink*)file1);
			uget_file_free(file1);
		}
	}
	files->sync_count -= 10;
}

// copy UgetFile from 'src' to 'files'.
static void uget_files_copy(UgetFiles* files, UgetFiles* src)
{
	UgetFile* file1;
	UgetFile* file1_src;

	for(file1_src = (UgetFile*)src->list.head;  file1_src;  file1_src = file1_src->next) {
		file1 = uget_file_new();
		ug_list_append(&files->list, (UgLink*)file1);

		if (file1_src->path)
			file1->path = ug_strdup(file1_src->path);
		else
			file1->path = NULL;
		file1->type  = file1_src->type;
		file1->state = file1_src->state;
//		file1->order = file1_src->order;
		file1->total = file1_src->total;
		file1->complete = file1_src->complete;
	}
    files->sync_count++;
}

// ----------------------------------------------------------------------------
// JSON

static const UgEntry  UgetFileEntry[] =
{	{"path",     offsetof(UgetFile, path),     UG_ENTRY_STRING,
			NULL, NULL},
	{"type",     offsetof(UgetFile, type),     UG_ENTRY_CUSTOM,
			ug_json_parse_int16, ug_json_write_int16},
	{"state",    offsetof(UgetFile, state),    UG_ENTRY_CUSTOM,
			ug_json_parse_int16, ug_json_write_int16},
//	{"order",    offsetof(UgetFile, order),    UG_ENTRY_CUSTOM,
//			ug_json_parse_int32, ug_json_write_int32},
	{"total",    offsetof(UgetFile, total),    UG_ENTRY_INT64,
			NULL, NULL},
	{"complete", offsetof(UgetFile, complete), UG_ENTRY_INT64,
			NULL, NULL},
	{NULL}    // null-terminated
};

static void ug_json_write_list(UgJson* json, void* list)
{
	UgList*   filelist = list;
	UgetFile* file1;

	for (file1 = (UgetFile*)filelist->head;  file1;  file1 = file1->next) {
		ug_json_write_object_head(json);
		ug_json_write_entry(json, file1, UgetFileEntry);
		ug_json_write_object_tail(json);
	}
}

static UgJsonError ug_json_parse_list(UgJson* json,
                                      const char* name, const char* value,
                                      void* list,       void* none)
{
	UgList*   filelist = list;
	UgetFile* file1;

	if (json->type != UG_JSON_OBJECT) {
//		if (json->type >= UG_JSON_OBJECT)
//			ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	}

	file1 = uget_file_new();
	ug_list_append(filelist, (UgLink*)file1);
	ug_json_push(json, ug_json_parse_entry,
	             file1, (void*)UgetFileEntry);
	return UG_JSON_ERROR_NONE;
}
