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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <UgUtil.h>
#include <UgtkSettingDialog.h>

#include <glib/gi18n.h>

// Callback
static void on_cursor_changed (GtkTreeView* view, UgtkSettingDialog* sdialog);
static void on_response (GtkDialog *dialog, gint response_id, UgtkSettingDialog* sdialog);

UgtkSettingDialog*  ugtk_setting_dialog_new (const gchar* title, GtkWindow* parent)
{
	PangoContext*  context;
	PangoLayout*   layout;
	int            text_width;
	UgtkSettingDialog*  dialog;
	GtkCellRenderer*    renderer;
	GtkWidget*  widget;
	GtkBox*     vbox;
	GtkBox*     hbox;

	dialog = g_malloc0 (sizeof (UgtkSettingDialog));
	dialog->self = (GtkDialog*) gtk_dialog_new_with_buttons (title, parent,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_OK"),     GTK_RESPONSE_OK,
			NULL);
#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION < 14
	gtk_window_set_has_resize_grip ((GtkWindow*) dialog->self, FALSE);
#endif
	gtk_dialog_set_default_response (dialog->self, GTK_RESPONSE_OK);
	vbox = (GtkBox*) gtk_dialog_get_content_area (dialog->self);
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, GTK_WIDGET (hbox));
	// Notebook
	widget = gtk_notebook_new ();
	gtk_widget_set_size_request (widget, 430, 320);
	gtk_box_append (hbox, widget);
	dialog->notebook = (GtkNotebook*) widget;
	gtk_notebook_set_show_tabs (dialog->notebook, FALSE);
	gtk_notebook_set_show_border (dialog->notebook, FALSE);
	// get text width
	context = gtk_widget_get_pango_context (widget);
	layout = pango_layout_new (context);
	pango_layout_set_text (layout, "User Interface", -1);
	pango_layout_get_pixel_size (layout, &text_width, NULL);
	g_object_unref (layout);
	text_width = text_width * 5 / 3;
	if (text_width < 130)
		text_width = 130;
	// TreeView
	dialog->list_store = gtk_list_store_new (1, G_TYPE_STRING);
	widget = gtk_tree_view_new_with_model (
			GTK_TREE_MODEL (dialog->list_store));
	gtk_widget_set_size_request (widget, text_width, 120);
	gtk_box_prepend (hbox, widget);
	dialog->tree_view = (GtkTreeView*) widget;
	gtk_tree_view_set_headers_visible (dialog->tree_view, FALSE);
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (
			dialog->tree_view, 0,
			_("Name"), renderer,
			"text", 0,
			NULL);
	g_signal_connect (dialog->tree_view, "cursor-changed",
			G_CALLBACK (on_cursor_changed), dialog);

	// ------------------------------------------------------------------------
	// UI settings page
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	g_object_set (vbox, "margin", 2, NULL);
	ugtk_user_interface_form_init (&dialog->ui);
	gtk_box_append (vbox, dialog->ui.self);
	ugtk_setting_dialog_add (dialog, _("User Interface"), (GtkWidget*) vbox);

	// ------------------------------------------------------------------------
	// Clipboard settings page
	ugtk_clipboard_form_init (&dialog->clipboard);
	g_object_set (dialog->clipboard.self, "margin", 2, NULL);
	ugtk_setting_dialog_add (dialog, _("Clipboard"), dialog->clipboard.self);

	// ------------------------------------------------------------------------
	// Bandwidth settings page
	ugtk_bandwidth_form_init (&dialog->bandwidth);
	g_object_set (dialog->bandwidth.self, "margin", 2, NULL);
	ugtk_setting_dialog_add (dialog, _("Bandwidth"), dialog->bandwidth.self);

	// ------------------------------------------------------------------------
	// Scheduler settings page
	ugtk_schedule_form_init (&dialog->scheduler);
	g_object_set (dialog->scheduler.self, "margin", 2, NULL);
	ugtk_setting_dialog_add (dialog, _("Scheduler"), dialog->scheduler.self);

	// ------------------------------------------------------------------------
	// Plugin settings page
	ugtk_plugin_form_init (&dialog->plugin);
	g_object_set (dialog->plugin.self, "margin", 2, NULL);
	ugtk_setting_dialog_add (dialog, _("Plug-in"), dialog->plugin.self);

	// ------------------------------------------------------------------------
	// Plugin Media & Media Website settings page
	ugtk_media_website_form_init (&dialog->media_website);
	g_object_set (dialog->media_website.self, "margin", 2, NULL);
	ugtk_setting_dialog_add (dialog, _("Media website"), dialog->media_website.self);

	// ------------------------------------------------------------------------
	// Others settings page
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	g_object_set (vbox, "margin", 2, NULL);
	ugtk_setting_dialog_add (dialog, _("Others"), (GtkWidget*) vbox);

	ugtk_commandline_form_init (&dialog->commandline);
	gtk_box_append (vbox, dialog->commandline.self);
	gtk_box_append (vbox, gtk_label_new (""));
	ugtk_auto_save_form_init (&dialog->auto_save);
	gtk_box_append (vbox, dialog->auto_save.self);
	gtk_box_append (vbox, gtk_label_new (""));
	ugtk_completion_form_init (&dialog->completion);
	gtk_box_append (vbox, dialog->completion.self);

	gtk_widget_set_visible ((GtkWidget*) hbox, TRUE);
//	gtk_container_set_focus_child (GTK_CONTAINER (dialog->self), dialog->pattern_entry);
//	g_signal_connect (dialog->pattern_entry, "key-press-event", G_CALLBACK (on_key_press_event), dialog);

	return dialog;
}

void  ugtk_setting_dialog_free (UgtkSettingDialog* dialog)
{
	gtk_window_destroy (GTK_WINDOW (dialog->self));
	g_free (dialog);
}

void  ugtk_setting_dialog_run (UgtkSettingDialog* dialog, UgtkApp* app)
{
	dialog->app = app;
	g_signal_connect (dialog->self, "response",
			G_CALLBACK (on_response), dialog);
	gtk_widget_show ((GtkWidget*) dialog->self);
}

void  ugtk_setting_dialog_set (UgtkSettingDialog* dialog, UgtkSetting* setting)
{
	ugtk_schedule_form_set (&dialog->scheduler, setting);
	ugtk_clipboard_form_set (&dialog->clipboard, setting);
	ugtk_bandwidth_form_set (&dialog->bandwidth, setting);
	ugtk_user_interface_form_set (&dialog->ui, setting);
	ugtk_completion_form_set (&dialog->completion, setting);
	ugtk_auto_save_form_set (&dialog->auto_save, setting);
	ugtk_commandline_form_set (&dialog->commandline, setting);
	ugtk_plugin_form_set (&dialog->plugin, setting);
	ugtk_media_website_form_set (&dialog->media_website, setting);
}

void  ugtk_setting_dialog_get (UgtkSettingDialog* dialog, UgtkSetting* setting)
{
	ugtk_schedule_form_get (&dialog->scheduler, setting);
	ugtk_clipboard_form_get (&dialog->clipboard, setting);
	ugtk_bandwidth_form_get (&dialog->bandwidth, setting);
	ugtk_user_interface_form_get (&dialog->ui, setting);
	ugtk_completion_form_get (&dialog->completion, setting);
	ugtk_auto_save_form_get (&dialog->auto_save, setting);
	ugtk_commandline_form_get (&dialog->commandline, setting);
	ugtk_plugin_form_get (&dialog->plugin, setting);
	ugtk_media_website_form_get (&dialog->media_website, setting);
}

void  ugtk_setting_dialog_add (UgtkSettingDialog* sdialog,
                               const gchar* title,
                               GtkWidget*   page)
{
	GtkTreeIter  iter;

	gtk_list_store_append (sdialog->list_store, &iter);
	gtk_list_store_set (sdialog->list_store, &iter, 0, title, -1);
	gtk_notebook_append_page (sdialog->notebook, page, gtk_label_new (title));
}

void  ugtk_setting_dialog_set_page (UgtkSettingDialog* sdialog, int nth)
{
	GtkTreePath* path;

	path = gtk_tree_path_new_from_indices (nth, -1);
	gtk_tree_view_set_cursor (sdialog->tree_view, path, NULL, FALSE);
	gtk_tree_path_free (path);
	gtk_notebook_set_current_page (sdialog->notebook, nth);
}

// ----------------------------------------------------------------------------
// Callback

static void on_cursor_changed (GtkTreeView* view, UgtkSettingDialog* sdialog)
{
	GtkTreePath*   path;
	int            nth;

	gtk_tree_view_get_cursor (view, &path, NULL);
	if (path == NULL)
		return;
	nth = *gtk_tree_path_get_indices (path);
	gtk_tree_path_free (path);
	gtk_notebook_set_current_page (sdialog->notebook, nth);
}

static void on_response (GtkDialog *dialog, gint response_id, UgtkSettingDialog* sdialog)
{
	UgtkApp*  app;

	app = sdialog->app;
	if (app)
		app->dialogs.setting = NULL;

	if (response_id == GTK_RESPONSE_OK) {
		ugtk_setting_dialog_get (sdialog, &app->setting);
		ugtk_app_set_ui_setting (app, &app->setting);
		ugtk_app_set_menu_setting (app, &app->setting);
		ugtk_app_set_other_setting (app, &app->setting);
		ugtk_app_set_plugin_setting (app, &app->setting);
	}
	ugtk_setting_dialog_free (sdialog);
}
