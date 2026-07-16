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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <UgString.h>
#include <UgetNode.h>
#include <UgetData.h>

#ifdef HAVE_GLIB
#include <glib.h>    // g_malloc/g_free
#endif

static void  uget_node_call_fake_filter (UgetNode* parent, UgetNode* sibling, UgetNode* child);
static UgJsonError  ug_json_parse_state2group (UgJson* json,
                                const char* name, const char* value,
                                void* node, void* none);
static UgJsonError  ug_json_parse_name2data (UgJson* json,
                                const char* name, const char* value,
                                void* node, void* none);
// ----------------------------------------------------------------------------
// UgetNode

struct UgetNodeNotifier  uget_node_default_notifier =
{
	.inserted = NULL,
	.removed  = NULL,
	.updated  = NULL,
	.data     = NULL,   // extra data for user
};

struct UgetNodeControl   uget_node_default_control =
{
	.notifier = &uget_node_default_notifier,
	.sort     = { .compare = NULL, .reverse = FALSE },
	.filter   = NULL,
};

const UgEntry  UgetNodeEntry[] =
{
	{"info",     offsetof (UgetNode, info),  UG_ENTRY_CUSTOM,
			ug_json_parse_info_ptr,   ug_json_write_info_ptr},
	{"children", 0,                          UG_ENTRY_ARRAY,
			ug_json_parse_uget_node_children, ug_json_write_uget_node_children},

	// deprecated
	{"name",     0,                          UG_ENTRY_CUSTOM,
			ug_json_parse_name2data,  NULL},
	{"state",    0,                          UG_ENTRY_CUSTOM,
			ug_json_parse_state2group, NULL},
	{"data",     offsetof (UgetNode, info),  UG_ENTRY_CUSTOM,
			ug_json_parse_info_ptr,   NULL},
	{NULL}    // null-terminated
};

UgetNode*  uget_node_new (UgetNode* node_real)
{
	UgetNode*  node;

#ifdef HAVE_GLIB
	node = g_malloc (sizeof (UgetNode));
#else
	node = ug_malloc (sizeof (UgetNode));
#endif // HAVE_GLIB
	uget_node_init (node, node_real);
	return node;
}

void  uget_node_free(UgetNode* node)
{
	uget_node_final(node);
#ifdef HAVE_GLIB
	g_free(node);
#else
	ug_free(node);
#endif // HAVE_GLIB
}

void  uget_node_init  (UgetNode* node, UgetNode* node_real)
{
	memset (node, 0, sizeof (UgetNode));

	node->control = &uget_node_default_control;

	if (node_real == NULL) {
		node->base = node;    // pointer to self
		node->info = ug_info_new(8, 2);
		node->info->at[0].key = (void*) UgetRelationInfo;
		node->info->at[1].key = (void*) UgetProgressInfo;
	}
	else {
		// this is a fake node.
//		node->group = 0;
		node->base = node_real->base;
		node->real = node_real;
		node->peer = node_real->fake;
		node_real->fake = node;
		node->info = node_real->info;
		ug_info_ref(node->info);
	}
}

void  uget_node_final(UgetNode* node)
{
	if (node->parent)
		uget_node_remove(node->parent, node);
	if (node->real)
		uget_node_remove_fake(node->real, node);

	uget_node_clear_fake(node);
	uget_node_clear_children(node);
//	ug_node_unlink((UgNode*)node);
	ug_info_unref(node->info);
}

void  uget_node_clear_children (UgetNode* node)
{
	UgetNode*  next;
	UgetNode*  children;

	for (children = node->children;  children;  children = next) {
		next = children->next;
		uget_node_free(children);
	}
	node->children = NULL;
}

void  uget_node_clear_fake (UgetNode* node)
{
	UgetNode*  peer;
	UgetNode*  fake;

	for (fake = node->fake;  fake;  fake = peer) {
		peer = fake->peer;
		fake->real = NULL;   // speed up uget_node_free()
		uget_node_free(fake);
	}
	node->fake = NULL;       // speed up uget_node_free()
}

void  uget_node_move (UgetNode* node, UgetNode* sibling, UgetNode* child)
{
	UgetNode*  fake_sibling;
	UgetNode*  fake_child;

	ug_node_remove ((UgNode*) node, (UgNode*) child);
	ug_node_insert ((UgNode*) node, (UgNode*) sibling, (UgNode*) child);

	fake_sibling = NULL;
	for (fake_child = child->fake;  fake_child;  fake_child = fake_child->peer) {
		node = fake_child->parent;
		if (node == NULL)
			continue;

		if (sibling) {
			for (fake_sibling = sibling->fake;  fake_sibling;  fake_sibling = fake_sibling->peer) {
				if (fake_sibling->parent == node)
					break;
			}
			if (fake_sibling == NULL)
				continue;
		}

		uget_node_move (node, fake_sibling, fake_child);
	}
}

void  uget_node_insert (UgetNode* node, UgetNode* sibling, UgetNode* child)
{
	UgetNodeFunc inserted;

	ug_node_insert ((UgNode*) node, (UgNode*) sibling, (UgNode*) child);
	child->control = node->control;
//	child->control = node->control->children;

	inserted = node->control->notifier->inserted;
	if (inserted)
		inserted (node, sibling, child);

	uget_node_call_fake_filter (node, sibling, child);
}

static void  uget_node_unlink_children_real (UgetNode* node)
{
	for (node = node->children;  node;  node = node->next) {
		uget_node_unlink_children_real (node);
		if (node->real)
			uget_node_remove_fake (node->real, node);
	}
}

static void  uget_node_unlink_fake_parent (UgetNode* child)
{
	UgetNode*    parent;
	UgetNode*    sibling;
	UgetNodeFunc removed;

	for (child = child->fake;  child;  child = child->peer) {
		uget_node_unlink_fake_parent (child);
		parent = child->parent;
		if (parent == NULL)
			continue;
		sibling = child->next;
		ug_node_remove ((UgNode*) parent, (UgNode*) child);
		// notify
		removed = parent->control->notifier->removed;
		if (removed)
			removed (parent, sibling, child);
	}
}

void  uget_node_remove (UgetNode* node, UgetNode* child)
{
	UgetNode*    sibling;
	UgetNodeFunc removed;

	sibling = child->next;
	ug_node_remove ((UgNode*)node, (UgNode*)child);
	uget_node_unlink_fake_parent (child);
	uget_node_unlink_children_real (child);

	removed = node->control->notifier->removed;
	if (removed)
		removed (node, sibling, child);
}

void  uget_node_append (UgetNode* node, UgetNode* child)
{
	UgetNodeFunc inserted;

	ug_node_append ((UgNode*) node, (UgNode*) child);
	child->control = node->control;
//	child->control = node->control->children;

	inserted = node->control->notifier->inserted;
	if (inserted)
		inserted (node, NULL, child);

	uget_node_call_fake_filter (node, NULL, child);
}

void  uget_node_prepend (UgetNode* node, UgetNode* child)
{
	UgetNode*    sibling;
	UgetNodeFunc inserted;

	sibling = node->children;
	ug_node_prepend ((UgNode*) node, (UgNode*) child);
	child->control = node->control;
//	child->control = node->control->children;

	inserted = node->control->notifier->inserted;
	if (inserted)
		inserted (node, sibling, child);

	uget_node_call_fake_filter (node, sibling, child);
}

// used by uget_node_sort()
static void uget_node_qsort(UgetNode** array, int left, int right, UgCompareFunc compare)
{
	UgetNode* temp;
	UgetNode* pivot = array[(left + right) / 2];
	int i = left, j = right;

	/* partition */
	while (i <= j) {
		while (compare(array[i], pivot) < 0)
			i++;
		while (compare(array[j], pivot) > 0)
			j--;
		if (i <= j) {
			temp = array[i];
			array[i] = array[j];
			array[j] = temp;
			i++;
			j--;
		}
	};

	/* recursion */
	if (left < j)
		uget_node_qsort(array, left, j, compare);
	if (i < right)
		uget_node_qsort(array, i, right, compare);
}

void  uget_node_sort(UgetNode* node, UgCompareFunc compare, int reversed)
{
	int        index;
	UgetNode** array;
	UgetNode*  cur;

	if (node->n_children == 0)
		return;
	array = (UgetNode**) ug_malloc(sizeof(UgetNode*) * node->n_children);
	for (index = 0, cur = node->children;  cur;  cur = cur->next, index++)
		array[index] = cur;

	uget_node_qsort(array, 0, node->n_children -1, compare);

	if (reversed) {
		for (index = node->n_children-1;  index >=0;  index--) {
			uget_node_remove(node, array[index]);
			uget_node_append(node, array[index]);
		}
	}
	else {
		for (index = 0;  index < node->n_children;  index++) {
			uget_node_remove(node, array[index]);
			uget_node_append(node, array[index]);
		}
	}
	ug_free(array);
}

/*
void  uget_node_sort (UgetNode* node, UgCompareFunc compare, int reversed)
{
	UgetNode* beg;
	UgetNode* cur;
	UgetNode* unsorted;

	for (beg = node->children;  beg;  beg = unsorted->next) {
		unsorted = beg;
		if (reversed == FALSE) {
			for (cur = beg->next;  cur;  cur = cur->next) {
				if (compare (cur, unsorted) < 0)
					unsorted = cur;
			}
		}
		else {
			for (cur = beg->next;  cur;  cur = cur->next) {
				if (compare (unsorted, cur) < 0)
					unsorted = cur;
			}
		}

		if (unsorted != beg)
			uget_node_move (node, beg, unsorted);
	}
}
 */

void  uget_node_insert_sorted (UgetNode* node, UgetNode* child)
{
	UgCompareFunc  compare;
	UgetNode*      cur;
	int            reverse;

	compare = node->control->sort.compare;
	reverse = node->control->sort.reverse;
	if (compare == NULL)
		return;

	if (reverse == FALSE) {
		for (cur = node->children;  cur;  cur = cur->next) {
			if (compare (cur, child) > 0) {
				uget_node_insert (node, cur, child);
				return;
			}
		}
	}
	else {
		for (cur = node->children;  cur;  cur = cur->next) {
			if (compare (child, cur) > 0) {
				uget_node_insert (node, cur, child);
				return;
			}
		}
	}
	uget_node_insert (node, NULL, child);
}

void  uget_node_reorder_by_real (UgetNode* node, UgetNode* real)
{
	UgetNode*  sibling;
	UgetNode*  fake;

	sibling = node->children;
	if (real == NULL)
		real = node->real;
	if (real == NULL)
		return;

	for (real = real->children;  real;  real = real->next) {
		for (fake = real->fake;  fake;  fake = fake->peer) {
			if (fake->parent != node)
				continue;
			if (fake == sibling) {
				sibling = sibling->next;
				break;
			}
			ug_node_remove ((UgNode*) node, (UgNode*) fake);
			ug_node_insert ((UgNode*) node, (UgNode*) sibling, (UgNode*) fake);
			break;
		}
	}
}

void  uget_node_reorder_by_fake (UgetNode* node, UgetNode* fake)
{
	UgetNode*  sibling;
	UgetNode*  real;

	if (fake == NULL || fake->children == NULL)
		return;
	sibling = fake->children->real;

	for (fake = fake->children;  fake;  fake = fake->next) {
		real = fake->real;
		if (real == NULL || real->parent != node)
			continue;
		if (real == sibling)
			sibling = sibling->next;
		ug_node_remove ((UgNode*) node, (UgNode*) real);
		ug_node_insert ((UgNode*) node, (UgNode*) sibling, (UgNode*) real);
	}
}

void  uget_node_remove_fake (UgetNode* node, UgetNode* fake)
{
	UgetNode*    curr;
	UgetNode*    prev;

	for (prev = NULL, curr = node->fake;  curr;  curr = curr->peer) {
		if (curr == fake) {
			if (prev)
				prev->peer = curr->peer;
			else
				node->fake = curr->peer;
			curr->real = NULL;
			curr->peer = NULL;
			return;
		}
		prev = curr;
	}
}

void  uget_node_make_fake (UgetNode* node)
{
	UgetNode*  child;
//	UgetNode*  sibling;

	if (node->fake == NULL)
		return;
	for (child = node->children;  child;  child = child->next) {
		uget_node_call_fake_filter (node, NULL, child);
		uget_node_make_fake (child);
	}
}

// fake filter node from real.
// If real node inserted a child node, all fake nodes call this to filter.
static void  uget_node_call_fake_filter (UgetNode* parent, UgetNode* sibling, UgetNode* child)
{
	UgetNode*    fake;
	UgetNodeFunc filter;

	for (fake = parent->fake;  fake;  fake = fake->peer) {
		filter = fake->control->filter;
		if (filter)
			filter (fake, sibling, child);
	}
}

// ----------------------------------------------------------------------------
// position
UgetNode* uget_node_nth_fake (UgetNode* node, int nth)
{
	UgetNode*  fake;

	for (fake = node->fake;  fake;  fake = fake->peer, nth--) {
		if (nth == 0)
			return fake;
	}
	return NULL;
}

int  uget_node_fake_position (UgetNode* node, UgetNode* fake)
{
	int  position = 0;

	for (node = node->fake;  node;  node = node->peer, position++) {
		if (node == fake)
			return position;
	}
	return -1;
}

// ----------------------------------------------------------------------------
// notify

void  uget_node_updated (UgetNode* node)
{
	UgNotifyFunc updated;

	updated = node->control->notifier->updated;
	if (updated)
		updated (node);
	// update fake node
	for (node = node->fake;  node;  node = node->peer)
		uget_node_updated (node);
}

// ----------------------------------------------------------------------------
// JSON

UgJsonError ug_json_parse_uget_node_children (UgJson* json,
                                const char* name, const char* value,
                                void* node, void* none)
{
	UgetNode*  temp;

	if (json->type != UG_JSON_OBJECT) {
//		if (json->type == UG_JSON_ARRAY)
//			ug_json_push (json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	}

	temp = uget_node_new (NULL);
	uget_node_append (node, temp);
	ug_json_push (json, ug_json_parse_entry, temp, (void*)UgetNodeEntry);
	return UG_JSON_ERROR_NONE;
}

void  ug_json_write_uget_node_children (UgJson* json, const UgetNode* node)
{
	for (node = node->children;  node;  node = node->next) {
		ug_json_write_object_head (json);
		ug_json_write_entry (json, (void*) node, UgetNodeEntry);
		ug_json_write_object_tail (json);
	}
}

// convert old format to new
static UgJsonError  ug_json_parse_name2data (UgJson* json,
                                const char* name, const char* value,
                                void* nodev, void* none)
{
	UgetNode*     node = nodev;
	union {
		UgetCommon*   common;
		UgetFiles*    files;
	} temp;

	if (json->type != UG_JSON_STRING)
		return UG_JSON_ERROR_TYPE_NOT_MATCH;

	// Now root node is category node
	if (node->parent == NULL || node->parent->parent == NULL) {
		// category or download node
		temp.common = ug_info_realloc(node->info, UgetCommonInfo);
		temp.common->name = ug_strdup(value);
	}
	else if (node->parent->parent->parent == NULL) {
		// file node
		temp.files = ug_info_realloc(node->parent->info, UgetFilesInfo);
		uget_files_realloc(temp.files, value);
	}
	return UG_JSON_ERROR_NONE;
}

// convert old format to new
static UgJsonError  ug_json_parse_state2group (UgJson* json,
                                const char* name, const char* value,
                                void* nodev, void* none)
{
	UgetRelation* relation;
	UgetNode*     node = nodev;
	int           group;

	if (json->type != UG_JSON_NUMBER)
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	else {
		group = strtol(value, NULL, 10);
		if (group != 0) {
			relation = ug_info_realloc(node->info, UgetRelationInfo);
			relation->group = group;
		}
	}
	return UG_JSON_ERROR_NONE;
}
