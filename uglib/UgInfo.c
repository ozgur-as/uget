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

#include <stdlib.h>
#include <string.h>
#include <UgInfo.h>

// ----------------------------------------------------------------------------
// UgRegistry for UgInfo

static UgRegistry*  ug_info_registry;

UgRegistry*  ug_info_get_registry(void)
{
	return ug_info_registry;
}

void  ug_info_set_registry(UgRegistry* registry)
{
	ug_info_registry = registry;
}

// ----------------------------------------------------------------------------
// UgInfo

UgInfo* ug_info_new(int allocated_length, int cache_length)
{
	UgInfo*  info;

#ifdef HAVE_GLIB
	info = g_malloc(sizeof(UgInfo));
#else
	info = ug_malloc(sizeof(UgInfo));
#endif // HAVE_GLIB
	ug_info_init(info, allocated_length, cache_length);
	return info;
}

void    ug_info_ref(UgInfo* info)
{
	info->ref_count++;
}

void    ug_info_unref(UgInfo* info)
{
	if (--info->ref_count == 0) {
		ug_info_final(info);
#ifdef HAVE_GLIB
		g_free(info);
#else
		ug_free(info);
#endif // HAVE_GLIB
	}
}

void  ug_info_init(UgInfo* info, int allocated_length, int cache_length)
{
	int     index;

	ug_array_init(info, sizeof(UgPair), allocated_length + cache_length);
	info->length       = cache_length;
	info->cache_length = cache_length;
	info->ref_count    = 1;

	// clear cache
	for (index = 0;  index < info->length;  index++) {
		info->at[index].key  = NULL;
		info->at[index].data = NULL;
	}
}

void  ug_info_final(UgInfo* info)
{
	UgPair* cur;
	UgPair* end;

	for (cur = info->at, end = cur + info->length;  cur < end;  cur++) {
		if (cur->key == NULL)
			continue;
		if (cur->data)
			ug_data_free(cur->data);
	}

	ug_array_clear(info);
}

UgPair* ug_info_find(UgInfo* info, const UgDataInfo* key, int* index)
{
	UgPair*   end;
	UgPair*   cur;

	// find key in cache space
	for (cur = info->at, end = cur + info->cache_length;  cur < end;  cur++) {
		if (cur->key == key)
			return cur;
	}

	// find key without cache space
	info->at     += info->cache_length;
	info->length -= info->cache_length;
	cur = ug_array_find_sorted(info, &key, ug_array_compare_pointer, index);
	info->at     -= info->cache_length;
	info->length += info->cache_length;
	if (index)
		index[0] += info->cache_length;
	return cur;
}

void*  ug_info_realloc(UgInfo* info, const UgDataInfo* key)
{
	UgPair* cur;
	int     index;

	cur = ug_info_find(info, key, &index);
	if (cur == NULL) {
		cur = ug_array_insert(info, index, 1);
		cur->key = (void*) key;
		cur->data = ug_data_new(key);
	}
	else if (cur->data == NULL)
		cur->data = ug_data_new(key);
	return cur->data;
}

void  ug_info_remove(UgInfo* info, const UgDataInfo* key)
{
	UgPair* cur;

	cur = ug_info_find(info, key, NULL);
	if (cur && cur->data) {
		ug_data_free(cur->data);
		cur->data = NULL;
	}
}

void* ug_info_set(UgInfo* info, const UgDataInfo* key, void* data)
{
	UgPair* cur;
	int     index;
	void*   result;

	cur = ug_info_find(info, key, &index);
	if (cur == NULL) {
		cur = ug_array_insert(info, index, 1);
		cur->key = (void*) key;
		cur->data = NULL;
	}
	result = cur->data;
	cur->data = data;
	return result;
}

void* ug_info_get(UgInfo* info, const UgDataInfo* key)
{
	UgPair* cur;

	cur = ug_info_find(info, key, NULL);
	if (cur == NULL)
		return NULL;
	return cur->data;
}

void  ug_info_assign(UgInfo* info, UgInfo* src, const UgDataInfo* exclude_info)
{
	int      index;
	UgPair*  pair;
	UgData*  data;

	for (index = 0;  index < src->length;  index++) {
		pair = src->at + index;
		if (pair->key == NULL || pair->data == NULL)
			continue;
		if (pair->key == exclude_info)
			continue;
		data = ug_info_realloc(info, pair->key);
		ug_data_assign(data, pair->data);
	}
}

// UgJsonParseFunc for key/data pairs in UgInfo
static UgJsonError ug_json_parse_info_reg(UgJson* json,
                                const char* name, const char* value,
                                void* info, void* infoRegistry)
{
	UgRegistry* registry;
	UgPair*     cur;

	if (infoRegistry)
		registry = infoRegistry;
	else if (ug_info_registry)
		registry = ug_info_registry;
	else
		registry = NULL;

	if (registry) {
		if (registry->sorted == FALSE)
			ug_registry_sort(registry);
		cur = ug_registry_find(registry, name, NULL);

		if (cur) {
			ug_json_push(json, ug_json_parse_entry,
					ug_info_realloc(info, cur->data),
					(void*)((UgDataInfo*)cur->data)->entry);
			return UG_JSON_ERROR_NONE;
		}
	}

	if (json->type >= UG_JSON_OBJECT)
		ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
	return UG_JSON_ERROR_CUSTOM;
}

// ----------------
// JSON parser/writer that used with UG_ENTRY_CUSTOM.

// JSON parser for UgInfo pointer.
UgJsonError ug_json_parse_info_ptr(UgJson* json,
                               const char* name, const char* value,
                               void** pinfo, void* none)
{
	UgInfo* info = *pinfo;

	// UgInfo's type is UG_JSON_OBJECT
	if (json->type != UG_JSON_OBJECT) {
//		if (json->type == UG_JSON_ARRAY)
//			ug_json_push(json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	}

	// confirm that target UgInfo is empty. This can avoid parsing UgInfo repeatedly.
	if (info->length == info->cache_length)
		ug_json_push(json, ug_json_parse_info_reg, info, NULL);
	return UG_JSON_ERROR_NONE;
}

// JSON writer for UgInfo pointer.
void  ug_json_write_info_ptr(UgJson* json, UgInfo** pinfo)
{
	UgInfo* info = *pinfo;
	UgPair* cur;
	UgPair* end;

	ug_json_write_object_head(json);
	for (cur = info->at, end = cur + info->length;  cur < end;  cur++) {
		if (cur->data == NULL || ((UgDataInfo*)cur->key)->entry == NULL)
			continue;

		ug_json_write_string(json, ((UgDataInfo*)cur->key)->name);
		ug_json_write_object_head(json);
		ug_json_write_entry(json, cur->data,
				((UgDataInfo*)cur->key)->entry);
		ug_json_write_object_tail(json);
	}
	ug_json_write_object_tail(json);
}

// JSON parser for UgInfo.
UgJsonError ug_json_parse_info(UgJson* json,
                               const char* name, const char* value,
                               void* info, void* none)
{
	return ug_json_parse_info_ptr(json, name, value, &info, none);
}

// JSON writer for UgInfo.
void  ug_json_write_info(UgJson* json, UgInfo* info)
{
	ug_json_write_info_ptr(json, &info);
}

