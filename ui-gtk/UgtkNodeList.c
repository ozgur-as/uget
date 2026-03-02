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

// GListModel implementation for UgetNode flat list (fake nodes).

#include <UgtkNodeList.h>
#include <UgtkNodeObject.h>

// GListModel interface
static GType    list_model_get_item_type (GListModel* list);
static guint    list_model_get_n_items   (GListModel* list);
static gpointer list_model_get_item      (GListModel* list, guint position);

static void ugtk_node_list_list_model_init (GListModelInterface* iface)
{
	iface->get_item_type = list_model_get_item_type;
	iface->get_n_items   = list_model_get_n_items;
	iface->get_item      = list_model_get_item;
}

G_DEFINE_TYPE_WITH_CODE (UgtkNodeList, ugtk_node_list, G_TYPE_OBJECT,
	G_IMPLEMENT_INTERFACE (G_TYPE_LIST_MODEL, ugtk_node_list_list_model_init))

static void ugtk_node_list_class_init (UgtkNodeListClass* klass)
{
}

static void ugtk_node_list_init (UgtkNodeList* ulist)
{
	ulist->root = NULL;
	ulist->n_fake = 0;
	ulist->root_visible = FALSE;
	ulist->last_count = 0;
}

// --- GListModel helpers ---

static gint ugtk_node_list_count (UgtkNodeList* ulist)
{
	UgetNode* node;
	gint n;

	if (ulist->root == NULL)
		return 0;

	for (n = 0, node = ulist->root->fake;  node;  node = node->peer)
		n++;
	if (n > ulist->n_fake)
		n = ulist->n_fake;
	if (ulist->root_visible)
		n++;
	return n;
}

static UgetNode* ugtk_node_list_nth (UgtkNodeList* ulist, guint position)
{
	if (ulist->root == NULL)
		return NULL;

	if (ulist->root_visible) {
		if (position == 0)
			return ulist->root;
		position--;
	}

	if ((gint)position >= ulist->n_fake)
		return NULL;
	return uget_node_nth_fake (ulist->root, position);
}

// --- GListModel interface ---

static GType list_model_get_item_type (GListModel* list)
{
	return UGTK_TYPE_NODE_OBJECT;
}

static guint list_model_get_n_items (GListModel* list)
{
	return ugtk_node_list_count (UGTK_NODE_LIST (list));
}

static gpointer list_model_get_item (GListModel* list, guint position)
{
	UgtkNodeList* ulist = UGTK_NODE_LIST (list);
	UgetNode* node;

	node = ugtk_node_list_nth (ulist, position);
	if (node == NULL)
		return NULL;
	return ugtk_node_object_new (node);
}

// --- public API ---

UgtkNodeList* ugtk_node_list_new (UgetNode* root, gint n_fake, gboolean root_visible)
{
	UgtkNodeList* ulist;

	ulist = g_object_new (UGTK_TYPE_NODE_LIST, NULL);
	ulist->root = root;
	ulist->n_fake = n_fake;
	ulist->root_visible = root_visible;
	return ulist;
}

void ugtk_node_list_refresh (UgtkNodeList* ulist)
{
	guint old_n = ulist->last_count;
	guint new_n = ugtk_node_list_count (ulist);
	ulist->last_count = new_n;
	g_list_model_items_changed (G_LIST_MODEL (ulist), 0, old_n, new_n);
}
