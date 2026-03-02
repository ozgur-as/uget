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

static inline void set_margin_all(GtkWidget* w, int m) {
	gtk_widget_set_margin_start(w, m);
	gtk_widget_set_margin_end(w, m);
	gtk_widget_set_margin_top(w, m);
	gtk_widget_set_margin_bottom(w, m);
}

// Callback
static void on_selection_changed (GtkSingleSelection* sel, GParamSpec* pspec, UgtkSettingDialog* sdialog);
static void on_setting_ok (GtkWidget* button, UgtkSettingDialog* sdialog);
static void on_setting_cancel (GtkWidget* button, UgtkSettingDialog* sdialog);
static gboolean on_setting_close_request (GtkWindow* window, UgtkSettingDialog* sdialog);

// Factory callbacks for sidebar list
static void sidebar_setup_cb (GtkSignalListItemFactory* factory,
                               GtkListItem* list_item, gpointer user_data)
{
	GtkWidget* label = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_widget_set_margin_start (label, 6);
	gtk_widget_set_margin_end (label, 6);
	gtk_widget_set_margin_top (label, 4);
	gtk_widget_set_margin_bottom (label, 4);
	gtk_list_item_set_child (list_item, label);
}

static void sidebar_bind_cb (GtkSignalListItemFactory* factory,
                              GtkListItem* list_item, gpointer user_data)
{
	GtkStringObject* obj = gtk_list_item_get_item (list_item);
	GtkWidget* label = gtk_list_item_get_child (list_item);
	gtk_label_set_text (GTK_LABEL (label), gtk_string_object_get_string (obj));
}

UgtkSettingDialog*  ugtk_setting_dialog_new (const gchar* title, GtkWindow* parent)
{
	PangoContext*  context;
	PangoLayout*   layout;
	int            text_width;
	UgtkSettingDialog*  dialog;
	GtkWidget*  widget;
	GtkBox*     vbox;
	GtkBox*     hbox;

	GtkBox*     button_box;
	GtkWidget*  cancel_button;
	GtkWidget*  ok_button;
	GtkSignalListItemFactory* factory;

	dialog = g_malloc0 (sizeof (UgtkSettingDialog));
	dialog->self = (GtkWindow*) gtk_window_new ();
	gtk_window_set_title (dialog->self, title);
	gtk_window_set_transient_for (dialog->self, parent);
	gtk_window_set_destroy_with_parent (dialog->self, TRUE);
	gtk_window_set_modal (dialog->self, TRUE);

	// main vertical layout
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	gtk_window_set_child (dialog->self, (GtkWidget*) vbox);
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_widget_set_vexpand ((GtkWidget*) hbox, TRUE);
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
	// Sidebar ListView
	dialog->string_list = gtk_string_list_new (NULL);
	dialog->selection = gtk_single_selection_new (G_LIST_MODEL (dialog->string_list));
	factory = GTK_SIGNAL_LIST_ITEM_FACTORY (gtk_signal_list_item_factory_new ());
	g_signal_connect (factory, "setup", G_CALLBACK (sidebar_setup_cb), NULL);
	g_signal_connect (factory, "bind", G_CALLBACK (sidebar_bind_cb), NULL);
	dialog->list_view = GTK_LIST_VIEW (gtk_list_view_new (
			GTK_SELECTION_MODEL (dialog->selection),
			GTK_LIST_ITEM_FACTORY (factory)));
	widget = GTK_WIDGET (dialog->list_view);
	gtk_widget_set_size_request (widget, text_width, 120);
	gtk_box_prepend (hbox, widget);
	g_signal_connect (dialog->selection, "notify::selected",
			G_CALLBACK (on_selection_changed), dialog);

	// ------------------------------------------------------------------------
	// UI settings page
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	set_margin_all ((GtkWidget*) vbox, 2);
	ugtk_user_interface_form_init (&dialog->ui);
	gtk_box_append (vbox, dialog->ui.self);
	ugtk_setting_dialog_add (dialog, _("User Interface"), (GtkWidget*) vbox);

	// ------------------------------------------------------------------------
	// Clipboard settings page
	ugtk_clipboard_form_init (&dialog->clipboard);
	set_margin_all (dialog->clipboard.self, 2);
	ugtk_setting_dialog_add (dialog, _("Clipboard"), dialog->clipboard.self);

	// ------------------------------------------------------------------------
	// Bandwidth settings page
	ugtk_bandwidth_form_init (&dialog->bandwidth);
	set_margin_all (dialog->bandwidth.self, 2);
	ugtk_setting_dialog_add (dialog, _("Bandwidth"), dialog->bandwidth.self);

	// ------------------------------------------------------------------------
	// Scheduler settings page
	ugtk_schedule_form_init (&dialog->scheduler);
	set_margin_all (dialog->scheduler.self, 2);
	ugtk_setting_dialog_add (dialog, _("Scheduler"), dialog->scheduler.self);

	// ------------------------------------------------------------------------
	// Plugin settings page
	ugtk_plugin_form_init (&dialog->plugin);
	set_margin_all (dialog->plugin.self, 2);
	ugtk_setting_dialog_add (dialog, _("Plug-in"), dialog->plugin.self);

	// ------------------------------------------------------------------------
	// Plugin Media & Media Website settings page
	ugtk_media_website_form_init (&dialog->media_website);
	set_margin_all (dialog->media_website.self, 2);
	ugtk_setting_dialog_add (dialog, _("Media website"), dialog->media_website.self);

	// ------------------------------------------------------------------------
	// Others settings page
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	set_margin_all ((GtkWidget*) vbox, 2);
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

	// button box at bottom (use the main vbox, not the per-page one)
	vbox = (GtkBox*) gtk_widget_get_parent ((GtkWidget*) hbox);
	button_box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_halign ((GtkWidget*) button_box, GTK_ALIGN_END);
	gtk_widget_set_margin_top ((GtkWidget*) button_box, 6);
	gtk_widget_set_margin_bottom ((GtkWidget*) button_box, 6);
	gtk_widget_set_margin_end ((GtkWidget*) button_box, 6);
	gtk_box_append (vbox, (GtkWidget*) button_box);
	cancel_button = gtk_button_new_with_mnemonic (_("_Cancel"));
	gtk_box_append (button_box, cancel_button);
	ok_button = gtk_button_new_with_mnemonic (_("_OK"));
	gtk_widget_add_css_class (ok_button, "suggested-action");
	gtk_box_append (button_box, ok_button);
	g_signal_connect (ok_button, "clicked",
			G_CALLBACK (on_setting_ok), dialog);
	g_signal_connect (cancel_button, "clicked",
			G_CALLBACK (on_setting_cancel), dialog);
	g_signal_connect (dialog->self, "close-request",
			G_CALLBACK (on_setting_close_request), dialog);

	return dialog;
}

void  ugtk_setting_dialog_free (UgtkSettingDialog* dialog)
{
	gtk_window_destroy (dialog->self);
	g_free (dialog);
}

void  ugtk_setting_dialog_run (UgtkSettingDialog* dialog, UgtkApp* app)
{
	dialog->app = app;
	gtk_widget_set_visible((GtkWidget*) dialog->self, TRUE);
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
	gtk_string_list_append (sdialog->string_list, title);
	gtk_notebook_append_page (sdialog->notebook, page, gtk_label_new (title));
}

void  ugtk_setting_dialog_set_page (UgtkSettingDialog* sdialog, int nth)
{
	gtk_single_selection_set_selected (sdialog->selection, nth);
	gtk_notebook_set_current_page (sdialog->notebook, nth);
}

// ----------------------------------------------------------------------------
// Callback

static void on_selection_changed (GtkSingleSelection* sel, GParamSpec* pspec, UgtkSettingDialog* sdialog)
{
	guint nth = gtk_single_selection_get_selected (sel);
	if (nth != GTK_INVALID_LIST_POSITION)
		gtk_notebook_set_current_page (sdialog->notebook, nth);
}

static void on_setting_ok (GtkWidget* button, UgtkSettingDialog* sdialog)
{
	UgtkApp*  app;

	app = sdialog->app;
	if (app)
		app->dialogs.setting = NULL;

	ugtk_setting_dialog_get (sdialog, &app->setting);
	ugtk_app_set_ui_setting (app, &app->setting);
	ugtk_app_set_menu_setting (app, &app->setting);
	ugtk_app_set_other_setting (app, &app->setting);
	ugtk_app_set_plugin_setting (app, &app->setting);
	ugtk_setting_dialog_free (sdialog);
}

static void on_setting_cancel (GtkWidget* button, UgtkSettingDialog* sdialog)
{
	if (sdialog->app)
		sdialog->app->dialogs.setting = NULL;
	ugtk_setting_dialog_free (sdialog);
}

static gboolean on_setting_close_request (GtkWindow* window, UgtkSettingDialog* sdialog)
{
	on_setting_cancel (NULL, sdialog);
	return TRUE;
}
