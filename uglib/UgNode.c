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

#ifdef HAVE_GLIB
#include <glib.h>    // g_malloc/g_free
#endif // HAVE_GLIB

#include <stdlib.h>
#include <UgDefine.h>
#include <UgNode.h>

UgNode* ug_node_new  (void)
{
#ifdef HAVE_GLIB
	return g_malloc0 (sizeof (UgNode));
#else
	return ug_malloc0 (sizeof (UgNode));
#endif
}

void ug_node_free (UgNode* link)
{
#ifdef HAVE_GLIB
	g_free (link);
#else
	ug_free (link);
#endif // HAVE_GLIB
}

void   ug_node_init (UgNode* node)
{
	node->data = NULL;
	node->next = NULL;
	node->prev = NULL;
	node->parent = NULL;
	node->children = NULL;
	node->last = NULL;
	node->n_children = 0;
}

void    ug_node_reverse (UgNode* node)
{
	UgNode* temp;

	temp = node->last;
	node->last = node->children;
	node->children = temp;
	// warning: node->children is pointing to last now.
	for (node = temp;  node;  node = temp) {
		temp = node->prev;
		node->prev = node->next;
		node->next = temp;
	}
}

void    ug_node_prepend (UgNode* parent, UgNode* node)
{
	if (parent->children)
		parent->children->prev = node;
	else	// if (parent->last)
		parent->last = node;
	node->parent = parent;
	node->next = parent->children;
	parent->children = node;
	parent->n_children++;
}

void   ug_node_append (UgNode* parent, UgNode* node)
{
	if (parent->last)
		parent->last->next = node;
	else	// if (parent->children)
		parent->children = node;
	node->parent = parent;
	node->prev = parent->last;
	parent->last = node;
	parent->n_children++;
}

void   ug_node_insert (UgNode* parent, UgNode *sibling, UgNode* node)
{
	if (sibling == NULL) {
		ug_node_append (parent, node);
		return;
	}
	if (sibling == parent->children) {
		ug_node_prepend (parent, node);
		return;
	}
//	if (sibling->prev)
		sibling->prev->next = node;
	node->parent = parent;
	node->next = sibling;
	node->prev = sibling->prev;
	sibling->prev = node;
	parent->n_children++;
}

void   ug_node_remove (UgNode* parent, UgNode* node)
{
	if (parent->last == node)
		parent->last = node->prev;
	if (parent->children == node)
		parent->children = node->next;

	if (node->next)
		node->next->prev = node->prev;
	if (node->prev)
		node->prev->next = node->next;
	node->next = NULL;
	node->prev = NULL;
	node->parent = NULL;
	parent->n_children--;
}

void   ug_node_unlink (UgNode* node)
{
	if (node->parent)
		ug_node_remove (node->parent, node);
	else {
		if (node->prev) {
			node->prev->next = node->next;
			node->prev = NULL;
		}
		if (node->next) {
			node->next->prev = node->prev;
			node->next = NULL;
		}
	}

	while (node->children)
		ug_node_remove (node, node->children);
}

UgNode* ug_node_nth_child (UgNode* node, int nth)
{
	if (nth >= 0) {
		for (node = node->children;  node;  node = node->next, nth--) {
			if (nth == 0)
				return node;
		}
	}
	return NULL;
}

int   ug_node_child_position (UgNode* node, UgNode* child)
{
	int  position = 0;

	for (node = node->children;  node;  node = node->next, position++) {
		if (node == child)
			return position;
	}
	return -1;
}

