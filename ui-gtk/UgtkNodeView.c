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
#include <UgtkNodeObject.h>
#include <UgtkNodeView.h>

#include <glib/gi18n.h>

// Helper: get UgetNode* from a GtkListItem
static inline UgetNode* get_node_from_item (GtkListItem* item)
{
	UgtkNodeObject* obj = gtk_list_item_get_item (item);
	if (obj == NULL)
		return NULL;
	return obj->node;
}

// ------------------------------------
// Factory setup/bind helpers

static void setup_icon (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	GtkWidget* image = gtk_image_new ();
	gtk_image_set_pixel_size (GTK_IMAGE (image), 16);
	gtk_list_item_set_child (item, image);
}

static void setup_label (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	GtkWidget* label = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
	gtk_list_item_set_child (item, label);
}

static void setup_label_right (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	GtkWidget* label = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (label), 1.0);
	gtk_list_item_set_child (item, label);
}

static void setup_progress (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	GtkWidget* bar = gtk_progress_bar_new ();
	gtk_progress_bar_set_show_text (GTK_PROGRESS_BAR (bar), TRUE);
	gtk_widget_set_valign (bar, GTK_ALIGN_CENTER);
	gtk_list_item_set_child (item, bar);
}

// ------------------------------------
// icon/state pairs for downloads

static const UgPair state_icon_pair[] =
{
	{(void*)(intptr_t) UGET_GROUP_FINISHED,  "go-last"},
	{(void*)(intptr_t) UGET_GROUP_RECYCLED,  "list-remove"},
	{(void*)(intptr_t) UGET_GROUP_PAUSED,    "media-playback-pause"},
	{(void*)(intptr_t) UGET_GROUP_ERROR,     "dialog-error"},
	{(void*)(intptr_t) UGET_GROUP_UPLOADING, "go-up"},
	{(void*)(intptr_t) UGET_GROUP_COMPLETED, "object-select-symbolic"},
	{(void*)(intptr_t) UGET_GROUP_QUEUING,   "text-x-generic"},
	{(void*)(intptr_t) UGET_GROUP_ACTIVE,    "media-playback-start"},
};
static const int state_icon_pair_len = sizeof (state_icon_pair) / sizeof (UgPair);

// ------------------------------------
// Bind callbacks for Download columns

static void bind_icon (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*      node;
	UgetRelation*  relation;
	const gchar*   icon_name;
	int            key, index;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	icon_name = "text-x-generic";
	for (index = 0;  index < state_icon_pair_len;  index++) {
		key = (intptr_t)state_icon_pair[index].key;
		relation = ug_info_realloc(node->info, UgetRelationInfo);
		if ((key & relation->group) == key) {
			icon_name = state_icon_pair[index].data;
			break;
		}
	}
	gtk_image_set_from_icon_name (GTK_IMAGE (gtk_list_item_get_child (item)), icon_name);
}

static void bind_name (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetCommon*   common;
	UgetNode*     node;
	char*         name;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
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
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), name);
}

static void bind_complete (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*       node;
	UgetProgress*   progress;
	char*           string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress && progress->total)
		string = ug_str_from_int_unit (progress->complete, NULL);
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

static void bind_total (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*       node;
	UgetProgress*   progress;
	char*           string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress && progress->total)
		string = ug_str_from_int_unit (progress->total, NULL);
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

static void bind_percent (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*       node;
	UgetProgress*   progress;
	GtkProgressBar* bar;
	char*           string;

	node = get_node_from_item (item);
	bar = GTK_PROGRESS_BAR (gtk_list_item_get_child (item));
	if (node == NULL) {
		gtk_progress_bar_set_fraction (bar, 0.0);
		gtk_progress_bar_set_text (bar, "");
		return;
	}
	node = node->base;
	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress && progress->total) {
		string = ug_strdup_printf ("%d%c", progress->percent, '%');
		gtk_progress_bar_set_fraction (bar, progress->percent / 100.0);
		gtk_progress_bar_set_text (bar, string);
		gtk_widget_set_visible (GTK_WIDGET (bar), TRUE);
		ug_free (string);
	}
	else {
		gtk_progress_bar_set_fraction (bar, 0.0);
		gtk_progress_bar_set_text (bar, "");
		gtk_widget_set_visible (GTK_WIDGET (bar), FALSE);
	}
}

static void bind_elapsed (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*     node;
	UgetProgress* progress;
	char*         string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress)
		string = ug_str_from_seconds ((int) progress->elapsed, TRUE);
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

static void bind_left (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*     node;
	UgetProgress* progress;
	UgetRelation* relation;
	char*         string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	progress = ug_info_get (node->info, UgetProgressInfo);
	relation = ug_info_get (node->info, UgetRelationInfo);
	if (progress && relation && relation->task)
		string = ug_str_from_seconds ((int) progress->left, TRUE);
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

static void bind_speed (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*     node;
	UgetProgress* progress;
	UgetRelation* relation;
	char*         string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	progress = ug_info_get (node->info, UgetProgressInfo);
	relation = ug_info_get (node->info, UgetRelationInfo);
	if (progress && relation && relation->task)
		string = ug_str_from_int_unit (progress->download_speed, "/s");
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

static void bind_upload_speed (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*     node;
	UgetProgress* progress;
	UgetRelation* relation;
	char*         string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	progress = ug_info_get (node->info, UgetProgressInfo);
	relation = ug_info_get (node->info, UgetRelationInfo);
	if (progress && relation && relation->task && progress->upload_speed)
		string = ug_str_from_int_unit (progress->upload_speed, "/s");
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

static void bind_uploaded (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*     node;
	UgetProgress* progress;
	char*         string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress && progress->uploaded)
		string = ug_str_from_int_unit (progress->uploaded, NULL);
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

static void bind_ratio (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*     node;
	UgetProgress* progress;
	char*         string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	progress = ug_info_get (node->info, UgetProgressInfo);
	if (progress && progress->ratio)
		string = ug_strdup_printf ("%.2f", progress->ratio);
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

static void bind_retry (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*     node;
	UgetCommon*   common;
	char*         string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	common = ug_info_get (node->info, UgetCommonInfo);
	if (common == NULL || common->retry_count == 0)
		string = NULL;
	else if (common->retry_count < 100)
		string = ug_strdup_printf ("%d", common->retry_count);
	else {
		gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), "> 99");
		return;
	}
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

static void bind_category (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetCommon*   common;
	UgetNode*     node;
	char*         string;

	node = get_node_from_item (item);
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
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
}

static void bind_uri (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*     node;
	UgetCommon*   common;
	char*         string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	common = ug_info_get (node->info, UgetCommonInfo);
	if (common)
		string = common->uri;
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
}

static void bind_added_on (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*   node;
	UgetLog*    ulog;
	char*       string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	ulog = ug_info_get (node->info, UgetLogInfo);
	if (ulog && ulog->added_time)
		string = ug_str_from_time (ulog->added_time, FALSE);
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

static void bind_completed_on (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*   node;
	UgetLog*    ulog;
	char*       string;

	node = get_node_from_item (item);
	if (node == NULL)
		return;
	node = node->base;
	ulog = ug_info_get (node->info, UgetLogInfo);
	if (ulog && ulog->completed_time)
		string = ug_str_from_time (ulog->completed_time, FALSE);
	else
		string = NULL;
	gtk_label_set_text (GTK_LABEL (gtk_list_item_get_child (item)), string ? string : "");
	ug_free (string);
}

// ------------------------------------
// Status name pairs (used by sidebar bind_state_row)

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

// ------------------------------------
// Sidebar row setup/bind (shared by category & state GtkListView)

static void setup_sidebar_row (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	GtkWidget* box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 4);
	GtkWidget* image = gtk_image_new ();
	GtkWidget* name = gtk_label_new (NULL);
	GtkWidget* qty = gtk_label_new (NULL);

	gtk_image_set_pixel_size (GTK_IMAGE (image), 16);
	gtk_widget_set_margin_start (box, 4);
	gtk_widget_set_margin_end (box, 4);
	gtk_label_set_xalign (GTK_LABEL (name), 0.0);
	gtk_label_set_ellipsize (GTK_LABEL (name), PANGO_ELLIPSIZE_END);
	gtk_widget_set_hexpand (name, TRUE);
	gtk_label_set_xalign (GTK_LABEL (qty), 1.0);

	gtk_box_append (GTK_BOX (box), image);
	gtk_box_append (GTK_BOX (box), name);
	gtk_box_append (GTK_BOX (box), qty);
	gtk_list_item_set_child (item, box);
}

static void bind_category_row (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*    node;
	UgetCommon*  common;
	GtkWidget*   box;
	GtkWidget*   image;
	GtkWidget*   name_label;
	GtkWidget*   qty_label;
	gchar*       quantity;

	node = get_node_from_item (item);
	if (node == NULL)
		return;

	box = gtk_list_item_get_child (item);
	image = gtk_widget_get_first_child (box);
	name_label = gtk_widget_get_next_sibling (image);
	qty_label = gtk_widget_get_next_sibling (name_label);

	// icon
	if (uget_node_get_group(node) & UGET_GROUP_PAUSED)
		gtk_image_set_from_icon_name (GTK_IMAGE (image), "media-playback-pause");
	else
		gtk_image_set_from_icon_name (GTK_IMAGE (image), "view-list-symbolic");

	// name
	common = ug_info_get(node->base->info, UgetCommonInfo);
	if (common && common->name)
		gtk_label_set_text (GTK_LABEL (name_label), common->name);
	else
		gtk_label_set_text (GTK_LABEL (name_label), _("unnamed"));

	// quantity
	quantity = ug_strdup_printf ("%u", node->n_children);
	gtk_label_set_text (GTK_LABEL (qty_label), quantity);
	ug_free (quantity);
}

static void bind_state_row (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	UgetNode*      node;
	GtkWidget*     box;
	GtkWidget*     image;
	GtkWidget*     name_label;
	GtkWidget*     qty_label;
	const gchar*   icon_name;
	char*          name;
	gchar*         quantity;
	int            key, index, group;

	node = get_node_from_item (item);
	if (node == NULL)
		return;

	box = gtk_list_item_get_child (item);
	image = gtk_widget_get_first_child (box);
	name_label = gtk_widget_get_next_sibling (image);
	qty_label = gtk_widget_get_next_sibling (name_label);

	// icon
	icon_name = "view-list-symbolic";
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
	gtk_image_set_from_icon_name (GTK_IMAGE (image), icon_name);

	// name
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
	gtk_label_set_text (GTK_LABEL (name_label), name);

	// quantity
	quantity = ug_strdup_printf ("%u", node->n_children);
	gtk_label_set_text (GTK_LABEL (qty_label), quantity);
	ug_free (quantity);
}

// ------------------------------------
// Factory helper: create a factory with setup/bind callbacks

static void teardown_item (GtkSignalListItemFactory* factory, GtkListItem* item, gpointer data)
{
	gtk_list_item_set_child (item, NULL);
}

static GtkListItemFactory* make_factory (GCallback setup_fn, GCallback bind_fn)
{
	GtkListItemFactory* factory;

	factory = gtk_signal_list_item_factory_new ();
	g_signal_connect (factory, "setup", setup_fn, NULL);
	g_signal_connect (factory, "bind",  bind_fn,  NULL);
	g_signal_connect (factory, "teardown", G_CALLBACK (teardown_item), NULL);
	return factory;
}

// ------------------------------------
// Helper: add a column to a column view

static GtkColumnViewColumn* add_column (GtkColumnView* view, const gchar* title,
                                         GtkListItemFactory* factory, int min_width,
                                         gboolean resizable, gboolean expand)
{
	GtkColumnViewColumn* column;

	column = gtk_column_view_column_new (title, factory);
	gtk_column_view_column_set_resizable (column, resizable);
	gtk_column_view_column_set_expand (column, expand);
	if (min_width > 0)
		gtk_column_view_column_set_fixed_width (column, min_width);
	gtk_column_view_append_column (view, column);
	g_object_unref (column);
	return column;
}

// ----------------------------------------------------------------------------
// UgtkNodeView

GtkWidget* ugtk_node_view_new_for_download (void)
{
	GtkColumnView*  view;
	GtkListItemFactory* factory;

	view = GTK_COLUMN_VIEW (gtk_column_view_new (NULL));
	gtk_column_view_set_show_column_separators (view, TRUE);

	// UGTK_NODE_COLUMN_STATE (icon)
	factory = make_factory (G_CALLBACK (setup_icon), G_CALLBACK (bind_icon));
	add_column (view, "", factory, 28, FALSE, FALSE);

	// UGTK_NODE_COLUMN_NAME
	factory = make_factory (G_CALLBACK (setup_label), G_CALLBACK (bind_name));
	add_column (view, _("Name"), factory, 180, TRUE, TRUE);

	// UGTK_NODE_COLUMN_COMPLETE
	factory = make_factory (G_CALLBACK (setup_label_right), G_CALLBACK (bind_complete));
	add_column (view, _("Complete"), factory, 70, TRUE, FALSE);

	// UGTK_NODE_COLUMN_TOTAL
	factory = make_factory (G_CALLBACK (setup_label_right), G_CALLBACK (bind_total));
	add_column (view, _("Size"), factory, 70, TRUE, FALSE);

	// UGTK_NODE_COLUMN_PERCENT
	factory = make_factory (G_CALLBACK (setup_progress), G_CALLBACK (bind_percent));
	add_column (view, _("%"), factory, 60, TRUE, FALSE);

	// UGTK_NODE_COLUMN_ELAPSED
	factory = make_factory (G_CALLBACK (setup_label_right), G_CALLBACK (bind_elapsed));
	add_column (view, _("Elapsed"), factory, 65, TRUE, FALSE);

	// UGTK_NODE_COLUMN_LEFT
	factory = make_factory (G_CALLBACK (setup_label_right), G_CALLBACK (bind_left));
	add_column (view, _("Left"), factory, 65, TRUE, FALSE);

	// UGTK_NODE_COLUMN_SPEED
	factory = make_factory (G_CALLBACK (setup_label_right), G_CALLBACK (bind_speed));
	add_column (view, _("Speed"), factory, 80, TRUE, FALSE);

	// UGTK_NODE_COLUMN_UPLOAD_SPEED
	factory = make_factory (G_CALLBACK (setup_label_right), G_CALLBACK (bind_upload_speed));
	add_column (view, _("Up Speed"), factory, 80, TRUE, FALSE);

	// UGTK_NODE_COLUMN_UPLOADED
	factory = make_factory (G_CALLBACK (setup_label_right), G_CALLBACK (bind_uploaded));
	add_column (view, _("Uploaded"), factory, 70, TRUE, FALSE);

	// UGTK_NODE_COLUMN_RATIO
	factory = make_factory (G_CALLBACK (setup_label_right), G_CALLBACK (bind_ratio));
	add_column (view, _("Ratio"), factory, 45, TRUE, FALSE);

	// UGTK_NODE_COLUMN_RETRY
	factory = make_factory (G_CALLBACK (setup_label_right), G_CALLBACK (bind_retry));
	add_column (view, _("Retry"), factory, 45, TRUE, FALSE);

	// UGTK_NODE_COLUMN_CATEGORY
	factory = make_factory (G_CALLBACK (setup_label), G_CALLBACK (bind_category));
	add_column (view, _("Category"), factory, 100, TRUE, FALSE);

	// UGTK_NODE_COLUMN_URI
	factory = make_factory (G_CALLBACK (setup_label), G_CALLBACK (bind_uri));
	add_column (view, _("URI"), factory, 300, TRUE, FALSE);

	// UGTK_NODE_COLUMN_ADDED_ON
	factory = make_factory (G_CALLBACK (setup_label), G_CALLBACK (bind_added_on));
	add_column (view, _("Added On"), factory, 140, TRUE, FALSE);

	// UGTK_NODE_COLUMN_COMPLETED_ON
	factory = make_factory (G_CALLBACK (setup_label), G_CALLBACK (bind_completed_on));
	add_column (view, _("Completed On"), factory, 140, TRUE, FALSE);

	gtk_widget_set_visible (GTK_WIDGET (view), TRUE);
	return GTK_WIDGET (view);
}

GtkWidget* ugtk_node_view_new_for_category (void)
{
	GtkListView*        view;
	GtkListItemFactory* factory;

	factory = make_factory (G_CALLBACK (setup_sidebar_row),
	                        G_CALLBACK (bind_category_row));
	view = GTK_LIST_VIEW (gtk_list_view_new (NULL, factory));
	gtk_list_view_set_show_separators (view, FALSE);
	gtk_widget_set_visible (GTK_WIDGET (view), TRUE);
	return GTK_WIDGET (view);
}

GtkWidget* ugtk_node_view_new_for_state (void)
{
	GtkListView*        view;
	GtkListItemFactory* factory;

	factory = make_factory (G_CALLBACK (setup_sidebar_row),
	                        G_CALLBACK (bind_state_row));
	view = GTK_LIST_VIEW (gtk_list_view_new (NULL, factory));
	gtk_list_view_set_show_separators (view, FALSE);
	gtk_widget_set_visible (GTK_WIDGET (view), TRUE);
	return GTK_WIDGET (view);
}
