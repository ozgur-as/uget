/*
 *
 *   Copyright (C) 2005-2020 by C.H. Huang
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

#include <UgetNode.h>
#include <UgetData.h>
#include <UgtkSummary.h>

#include <glib/gi18n.h>

static void ugtk_summary_add_row (UgtkSummary* summary,
                                   const gchar* icon_name,
                                   const gchar* name,
                                   const gchar* value);
static void ugtk_summary_clear (UgtkSummary* summary);

void  ugtk_summary_init (UgtkSummary* summary, gpointer accel_group)
{
	GtkScrolledWindow*	scroll;

	summary->list_box = GTK_LIST_BOX (gtk_list_box_new ());
	gtk_list_box_set_selection_mode (summary->list_box, GTK_SELECTION_SINGLE);

	summary->self = gtk_scrolled_window_new ();
	scroll = GTK_SCROLLED_WINDOW (summary->self);
	gtk_scrolled_window_set_policy (scroll,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (scroll, GTK_WIDGET (summary->list_box));
	gtk_widget_set_size_request (summary->self, 200, 90);

	// visible
	summary->visible.name = 1;
	summary->visible.folder = 1;
	summary->visible.category = 0;
	summary->visible.uri = 0;
	summary->visible.message = 1;
}

void  ugtk_summary_show (UgtkSummary* summary, UgetNode* node)
{
	gchar*        name;
	gchar*        value;
	gchar*        stock;
	union {
		UgetLog*      log;
		UgetEvent*    event;
		UgetCommon*   common;
	} temp;

	ugtk_summary_clear (summary);

	if (node == NULL)
		return;

	temp.common = ug_info_get (node->info, UgetCommonInfo);

	// Summary Name
	if (summary->visible.name) {
		if (temp.common && temp.common->name) {
			name = g_strconcat (_("Name"), ":", NULL);
			value = temp.common->name;
		}
		else {
			name = g_strconcat (_("File"), ":", NULL);
			value = (temp.common) ? temp.common->file : NULL;
			if (value == NULL)
				value = _("unnamed");
		}
		ugtk_summary_add_row (summary, "text-x-generic", name, value);
		g_free (name);
	}
	// Summary Folder
	if (summary->visible.folder) {
		name = g_strconcat (_("Folder"), ":", NULL);
		value = (temp.common) ? temp.common->folder : NULL;
		ugtk_summary_add_row (summary, "folder", name, value);
		g_free (name);
	}
	// Summary Category
	if (summary->visible.category) {
		name = g_strconcat (_("Category"), ":", NULL);
		if (node->parent) {
			temp.common = ug_info_get (node->parent->info, UgetCommonInfo);
			value = (temp.common) ? temp.common->name : NULL;
			temp.common = ug_info_get (node->info, UgetCommonInfo);
		}
		else
			value = NULL;
		ugtk_summary_add_row (summary, "view-list-symbolic", name, value);
		g_free (name);
	}
	// Summary URL
	if (summary->visible.uri) {
		name = g_strconcat (_("URI"), ":", NULL);
		value = (temp.common) ? temp.common->uri : NULL;
		ugtk_summary_add_row (summary, "network-workgroup", name, value);
		g_free (name);
	}
	// Summary Message
	temp.log = ug_info_get (node->info, UgetLogInfo);
	if (temp.log)
		temp.event = (UgetEvent*) temp.log->messages.head;
	if (summary->visible.message) {
		if (temp.event == NULL) {
			stock = "dialog-information";
			value = NULL;
		}
		else {
			value = temp.event->string;
			switch (temp.event->type) {
			case UGET_EVENT_ERROR:
				stock = "dialog-error";
				break;
			case UGET_EVENT_WARNING:
				stock = "dialog-warning";
				break;
			default:
				stock = "dialog-information";
				break;
			}
		}
		name = g_strconcat (_("Message"), ":", NULL);
		ugtk_summary_add_row (summary, stock, name, value);
		g_free (name);
	}
}

gchar*  ugtk_summary_get_text_selected (UgtkSummary* summary)
{
	GtkListBoxRow* row;
	GtkWidget*     box;
	GtkWidget*     child;
	const gchar*   name = NULL;
	const gchar*   value = NULL;

	row = gtk_list_box_get_selected_row (summary->list_box);
	if (row == NULL)
		return NULL;

	box = gtk_list_box_row_get_child (row);
	// children: image, name_label, value_label
	child = gtk_widget_get_first_child (box);
	if (child) child = gtk_widget_get_next_sibling (child);  // skip icon
	if (child) {
		name = gtk_label_get_text (GTK_LABEL (child));
		child = gtk_widget_get_next_sibling (child);
	}
	if (child)
		value = gtk_label_get_text (GTK_LABEL (child));

	return g_strconcat (name ? name : "", " ", value ? value : "", NULL);
}

gchar*  ugtk_summary_get_text_all (UgtkSummary* summary)
{
	GString*       gstr;
	GtkWidget*     row;
	GtkWidget*     box;
	GtkWidget*     child;
	const gchar*   name;
	const gchar*   value;
	int            index;

	gstr = g_string_sized_new (60);
	for (index = 0; ; index++) {
		row = GTK_WIDGET (gtk_list_box_get_row_at_index (summary->list_box, index));
		if (row == NULL)
			break;
		box = gtk_list_box_row_get_child (GTK_LIST_BOX_ROW (row));
		child = gtk_widget_get_first_child (box);
		if (child) child = gtk_widget_get_next_sibling (child);  // skip icon
		name = NULL;
		value = NULL;
		if (child) {
			name = gtk_label_get_text (GTK_LABEL (child));
			child = gtk_widget_get_next_sibling (child);
		}
		if (child)
			value = gtk_label_get_text (GTK_LABEL (child));
		if (name)
			g_string_append (gstr, name);
		if (value) {
			g_string_append_c (gstr, ' ');
			g_string_append (gstr, value);
		}
		g_string_append_c (gstr, '\n');
	}
	return g_string_free (gstr, FALSE);
}

// ----------------------------------------------------------------------------
// Static functions

static void ugtk_summary_clear (UgtkSummary* summary)
{
	GtkListBoxRow* row;

	while ((row = gtk_list_box_get_row_at_index (summary->list_box, 0)) != NULL)
		gtk_list_box_remove (summary->list_box, GTK_WIDGET (row));
}

static void ugtk_summary_add_row (UgtkSummary* summary,
                                   const gchar* icon_name,
                                   const gchar* name,
                                   const gchar* value)
{
	GtkWidget*  box;
	GtkWidget*  image;
	GtkWidget*  name_label;
	GtkWidget*  value_label;

	box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	image = gtk_image_new_from_icon_name (icon_name);
	gtk_box_append (GTK_BOX (box), image);

	name_label = gtk_label_new (name);
	gtk_label_set_xalign (GTK_LABEL (name_label), 0.0);
	gtk_box_append (GTK_BOX (box), name_label);

	value_label = gtk_label_new (value);
	gtk_label_set_xalign (GTK_LABEL (value_label), 0.0);
	gtk_label_set_ellipsize (GTK_LABEL (value_label), PANGO_ELLIPSIZE_END);
	gtk_widget_set_hexpand (value_label, TRUE);
	gtk_box_append (GTK_BOX (box), value_label);

	gtk_list_box_append (summary->list_box, box);
}
