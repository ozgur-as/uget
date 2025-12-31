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

#include <UgRegistry.h>
#include <UgString.h>
#include <UgetNode.h>
#include <UgetData.h>
#include <UgtkNodeTree.h>
#include <UgtkNodeView.h>

#include <glib/gi18n.h>

// ------------------------------------
// column data & functions for Common

#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 10
static const UgPair state_icon_pair[] =
{
	{(void*)(intptr_t) UGET_GROUP_FINISHED,  "go-last"},
	{(void*)(intptr_t) UGET_GROUP_RECYCLED,  "list-remove"},
	{(void*)(intptr_t) UGET_GROUP_PAUSED,    "media-playback-pause"},
	{(void*)(intptr_t) UGET_GROUP_ERROR,     "dialog-error"},
	{(void*)(intptr_t) UGET_GROUP_UPLOADING, "go-up"},
	{(void*)(intptr_t) UGET_GROUP_COMPLETED, "gtk-yes"},
	{(void*)(intptr_t) UGET_GROUP_QUEUING,   "text-x-generic"},
	{(void*)(intptr_t) UGET_GROUP_ACTIVE,    "media-playback-start"},
};
#else
static const UgPair state_icon_pair[] =
{
	{(void*)(intptr_t) UGET_GROUP_FINISHED,  GTK_STOCK_GOTO_LAST},
	{(void*)(intptr_t) UGET_GROUP_RECYCLED,  GTK_STOCK_DELETE},
	{(void*)(intptr_t) UGET_GROUP_PAUSED,    GTK_STOCK_MEDIA_PAUSE},
	{(void*)(intptr_t) UGET_GROUP_ERROR,     GTK_STOCK_DIALOG_ERROR},
	{(void*)(intptr_t) UGET_GROUP_UPLOADING, GTK_STOCK_GO_UP},
	{(void*)(intptr_t) UGET_GROUP_COMPLETED, GTK_STOCK_YES},
	{(void*)(intptr_t) UGET_GROUP_QUEUING,   GTK_STOCK_FILE},
	{(void*)(intptr_t) UGET_GROUP_ACTIVE,    GTK_STOCK_MEDIA_PLAY},
};
#endif
static const int state_icon_pair_len = sizeof (state_icon_pair) / sizeof (UgPair);

static void col_set_icon (GtkTreeViewColumn *tree_column,
                          GtkCellRenderer   *cell,
                          GtkTreeModel      *model,
                          GtkTreeIter       *iter,
                          gpointer           data)
{
	UgetNode*      node;
	UgetRelation*  relation;
	const gchar*   icon_name;
	int            key, index;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;

	node = node->base;
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 10
	icon_name = "text-x-generic";
#else
	icon_name = GTK_STOCK_FILE;
#endif
	// select icon_name
	for (index = 0;  index < state_icon_pair_len;  index++) {
		key = (intptr_t)state_icon_pair[index].key;
		relation = ug_info_realloc(node->info, UgetRelationInfo);
		if ((key & relation->group) == key) {
			icon_name = state_icon_pair[index].data;
			break;
		}
	}
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 10
	g_object_set (cell, "icon-name", icon_name, NULL);
#else
	g_object_set (cell, "stock-id", icon_name, NULL);
#endif
}

// ------------------------------------
// column functions for Download

static void col_set_name (GtkTreeViewColumn *tree_column,
                          GtkCellRenderer   *cell,
                          GtkTreeModel      *model,
                          GtkTreeIter       *iter,
                          gpointer           data)
{
//	UgtkNodeTree* utree;
	UgetCommon*   common;
	UgetNode*     node;
	char*         name;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;

//	if (UGTK_IS_NODE_TREE (model)) {
//		// prefix.root
//		utree = UGTK_NODE_TREE (model);
//		if (utree->prefix.root && utree->prefix.root->children == node) {
//			g_object_set (cell, "text", utree->prefix.name, NULL);
//			return;
//		}
//	}

	node = node->base;
	name = _("unnamed");
	common = ug_info_get(node->info, UgetCommonInfo);

	if (common && common->name)
		name = common->name;
	else {
		common = ug_info_get (node->info, UgetCommonInfo);
		if (common) {
			if (common->file)
				name = common->file;
			else if (common->uri)
				name = common->uri;
		}
	}
	g_object_set (cell, "text", name, NULL);
}

static void col_set_complete (GtkTreeViewColumn *tree_column,
                              GtkCellRenderer   *cell,
                              GtkTreeModel      *model,
                              GtkTreeIter       *iter,
                              gpointer           data)
{
	UgetNode*       node;
	UgetProgress*   progress;
	char*           string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress && progress->total)
		string = ug_str_from_int_unit (progress->complete, NULL);
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

static void col_set_total (GtkTreeViewColumn *tree_column,
                           GtkCellRenderer   *cell,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           data)
{
	UgetNode*       node;
	UgetProgress*   progress;
	char*           string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress && progress->total)
		string = ug_str_from_int_unit (progress->total, NULL);
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

static void col_set_percent (GtkTreeViewColumn *tree_column,
                             GtkCellRenderer   *cell,
                             GtkTreeModel      *model,
                             GtkTreeIter       *iter,
                             gpointer           data)
{
	UgetNode*       node;
	UgetProgress*   progress;
	char*           string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress && progress->total) {
		string = ug_strdup_printf ("%d%c", progress->percent, '%');
		g_object_set (cell, "visible", TRUE, NULL);
		g_object_set (cell, "value", progress->percent, NULL);
		g_object_set (cell, "text", string, NULL);
		ug_free (string);
	}
	else {
		g_object_set (cell, "visible", FALSE, NULL);
		g_object_set (cell, "value", 0, NULL);
		g_object_set (cell, "text", "", NULL);
	}
}

// consume time
static void col_set_elapsed (GtkTreeViewColumn *tree_column,
                             GtkCellRenderer   *cell,
                             GtkTreeModel      *model,
                             GtkTreeIter       *iter,
                             gpointer           data)
{
	UgetNode*     node;
	UgetProgress* progress;
	char*         string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress)
		string = ug_str_from_seconds ((int) progress->elapsed, TRUE);
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

// remain time
static void col_set_left (GtkTreeViewColumn *tree_column,
                          GtkCellRenderer   *cell,
                          GtkTreeModel      *model,
                          GtkTreeIter       *iter,
                          gpointer           data)
{
	UgetNode*     node;
	UgetProgress* progress;
	UgetRelation* relation;
	char*         string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	progress = ug_info_get (node->info, UgetProgressInfo);
	relation = ug_info_get (node->info, UgetRelationInfo);

	if (progress && relation && relation->task)
		string = ug_str_from_seconds ((int) progress->left, TRUE);
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

static void col_set_speed (GtkTreeViewColumn *tree_column,
                           GtkCellRenderer   *cell,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           data)
{
	UgetNode*     node;
	UgetProgress* progress;
	UgetRelation* relation;
	char*         string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	progress = ug_info_get (node->info, UgetProgressInfo);
	relation = ug_info_get (node->info, UgetRelationInfo);

	if (progress && relation && relation->task)
		string = ug_str_from_int_unit (progress->download_speed, "/s");
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

static void col_set_upload_speed (GtkTreeViewColumn *tree_column,
                           GtkCellRenderer   *cell,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           data)
{
	UgetNode*     node;
	UgetProgress* progress;
	UgetRelation* relation;
	char*         string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	progress = ug_info_get (node->info, UgetProgressInfo);
	relation = ug_info_get (node->info, UgetRelationInfo);

	if (progress && relation && relation->task && progress->upload_speed)
		string = ug_str_from_int_unit (progress->upload_speed, "/s");
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

static void col_set_uploaded (GtkTreeViewColumn *tree_column,
                              GtkCellRenderer   *cell,
                              GtkTreeModel      *model,
                              GtkTreeIter       *iter,
                              gpointer           data)
{
	UgetNode*     node;
	UgetProgress* progress;
	char*         string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress && progress->uploaded)
		string = ug_str_from_int_unit (progress->uploaded, NULL);
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

static void col_set_ratio (GtkTreeViewColumn *tree_column,
                           GtkCellRenderer   *cell,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           data)
{
	UgetNode*     node;
	UgetProgress* progress;
	char*         string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress && progress->ratio)
		string = ug_strdup_printf ("%.2f", progress->ratio);
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

static void col_set_retry (GtkTreeViewColumn *tree_column,
                           GtkCellRenderer   *cell,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           data)
{
	UgetNode*     node;
	UgetCommon*   common;
	char*         string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	common = ug_info_get (node->info, UgetCommonInfo);
	if (common == NULL || common->retry_count == 0)
		string = NULL;
	else if (common->retry_count < 100)
		string = ug_strdup_printf ("%d", common->retry_count);
	else {
		g_object_set (cell, "text", "> 99", NULL);
		return;
	}

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

static void col_set_category (GtkTreeViewColumn *tree_column,
                              GtkCellRenderer   *cell,
                              GtkTreeModel      *model,
                              GtkTreeIter       *iter,
                              gpointer           data)
{
	UgetCommon*   common;
	UgetNode*     node;
	char*         string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	if (node->parent) {
		common = ug_info_get(node->parent->info, UgetCommonInfo);
		if (common)
			string = common->name;
		else
			string = NULL;
	}
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
}

static void col_set_uri (GtkTreeViewColumn *tree_column,
                         GtkCellRenderer   *cell,
                         GtkTreeModel      *model,
                         GtkTreeIter       *iter,
                         gpointer           data)
{
	UgetNode*     node;
	UgetCommon*   common;
	char*         string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	common = ug_info_get (node->info, UgetCommonInfo);
	if (common)
		string = common->uri;
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
}

static void col_set_added_on (GtkTreeViewColumn *tree_column,
                              GtkCellRenderer   *cell,
                              GtkTreeModel      *model,
                              GtkTreeIter       *iter,
                              gpointer           data)
{
	UgetNode*   node;
	UgetLog*    ulog;
	char*       string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	ulog = ug_info_get (node->info, UgetLogInfo);
	if (ulog && ulog->added_time)
		string = ug_str_from_time (ulog->added_time, FALSE);
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

static void col_set_completed_on (GtkTreeViewColumn *tree_column,
                                  GtkCellRenderer   *cell,
                                  GtkTreeModel      *model,
                                  GtkTreeIter       *iter,
                                  gpointer           data)
{
	UgetNode*   node;
	UgetLog*    ulog;
	char*       string;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;
	node = node->base;

	ulog = ug_info_get (node->info, UgetLogInfo);
	if (ulog && ulog->completed_time)
		string = ug_str_from_time (ulog->completed_time, FALSE);
	else
		string = NULL;

	g_object_set (cell, "text", string, NULL);
	ug_free (string);
}

// ------------------------------------
// column functions for Category, Status

static void col_set_quantity (GtkTreeViewColumn *tree_column,
                              GtkCellRenderer   *cell,
                              GtkTreeModel      *model,
                              GtkTreeIter       *iter,
                              gpointer           data)
{
	UgetNode*  node;
	gchar*     quantity;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;

	quantity = ug_strdup_printf ("%u", node->n_children);
	g_object_set (cell, "text", quantity, NULL);
	ug_free (quantity);
}

// ------------------------------------
// column functions for Category

static void col_set_name_c (GtkTreeViewColumn *tree_column,
                            GtkCellRenderer   *cell,
                            GtkTreeModel      *model,
                            GtkTreeIter       *iter,
                            gpointer           data)
{
	UgetCommon*   common;
	UgetNode*     node;
	char*         name;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;

//	if (UGTK_IS_NODE_TREE (model)) {
//		UgtkNodeTree* utree;
//		// prefix.root
//		utree = UGTK_NODE_TREE (model);
//		if (utree->prefix.root && utree->prefix.root->children == node) {
//			g_object_set (cell, "text", _("All Category"), NULL);
//			return;
//		}
//	}

	node = node->base;
	common = ug_info_get(node->info, UgetCommonInfo);
	if (common && common->name)
		name = common->name;
	else
		name = _("unnamed");

	g_object_set (cell, "text", name, NULL);
}

static void col_set_icon_c (GtkTreeViewColumn *tree_column,
                            GtkCellRenderer   *cell,
                            GtkTreeModel      *model,
                            GtkTreeIter       *iter,
                            gpointer           data)
{
	UgetNode*  node;

	node = iter->user_data;
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 10
	if (uget_node_get_group(node) & UGET_GROUP_PAUSED)
		g_object_set (cell, "icon-name", "media-playback-pause", NULL);
	else
		g_object_set (cell, "icon-name", "gtk-dnd-multiple", NULL);
#else
	if (uget_node_get_group(node) & UGET_GROUP_PAUSED)
		g_object_set (cell, "stock-id", GTK_STOCK_MEDIA_PAUSE, NULL);
	else
		g_object_set (cell, "stock-id", GTK_STOCK_DND_MULTIPLE, NULL);
#endif
}

// ------------------------------------
// column functions for Status

static const UgPair state_name_pair[] =
{
	{(void*)(intptr_t) UGET_GROUP_ERROR,     N_("Error")},
	{(void*)(intptr_t) UGET_GROUP_PAUSED,    N_("Paused")},
	{(void*)(intptr_t) UGET_GROUP_UPLOADING, N_("Uploading")},
	{(void*)(intptr_t) UGET_GROUP_COMPLETED, N_("Completed")},
	{(void*)(intptr_t) UGET_GROUP_FINISHED,  N_("Finished")},
	{(void*)(intptr_t) UGET_GROUP_RECYCLED,  N_("Recycled")},
	{(void*)(intptr_t) UGET_GROUP_QUEUING,   N_("Queuing")},
	{(void*)(intptr_t) UGET_GROUP_ACTIVE,    N_("Active")},
};
static const int state_name_pair_len = sizeof (state_name_pair) / sizeof (UgPair);

static void col_set_name_s (GtkTreeViewColumn *tree_column,
                            GtkCellRenderer   *cell,
                            GtkTreeModel      *model,
                            GtkTreeIter       *iter,
                            gpointer           data)
{
	UgetNode*     node;
	char*         name;
	int           key;
	int           index;
	int           group;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;

	name = _("All Status");
	if (node->real) {
		group = uget_node_get_group(node);
		for (index = 0;  index < state_name_pair_len;  index++) {
			key = (intptr_t)state_name_pair[index].key;
			if ((key & group) == key) {
				name = gettext (state_name_pair[index].data);
				break;
			}
		}
	}
	g_object_set (cell, "text", name, NULL);
}

static void col_set_icon_s (GtkTreeViewColumn *tree_column,
                            GtkCellRenderer   *cell,
                            GtkTreeModel      *model,
                            GtkTreeIter       *iter,
                            gpointer           data)
{
	UgetNode*      node;
	const gchar*   icon_name;
	int            key, index;
	int            group;

//	gtk_tree_model_get (model, iter, 0, &node, -1);
	node = iter->user_data;
	// avoid crash in GTK3
	if (node == NULL)
		return;

	// select icon_name
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 10
	icon_name = "gtk-dnd-multiple";
#else
	icon_name = GTK_STOCK_DND_MULTIPLE;
#endif
	if (node->real) {
		group = uget_node_get_group(node);
		for (index = 0;  index < state_icon_pair_len;  index++) {
			key = (intptr_t)state_icon_pair[index].key;
			if ((key & group) == key) {
				icon_name = state_icon_pair[index].data;
				break;
			}
		}
	}
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 10
	g_object_set (cell, "icon-name", icon_name, NULL);
#else
	g_object_set (cell, "stock-id", icon_name, NULL);
#endif
}

// ----------------------------------------------------------------------------
// UgtkNodeView

static GtkWidget*  ugtk_node_view_new (GtkTreeCellDataFunc icon_func,
                                       GtkTreeCellDataFunc name_func,
                                       const gchar*        name_title)
{
	GtkTreeView*       view;
	GtkCellRenderer*   renderer;
	GtkTreeViewColumn* column;

	view = (GtkTreeView*)gtk_tree_view_new ();

	// UGTK_NODE_COLUMN_STATE
	// GtkCellRendererPixbuf "stock-size" = 1, 16x16
	column = gtk_tree_view_column_new ();
//	gtk_tree_view_column_set_title (column, "");
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         icon_func,
	                                         NULL, NULL);
//	gtk_tree_view_column_set_resizable (column, FALSE);
	gtk_tree_view_column_set_min_width (column, 18);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_NAME
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, name_title);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         name_func,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_append_column (view, column);

	gtk_widget_show (GTK_WIDGET (view));
	return (GtkWidget*) view;
}

GtkWidget*  ugtk_node_view_new_for_download (void)
{
	GtkTreeView*       view;
	GtkTreeSelection*  selection;
	GtkCellRenderer*   renderer;
	GtkCellRenderer*   renderer_progress;
	GtkTreeViewColumn* column;

	view = (GtkTreeView*) ugtk_node_view_new (col_set_icon,
			col_set_name, _("Name"));
	selection = gtk_tree_view_get_selection (view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

	// UGTK_NODE_COLUMN_STATE
	column = gtk_tree_view_get_column (view, UGTK_NODE_COLUMN_STATE);
	gtk_tree_view_column_set_resizable (column, TRUE);

	// UGTK_NODE_COLUMN_NAME
	column = gtk_tree_view_get_column (view, UGTK_NODE_COLUMN_NAME);
	gtk_tree_view_column_set_min_width (column, 180);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);

	// UGTK_NODE_COLUMN_COMPLETE
	column = gtk_tree_view_column_new ();
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_set_title (column, _("Complete"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_complete,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 70);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_TOTAL
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Size"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_total,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 70);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_PERCENT
	column = gtk_tree_view_column_new ();
	renderer_progress = gtk_cell_renderer_progress_new ();
	gtk_tree_view_column_set_title (column, _("%"));
	gtk_tree_view_column_pack_start (column, renderer_progress, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer_progress,
	                                         col_set_percent,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 60);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_ELAPSED for consuming time
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Elapsed"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_elapsed,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 65);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_LEFT for remaining time
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Left"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_left,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 65);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_SPEED
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Speed"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_speed,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 80);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_UPLOAD_SPEED
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Up Speed"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_upload_speed,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 80);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_UPLOADED
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Uploaded"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_uploaded,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 70);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_RATIO
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Ratio"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_ratio,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 45);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_RETRY
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Retry"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_retry,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 45);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_alignment (column, 1.0);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_CATEGORY
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Category"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_category,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 100);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_URI
//	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("URI"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_uri,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 300);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_ADDED_ON
//	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Added On"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_added_on,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 140);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (view, column);

	// UGTK_NODE_COLUMN_COMPLETED_ON
//	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Completed On"));
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_completed_on,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_min_width (column, 140);
	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (view, column);

	gtk_tree_view_set_fixed_height_mode (view, TRUE);
	gtk_widget_show (GTK_WIDGET (view));
	return (GtkWidget*) view;
}

static void add_column_quantity (GtkTreeView* view)
{
	GtkCellRenderer*   renderer;
	GtkTreeViewColumn* column;

	// UGTK_NODE_COLUMN_QUANTITY : number of tasks
	column = gtk_tree_view_column_new ();
	gtk_tree_view_column_set_title (column, _("Quantity"));
//	gtk_tree_view_column_set_title (column, _("N"));
	renderer = gtk_cell_renderer_text_new ();
	g_object_set (renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func (column,
	                                         renderer,
	                                         col_set_quantity,
	                                         NULL, NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	gtk_tree_view_column_set_expand (column, TRUE);
	gtk_tree_view_column_set_alignment (column, 1.0);
//	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	gtk_tree_view_append_column (view, column);
}

GtkWidget*  ugtk_node_view_new_for_category (void)
{
	GtkTreeView*  view;
//	GtkCellRenderer*   renderer;
//	GtkTreeViewColumn* column;

	view = (GtkTreeView*) ugtk_node_view_new (col_set_icon_c,
			col_set_name_c, _("Category"));
	gtk_tree_view_set_headers_visible (view, FALSE);

	// UGTK_NODE_COLUMN_NAME
//	column = gtk_tree_view_get_column (view, 0);
//	gtk_tree_view_column_set_min_width (column, -1);
//	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	// column Quantity = number of tasks
	add_column_quantity (view);

	return (GtkWidget*) view;
}

GtkWidget*  ugtk_node_view_new_for_state (void)
{
	GtkTreeView*  view;
//	GtkTreeViewColumn* column;

	view = (GtkTreeView*) ugtk_node_view_new (col_set_icon_s,
			col_set_name_s, _("Status"));
	gtk_tree_view_set_headers_visible (view, FALSE);
//	column = gtk_tree_view_get_column (view, 0);
//	gtk_tree_view_column_set_title (column, _("Status"));

	// column Quantity = number of tasks
	add_column_quantity (view);

	return (GtkWidget*) view;
}

void  ugtk_node_view_use_large_icon (GtkTreeView* view, gboolean is_large, int fixed_width)
{
	GtkTreeViewColumn*  column;
	GtkIconSize         icon_size;
	int                 icon_width;
	GList*              list;

	if (is_large) {
		icon_size = GTK_ICON_SIZE_LARGE;
		icon_width = 24 + 2;    // icon size + 2 pixel
	}
	else {
		icon_size = GTK_ICON_SIZE_NORMAL;
		icon_width = 16 + 2;    // icon size + 2 pixel
	}

	column = gtk_tree_view_get_column (view, UGTK_NODE_COLUMN_STATE);
	gtk_tree_view_column_set_min_width (column, icon_width);
	gtk_tree_view_column_set_fixed_width (column, fixed_width);
	list = gtk_cell_layout_get_cells (GTK_CELL_LAYOUT(column));
	g_object_set (list->data, "stock-size", icon_size, NULL);
	g_list_free (list);
}
