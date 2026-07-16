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

#include <string.h>
#include <UgString.h>
#include <UgetResolver.h>
#include <UgtkSettingForm.h>

#include <glib/gi18n.h>

static inline void set_margin_all(GtkWidget* w, int m) {
	gtk_widget_set_margin_start(w, m);
	gtk_widget_set_margin_end(w, m);
	gtk_widget_set_margin_top(w, m);
	gtk_widget_set_margin_bottom(w, m);
}

// ----------------------------------------------------------------------------
// UgtkClipboardForm
//
//
void  ugtk_clipboard_form_init (struct UgtkClipboardForm* cbform)
{
	PangoContext*  context;
	PangoLayout*   layout;
	int            text_height;
	GtkTextView* textview;
	GtkWidget*   widget;
	GtkBox*      vbox;
	GtkBox*      hbox;
	GtkScrolledWindow*  scroll;

	cbform->self = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	vbox = (GtkBox*) cbform->self;
	// Monitor button
	widget = gtk_check_button_new_with_mnemonic (_("_Enable clipboard monitor"));
	gtk_box_append (vbox, widget);
	cbform->monitor = (GtkCheckButton*) widget;

	// quiet mode
	widget = gtk_check_button_new_with_mnemonic (_("_Quiet mode"));
	gtk_box_append (vbox, widget);
	cbform->quiet = (GtkCheckButton*) widget;
	// Nth category
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Default category index"));
	gtk_box_append (hbox, widget);
	cbform->nth_label = widget;
	widget = gtk_spin_button_new_with_range (0.0, 1000.0, 1.0);
	gtk_spin_button_set_activates_default (GTK_SPIN_BUTTON (widget), TRUE);
	gtk_box_append (hbox, widget);
	cbform->nth_spin = (GtkSpinButton*) widget;
	// hint
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (" - ");
	gtk_box_append (hbox, widget);
	widget = gtk_label_new (_("Adding to Nth category if no matched category."));
	gtk_box_append (hbox, widget);

	gtk_box_append (vbox, gtk_label_new (""));

	widget = gtk_check_button_new_with_mnemonic (_("_Monitor URL of website"));
	gtk_box_append (vbox, widget);
	cbform->website = (GtkCheckButton*) widget;

	gtk_box_append (vbox, gtk_label_new (""));

	// get text height --- begin ---
	context = gtk_widget_get_pango_context (widget);
	layout = pango_layout_new (context);
	pango_layout_set_text (layout, "joIN", -1);
	pango_layout_get_pixel_size (layout, NULL, &text_height);
	g_object_unref (layout);
	//  get text height --- end ---
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Monitor clipboard for specified file types:"));
	gtk_box_append (hbox, widget);
	// Scrolled Window
	scroll = (GtkScrolledWindow*) gtk_scrolled_window_new ();
	gtk_widget_set_size_request (GTK_WIDGET (scroll), 100, text_height * 6);
	gtk_box_append (vbox, GTK_WIDGET (scroll));
	// file type pattern : TextView
	cbform->buffer = gtk_text_buffer_new (NULL);
	cbform->pattern = gtk_text_view_new_with_buffer (cbform->buffer);
	g_object_unref (cbform->buffer);
	textview = (GtkTextView*) cbform->pattern;
	gtk_text_view_set_wrap_mode (textview, GTK_WRAP_WORD_CHAR);
	gtk_scrolled_window_set_child (scroll, GTK_WIDGET (cbform->pattern));

	// tips
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	gtk_box_append (hbox,
			gtk_label_new (_("Separate the types with character '|'.")));
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	gtk_box_append (hbox,
			gtk_label_new (_("You can use regular expressions here.")));
}

void  ugtk_clipboard_form_set (struct UgtkClipboardForm* cbform, UgtkSetting* setting)
{
	if (setting->clipboard.pattern)
		gtk_text_buffer_set_text (cbform->buffer, setting->clipboard.pattern, -1);
	gtk_check_button_set_active (cbform->monitor, setting->clipboard.monitor);
	gtk_check_button_set_active (cbform->quiet, setting->clipboard.quiet);
	gtk_check_button_set_active (cbform->website, setting->clipboard.website);
	gtk_spin_button_set_value (cbform->nth_spin, setting->clipboard.nth_category);
	g_signal_emit_by_name (cbform->monitor, "toggled");
	g_signal_emit_by_name (cbform->quiet, "toggled");
//	on_clipboard_quiet_mode_toggled ((GtkWidget*) cbform->quiet, cbform);
}

void  ugtk_clipboard_form_get (struct UgtkClipboardForm* cbform, UgtkSetting* setting)
{
	GtkTextIter  iter1;
	GtkTextIter  iter2;

	gtk_text_buffer_get_start_iter (cbform->buffer, &iter1);
	gtk_text_buffer_get_end_iter (cbform->buffer, &iter2);

	ug_free (setting->clipboard.pattern);
	setting->clipboard.pattern = gtk_text_buffer_get_text (cbform->buffer, &iter1, &iter2, FALSE);
	setting->clipboard.monitor = gtk_check_button_get_active (cbform->monitor);
	setting->clipboard.quiet = gtk_check_button_get_active (cbform->quiet);
	setting->clipboard.website = gtk_check_button_get_active (cbform->website);
	setting->clipboard.nth_category = gtk_spin_button_get_value_as_int (cbform->nth_spin);
	// remove line break
	ug_str_remove_crlf (setting->clipboard.pattern, setting->clipboard.pattern);
}

// ----------------------------------------------------------------------------
// UgtkUserInterfaceForm
//
void  ugtk_user_interface_form_init (struct UgtkUserInterfaceForm* uiform)
{
	GtkWidget*  widget;
	GtkBox*     vbox;

	uiform->self = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	// Confirmation
	vbox = (GtkBox*) uiform->self;
	widget = gtk_frame_new (_("Confirmation"));
	gtk_box_append (vbox, widget);
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_frame_set_child (GTK_FRAME (widget), (GtkWidget*) vbox);
	set_margin_all ((GtkWidget*) vbox, 2);
	// Confirmation check buttons
	widget = gtk_check_button_new_with_label (_("Show confirmation dialog on exit"));
	uiform->confirm_exit = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);
	widget = gtk_check_button_new_with_label (_("Confirm when deleting files"));
	uiform->confirm_delete = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);

	// System Tray
	vbox = (GtkBox*) uiform->self;
	widget = gtk_frame_new (_("System Tray"));
	gtk_box_append (vbox, widget);
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_frame_set_child (GTK_FRAME (widget), (GtkWidget*) vbox);
	set_margin_all ((GtkWidget*) vbox, 2);
	// System Tray check buttons
	widget = gtk_check_button_new_with_label (_("Always show tray icon"));
	uiform->show_trayicon = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);
	widget = gtk_check_button_new_with_label (_("Minimize to tray on startup"));
	uiform->start_in_tray = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);
	widget = gtk_check_button_new_with_label (_("Close to tray on window close"));
	uiform->close_to_tray = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);

	// Others
	vbox = (GtkBox*) uiform->self;
	widget = gtk_check_button_new_with_label (_("Enable offline mode on startup"));
	uiform->start_in_offline_mode = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);
	widget = gtk_check_button_new_with_label (_("Download starting notification"));
	uiform->start_notification = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);
	widget = gtk_check_button_new_with_label (_("Sound when download is finished"));
	uiform->sound_notification = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);
	widget = gtk_check_button_new_with_label (_("Apply recent download settings"));
	uiform->apply_recent = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);
//	widget = gtk_check_button_new_with_label (_("Skip existing URI from clipboard and command-line"));
//	uiform->skip_existing = (GtkCheckButton*) widget;
//	gtk_box_append (vbox, widget);
	widget = gtk_check_button_new_with_label (_("Display large icon"));
	uiform->large_icon = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);
}

void  ugtk_user_interface_form_set (struct UgtkUserInterfaceForm* uiform, UgtkSetting* setting)
{
	gtk_check_button_set_active (uiform->close_to_tray,
			setting->ui.close_to_tray);
	gtk_check_button_set_active (uiform->confirm_exit,
			setting->ui.exit_confirmation);
	gtk_check_button_set_active (uiform->confirm_delete,
			setting->ui.delete_confirmation);
	gtk_check_button_set_active (uiform->show_trayicon,
			setting->ui.show_trayicon);
	gtk_check_button_set_active (uiform->start_in_tray,
			setting->ui.start_in_tray);
	gtk_check_button_set_active (uiform->start_in_offline_mode,
			setting->ui.start_in_offline_mode);
	gtk_check_button_set_active (uiform->start_notification,
			setting->ui.start_notification);
	gtk_check_button_set_active (uiform->sound_notification,
			setting->ui.sound_notification);
	gtk_check_button_set_active (uiform->apply_recent,
			setting->ui.apply_recent);
//	gtk_check_button_set_active (uiform->skip_existing,
//			setting->ui.skip_existing);
	gtk_check_button_set_active (uiform->large_icon,
			setting->ui.large_icon);
}

void  ugtk_user_interface_form_get (struct UgtkUserInterfaceForm* uiform, UgtkSetting* setting)
{
	setting->ui.close_to_tray = gtk_check_button_get_active (uiform->close_to_tray);
	setting->ui.exit_confirmation = gtk_check_button_get_active (uiform->confirm_exit);
	setting->ui.delete_confirmation = gtk_check_button_get_active (uiform->confirm_delete);
	setting->ui.show_trayicon = gtk_check_button_get_active (uiform->show_trayicon);
	setting->ui.start_in_tray = gtk_check_button_get_active (uiform->start_in_tray);
	setting->ui.start_in_offline_mode = gtk_check_button_get_active (uiform->start_in_offline_mode);
	setting->ui.start_notification = gtk_check_button_get_active (uiform->start_notification);
	setting->ui.sound_notification = gtk_check_button_get_active (uiform->sound_notification);
	setting->ui.apply_recent = gtk_check_button_get_active (uiform->apply_recent);
//	setting->ui.skip_existing = gtk_check_button_get_active (uiform->skip_existing);
	setting->ui.large_icon = gtk_check_button_get_active (uiform->large_icon);
}

// ----------------------------------------------------------------------------
// UgtkBandwidthForm
//

void  ugtk_bandwidth_form_init (struct UgtkBandwidthForm* bwform)
{
	GtkWidget*  widget;
	GtkBox*     box;
	GtkBox*     vbox;
	GtkBox*     hbox;

	box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	bwform->self = (GtkWidget*) box;
	widget = gtk_label_new (_("These will affect all plug-ins."));
	gtk_box_append (box, widget);
	widget = gtk_label_new ("");
	gtk_box_append (box, widget);

	// Global speed limit
	widget = gtk_frame_new (_("Global speed limit"));
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_frame_set_child (GTK_FRAME (widget), (GtkWidget*) vbox);
	set_margin_all ((GtkWidget*) vbox, 2);
	gtk_box_append (box, widget);
	// Max upload speed
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Max upload speed"));
	gtk_box_append (hbox, widget);
	
	widget = gtk_spin_button_new_with_range (0.0, 4000000000.0, 5.0);
	gtk_editable_set_width_chars (GTK_EDITABLE (widget), 15);
	gtk_spin_button_set_activates_default (GTK_SPIN_BUTTON (widget), TRUE);
	gtk_box_append (hbox, widget);
	bwform->upload = (GtkSpinButton*) widget;

	widget = gtk_label_new (_("KiB/s"));
	gtk_box_append (hbox, widget);

	// Max download speed
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Max download speed"));
	gtk_box_append (hbox, widget);
	
	widget = gtk_spin_button_new_with_range (0.0, 4000000000.0, 5.0);
	gtk_editable_set_width_chars (GTK_EDITABLE (widget), 15);
	gtk_spin_button_set_activates_default (GTK_SPIN_BUTTON (widget), TRUE);
	gtk_box_append (hbox, widget);
	bwform->download = (GtkSpinButton*) widget;

	widget = gtk_label_new (_("KiB/s"));
	gtk_box_append (hbox, widget);
}

void  ugtk_bandwidth_form_set (struct UgtkBandwidthForm* bwform, UgtkSetting* setting)
{
	gtk_spin_button_set_value (bwform->upload, setting->bandwidth.normal.upload);
	gtk_spin_button_set_value (bwform->download, setting->bandwidth.normal.download);
}

void  ugtk_bandwidth_form_get (struct UgtkBandwidthForm* bwform, UgtkSetting* setting)
{
	setting->bandwidth.normal.upload   = (guint) gtk_spin_button_get_value (bwform->upload);
	setting->bandwidth.normal.download = (guint) gtk_spin_button_get_value (bwform->download);
}

// ----------------------------------------------------------------------------
// UgtkCompletionForm
//
void  ugtk_completion_form_init (struct UgtkCompletionForm* csform)
{
	GtkWidget*  widget;
	GtkWidget*  entry;
	GtkBox*     vbox;
	GtkBox*     hbox;

	widget = gtk_frame_new (_("Completion Auto-Actions"));
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_frame_set_child (GTK_FRAME (widget), (GtkWidget*) vbox);
	set_margin_all ((GtkWidget*) vbox, 2);
	csform->self = widget;

	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Custom command:"));
	gtk_box_append (hbox, widget);

	entry = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	gtk_widget_set_hexpand (entry, TRUE);
	gtk_box_append (vbox, entry);
	csform->command = (GtkEntry*) entry;

	gtk_box_append (vbox, gtk_label_new (""));

	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Custom command if error occurred:"));
	gtk_box_append (hbox, widget);

	entry = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (entry), TRUE);
	gtk_widget_set_hexpand (entry, TRUE);
	gtk_box_append (vbox, entry);
	csform->on_error = (GtkEntry*) entry;
}

void  ugtk_completion_form_set (struct UgtkCompletionForm* csform, UgtkSetting* setting)
{
	if (setting->completion.command)
		gtk_editable_set_text (GTK_EDITABLE (csform->command), setting->completion.command);
	if (setting->completion.on_error)
		gtk_editable_set_text (GTK_EDITABLE (csform->on_error), setting->completion.on_error);
}

void  ugtk_completion_form_get (struct UgtkCompletionForm* csform, UgtkSetting* setting)
{
	const char*  string;

	ug_free (setting->completion.command);
	string = gtk_editable_get_text (GTK_EDITABLE (csform->command));
	setting->completion.command = (string[0]) ? ug_strdup (string) : NULL;

	ug_free (setting->completion.on_error);
	string = gtk_editable_get_text (GTK_EDITABLE (csform->on_error));
	setting->completion.on_error = (string[0]) ? ug_strdup (string) : NULL;
}

// ----------------------------------------------------------------------------
// UgtkAutoSaveForm
//
static void on_auto_save_toggled (GtkWidget* widget, struct UgtkAutoSaveForm* asform)
{
	gboolean  sensitive;

	sensitive = gtk_check_button_get_active (asform->enable);
	gtk_widget_set_sensitive (asform->interval_label, sensitive);
	gtk_widget_set_sensitive ((GtkWidget*) asform->interval_spin, sensitive);
	gtk_widget_set_sensitive (asform->minutes_label, sensitive);
}

void  ugtk_auto_save_form_init (struct UgtkAutoSaveForm* asform)
{
	GtkBox*    hbox;
	GtkWidget* widget;

	asform->self = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	hbox = (GtkBox*) asform->self;
	widget = gtk_check_button_new_with_mnemonic (_("_Autosave"));
	gtk_box_append (hbox, widget);
	g_signal_connect (widget, "toggled",
			G_CALLBACK (on_auto_save_toggled), asform);
	asform->enable = (GtkCheckButton*) widget;

	// space
	widget = gtk_label_new ("");
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_box_append (hbox, widget);

	// auto save interval label
	widget = gtk_label_new_with_mnemonic (_("_Interval:"));
	gtk_box_append (hbox, widget);
	asform->interval_label = widget;
	gtk_label_set_mnemonic_widget (GTK_LABEL (asform->interval_label),
			(GtkWidget*) asform->interval_spin);
	// auto save interval spin
	widget = gtk_spin_button_new_with_range (1.0, 120.0, 1.0);
	gtk_spin_button_set_activates_default (GTK_SPIN_BUTTON (widget), TRUE);
	gtk_box_append (hbox, widget);
	asform->interval_spin = (GtkSpinButton*) widget;
	// auto save interval unit label
	widget = gtk_label_new_with_mnemonic (_("minutes"));
	gtk_box_append (hbox, widget);
	asform->minutes_label = widget;
}

void  ugtk_auto_save_form_set (struct UgtkAutoSaveForm* asform, UgtkSetting* setting)
{
	gtk_check_button_set_active (asform->enable, setting->auto_save.enable);
	gtk_spin_button_set_value (asform->interval_spin, (gdouble) setting->auto_save.interval);
	g_signal_emit_by_name (asform->enable, "toggled");
//	on_auto_save_toggled ((GtkWidget*) asform->enable, asform);
}

void  ugtk_auto_save_form_get (struct UgtkAutoSaveForm* asform, UgtkSetting* setting)
{
	setting->auto_save.enable = gtk_check_button_get_active (asform->enable);
	setting->auto_save.interval = gtk_spin_button_get_value_as_int (asform->interval_spin);
}

// ----------------------------------------------------------------------------
// UgtkCommandlineForm
//
void  ugtk_commandline_form_init (struct UgtkCommandlineForm* clform)
{
	GtkWidget*  widget;
	GtkBox*     vbox;
	GtkBox*     hbox;

	// Commandline Settings
	widget = gtk_frame_new (_("Commandline Settings"));
	clform->self = widget;
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_frame_set_child (GTK_FRAME (widget), (GtkWidget*) vbox);
	set_margin_all ((GtkWidget*) vbox, 2);

	// --quiet
	widget = gtk_check_button_new_with_mnemonic (_("Use '--quiet' by default"));
	gtk_box_append (vbox, widget);
	clform->quiet = (GtkCheckButton*) widget;
	// --category-index
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Default category index"));
	gtk_box_append (hbox, widget);
	clform->index_label = widget;
	widget = gtk_spin_button_new_with_range (0.0, 1000.0, 1.0);
	gtk_spin_button_set_activates_default (GTK_SPIN_BUTTON (widget), TRUE);
	gtk_box_append (hbox, widget);
	clform->index_spin = (GtkSpinButton*) widget;
	// hint
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (" - ");
	gtk_box_append (hbox, widget);
	widget = gtk_label_new (_("Adding to Nth category if no matched category."));
	gtk_box_append (hbox, widget);
}

void  ugtk_commandline_form_set (struct UgtkCommandlineForm* clform, UgtkSetting* setting)
{
	gtk_check_button_set_active (clform->quiet, setting->commandline.quiet);
	gtk_spin_button_set_value (clform->index_spin, setting->commandline.nth_category);
}

void  ugtk_commandline_form_get (struct UgtkCommandlineForm* clform, UgtkSetting* setting)
{
	setting->commandline.quiet = gtk_check_button_get_active (clform->quiet);
	setting->commandline.nth_category = gtk_spin_button_get_value_as_int (clform->index_spin);
}

// ----------------------------------------------------------------------------
// UgtkPluginForm
//
static void on_plugin_launch_toggled (GtkWidget* widget, struct UgtkPluginForm* psform)
{
	gboolean  sensitive;

	sensitive = gtk_check_button_get_active (psform->launch);
	gtk_widget_set_sensitive ((GtkWidget*) psform->local, sensitive);
}

static void on_order_changed (GObject* object, GParamSpec* pspec, struct UgtkPluginForm* psform)
{
	int  index;

	index = gtk_drop_down_get_selected (GTK_DROP_DOWN (object));
	if (index >= UGTK_PLUGIN_ORDER_ARIA2)
		gtk_widget_set_sensitive (psform->aria2_opts, TRUE);
	else
		gtk_widget_set_sensitive (psform->aria2_opts, FALSE);
}

void  ugtk_plugin_form_init (struct UgtkPluginForm* psform)
{
	PangoContext*  context;
	PangoLayout*   layout;
	int            text_height;
	GtkBox*      vbox;
	GtkBox*      hbox;
	GtkBox*      box;
	GtkWidget*   widget;
	GtkScrolledWindow*  scroll;

	psform->self = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	vbox = (GtkBox*) psform->self;

	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Plug-in matching order:"));
	gtk_box_append (hbox, widget);
	{
		const char* plugin_orders[] = { "curl", "aria2", "curl + aria2", "aria2 + curl", NULL };
		widget = (GtkWidget*) gtk_drop_down_new_from_strings (plugin_orders);
	}
	psform->order = (GtkDropDown*) widget;
	g_signal_connect (psform->order, "notify::selected",
			G_CALLBACK (on_order_changed), psform);
	gtk_box_append (hbox, widget);

	widget = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_widget_set_vexpand (widget, TRUE);
	gtk_box_append (vbox, widget);
	psform->aria2_opts = widget;
	vbox = (GtkBox*) widget;

	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Aria2 plug-in options"));
	gtk_box_append (hbox, widget);
	gtk_box_append (hbox, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));

	// URI entry
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("URI"));
	gtk_box_append (hbox, widget);
	widget = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_box_append (hbox, widget);
	psform->uri = (GtkEntry*) widget;
	// Token entry
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	widget = gtk_label_new (_("RPC authorization secret token"));
	gtk_box_append (hbox, widget);
	widget = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_box_append (hbox, widget);
	psform->token = (GtkEntry*) widget;

	// ------------------------------------------------------------------------
	// Speed Limits
	widget = gtk_frame_new (_("Global speed limit for aria2 only"));
	gtk_box_append (vbox, widget);
	box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_frame_set_child (GTK_FRAME (widget), (GtkWidget*) box);
	set_margin_all ((GtkWidget*) box, 2);
	// Max upload speed
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (box, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Max upload speed"));
	gtk_box_append (hbox, widget);
	
	widget = gtk_spin_button_new_with_range (0.0, 4000000000.0, 5.0);
	gtk_editable_set_width_chars (GTK_EDITABLE (widget), 15);
	gtk_spin_button_set_activates_default (GTK_SPIN_BUTTON (widget), TRUE);
	gtk_box_append (hbox, widget);
	psform->upload = (GtkSpinButton*) widget;

	widget = gtk_label_new (_("KiB/s"));
	gtk_box_append (hbox, widget);

	// Max download speed
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (box, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Max download speed"));
	gtk_box_append (hbox, widget);
	
	widget = gtk_spin_button_new_with_range (0.0, 4000000000.0, 5.0);
	gtk_editable_set_width_chars (GTK_EDITABLE (widget), 15);
	gtk_spin_button_set_activates_default (GTK_SPIN_BUTTON (widget), TRUE);
	gtk_box_append (hbox, widget);
	psform->download = (GtkSpinButton*) widget;

	widget = gtk_label_new (_("KiB/s"));
	gtk_box_append (hbox, widget);

	// ------------------------------------------------------------------------
	// aria2 works on local device
	// launch
	widget = gtk_check_button_new_with_mnemonic (_("_Launch aria2 on startup"));
	gtk_box_append (vbox, widget);
	g_signal_connect (widget, "toggled",
			G_CALLBACK (on_plugin_launch_toggled), psform);
	psform->launch = (GtkCheckButton*) widget;
	// shutdown
	widget = gtk_check_button_new_with_mnemonic (_("_Shutdown aria2 on exit"));
	gtk_box_append (vbox, widget);
	psform->shutdown = (GtkCheckButton*) widget;

	// ------------------------------------------------------------------------
	// Local options
	widget = gtk_frame_new (_("Launch aria2 on local device"));
	gtk_box_append (vbox, widget);
	psform->local = widget;
	box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_frame_set_child (GTK_FRAME (widget), (GtkWidget*) box);
	set_margin_all ((GtkWidget*) box, 2);
	// Path
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (box, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Path"));
	gtk_box_append (hbox, widget);
	widget = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_box_append (hbox, widget);
	psform->path = (GtkEntry*) widget;
	//
	gtk_box_append (box, gtk_label_new (""));
	// Arguments
	// get text height --- begin ---
	context = gtk_widget_get_pango_context (widget);
	layout = pango_layout_new (context);
	pango_layout_set_text (layout, "joIN", -1);
	pango_layout_get_pixel_size (layout, NULL, &text_height);
	g_object_unref (layout);
	//  get text height --- end ---
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_append (box, (GtkWidget*) hbox);
	widget = gtk_label_new (_("Arguments"));
	gtk_box_append (hbox, widget);
	widget = gtk_label_new (" - ");
	gtk_box_append (hbox, widget);
	// Arguments - hint
	widget = gtk_label_new (_("You must restart uGet after modifying it."));
	gtk_box_append (hbox, widget);
	// Arguments - Scrolled Window
	scroll = (GtkScrolledWindow*) gtk_scrolled_window_new ();
	gtk_widget_set_size_request (GTK_WIDGET (scroll), 100, text_height * 3);
	gtk_widget_set_hexpand (GTK_WIDGET (scroll), TRUE);
    gtk_widget_set_vexpand (GTK_WIDGET (scroll), TRUE);
	gtk_box_append (box, GTK_WIDGET (scroll));
	// Arguments - text view
	psform->args_buffer = gtk_text_buffer_new (NULL);
	psform->args = gtk_text_view_new_with_buffer (psform->args_buffer);
	g_object_unref (psform->args_buffer);
	gtk_text_view_set_wrap_mode ((GtkTextView*) psform->args, GTK_WRAP_CHAR);
	gtk_scrolled_window_set_child (scroll, GTK_WIDGET (psform->args));

	// ------------------------------------------------------------------------
	on_plugin_launch_toggled ((GtkWidget*) psform->launch, psform);
	gtk_widget_set_visible(psform->self, TRUE);
}

void  ugtk_plugin_form_set (struct UgtkPluginForm* psform, UgtkSetting* setting)
{
	gtk_drop_down_set_selected (psform->order, setting->plugin_order);

	gtk_check_button_set_active (psform->launch, setting->aria2.launch);
	gtk_check_button_set_active (psform->shutdown, setting->aria2.shutdown);

	if (setting->aria2.uri)
		gtk_editable_set_text (GTK_EDITABLE (psform->uri),  setting->aria2.uri);
	if (setting->aria2.token)
		gtk_editable_set_text (GTK_EDITABLE (psform->token),  setting->aria2.token);
	if (setting->aria2.path)
		gtk_editable_set_text (GTK_EDITABLE (psform->path), setting->aria2.path);
//	if (setting->aria2.args)
//		gtk_entry_set_text (psform->args, setting->aria2.args);
	if (setting->aria2.args)
		gtk_text_buffer_set_text (psform->args_buffer, setting->aria2.args, -1);

	gtk_spin_button_set_value (psform->upload, setting->aria2.limit.upload);
	gtk_spin_button_set_value (psform->download, setting->aria2.limit.download);
}

void  ugtk_plugin_form_get (struct UgtkPluginForm* psform, UgtkSetting* setting)
{
	GtkTextIter  iter1;
	GtkTextIter  iter2;
	const char*  token;

	setting->plugin_order = gtk_drop_down_get_selected (psform->order);

	setting->aria2.launch = gtk_check_button_get_active (psform->launch);
	setting->aria2.shutdown = gtk_check_button_get_active (psform->shutdown);

	ug_free (setting->aria2.uri);
	ug_free (setting->aria2.token);
	ug_free (setting->aria2.path);
	ug_free (setting->aria2.args);
	setting->aria2.uri = (gchar*) ug_strdup (gtk_editable_get_text (GTK_EDITABLE (psform->uri)));
	token = gtk_editable_get_text (GTK_EDITABLE (psform->token));
	if (token[0] == 0)
		setting->aria2.token = NULL;
	else
		setting->aria2.token = (gchar*) ug_strdup (token);
	setting->aria2.path = (gchar*) ug_strdup (gtk_editable_get_text (GTK_EDITABLE (psform->path)));
//	setting->aria2.args = ug_strdup (gtk_entry_get_text (psform->args));
	gtk_text_buffer_get_start_iter (psform->args_buffer, &iter1);
	gtk_text_buffer_get_end_iter (psform->args_buffer, &iter2);
	setting->aria2.args = gtk_text_buffer_get_text (psform->args_buffer, &iter1, &iter2, FALSE);
	ug_str_remove_crlf (setting->aria2.args, setting->aria2.args);

	setting->aria2.limit.upload   = (guint) gtk_spin_button_get_value (psform->upload);
	setting->aria2.limit.download = (guint) gtk_spin_button_get_value (psform->download);
}

// ----------------------------------------------------------------------------
// UgtkResolverForm

static const int resolver_heights[] = { 0, 2160, 1440, 1080, 720, 480, 360, 240 };
static const int n_resolver_heights = sizeof (resolver_heights) / sizeof (int);

static const char* const resolver_exts[] = { "", "mp4", "webm", "mkv", NULL };

static int height_to_index (int height)
{
	int i;
	for (i = 0; i < n_resolver_heights; i++)
		if (resolver_heights[i] == height)
			return i;
	return 0;  // Best
}

static int ext_to_index (const char* ext)
{
	int i;
	if (ext == NULL || ext[0] == 0)
		return 0;
	for (i = 1; resolver_exts[i] != NULL; i++)
		if (strcmp (resolver_exts[i], ext) == 0)
			return i;
	return 0;
}

static void on_resolver_enable_toggled (GtkCheckButton* btn, struct UgtkResolverForm* rform)
{
	gboolean on = gtk_check_button_get_active (btn);
	gtk_widget_set_sensitive ((GtkWidget*) rform->max_height, on);
	gtk_widget_set_sensitive ((GtkWidget*) rform->preferred_ext, on);
	gtk_widget_set_sensitive ((GtkWidget*) rform->auto_pick, on);
	gtk_widget_set_sensitive ((GtkWidget*) rform->path, on);
}

static void resolver_refresh_status (struct UgtkResolverForm* rform)
{
	uget_resolver_invalidate ();
	if (uget_resolver_available ())
		gtk_label_set_text (rform->status,
				_("Status: media extractor is available."));
	else
		gtk_label_set_text (rform->status,
				_("Status: media extractor not found in PATH. Install yt-dlp (or set a path below) to enable media downloads."));
}

static void on_resolver_path_changed (GtkEditable* editable, struct UgtkResolverForm* rform)
{
	uget_resolver_set_path (gtk_editable_get_text (editable));
	resolver_refresh_status (rform);
}

void  ugtk_resolver_form_init (struct UgtkResolverForm* rform)
{
	GtkBox*     vbox;
	GtkGrid*    grid;
	GtkWidget*  widget;

	rform->self = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	vbox = (GtkBox*) rform->self;

	// Enable master switch
	widget = gtk_check_button_new_with_mnemonic (
			_("_Enable media extraction (via yt-dlp)"));
	rform->enable = (GtkCheckButton*) widget;
	gtk_box_append (vbox, widget);
	g_signal_connect (widget, "toggled",
			G_CALLBACK (on_resolver_enable_toggled), rform);

	// Explanation
	widget = gtk_label_new (_(
			"When enabled, clicking the \"find\" icon in the URL field of "
			"the New Download dialog will query an external extractor to "
			"fetch available stream formats from media page URLs."));
	gtk_label_set_wrap (GTK_LABEL (widget), TRUE);
	gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
	g_object_set (widget, "margin-start", 24, "margin-end", 4, NULL);
	gtk_box_append (vbox, widget);

	// Option grid
	grid = (GtkGrid*) gtk_grid_new ();
	gtk_grid_set_column_spacing (grid, 8);
	gtk_grid_set_row_spacing (grid, 4);
	g_object_set (grid, "margin-start", 24, "margin-top", 6, "margin-bottom", 6, NULL);
	gtk_box_append (vbox, (GtkWidget*) grid);

	// Max height
	widget = gtk_label_new (_("Max resolution:"));
	gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
	gtk_grid_attach (grid, widget, 0, 0, 1, 1);
	{
		const char* heights[] = { _("Best"), "2160p", "1440p", "1080p", "720p", "480p", "360p", "240p", NULL };
		widget = (GtkWidget*) gtk_drop_down_new_from_strings (heights);
	}
	rform->max_height = (GtkDropDown*) widget;
	gtk_grid_attach (grid, widget, 1, 0, 1, 1);

	// Preferred container
	widget = gtk_label_new (_("Preferred container:"));
	gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
	gtk_grid_attach (grid, widget, 0, 1, 1, 1);
	{
		const char* exts[] = { _("Best"), "mp4", "webm", "mkv", NULL };
		widget = (GtkWidget*) gtk_drop_down_new_from_strings (exts);
	}
	rform->preferred_ext = (GtkDropDown*) widget;
	gtk_grid_attach (grid, widget, 1, 1, 1, 1);

	// Auto-pick
	widget = gtk_check_button_new_with_mnemonic (
			_("_Auto-pick best matching format (skip selector)"));
	rform->auto_pick = (GtkCheckButton*) widget;
	g_object_set (widget, "margin-top", 4, NULL);
	gtk_grid_attach (grid, widget, 0, 2, 2, 1);

	// Path
	widget = gtk_label_new (_("Extractor:"));
	gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
	g_object_set (widget, "margin-top", 6, NULL);
	gtk_grid_attach (grid, widget, 0, 3, 1, 1);

	widget = gtk_entry_new ();
	gtk_entry_set_placeholder_text (GTK_ENTRY (widget), "yt-dlp");
	g_object_set (widget, "hexpand", TRUE, "margin-top", 6, NULL);
	rform->path = (GtkEntry*) widget;
	gtk_grid_attach (grid, widget, 1, 3, 1, 1);
	g_signal_connect (widget, "changed",
			G_CALLBACK (on_resolver_path_changed), rform);

	// Status line
	widget = gtk_label_new ("");
	gtk_label_set_xalign (GTK_LABEL (widget), 0.0);
	gtk_label_set_wrap (GTK_LABEL (widget), TRUE);
	g_object_set (widget, "margin-start", 24, "margin-end", 4, "margin-top", 6, NULL);
	rform->status = (GtkLabel*) widget;
	gtk_box_append (vbox, widget);

	gtk_widget_set_visible (rform->self, TRUE);
}

void  ugtk_resolver_form_set (struct UgtkResolverForm* rform, UgtkSetting* setting)
{
	gtk_check_button_set_active (rform->enable, setting->resolver.enable);
	gtk_drop_down_set_selected (rform->max_height,
			height_to_index (setting->resolver.max_height));
	gtk_drop_down_set_selected (rform->preferred_ext,
			ext_to_index (setting->resolver.preferred_ext));
	gtk_check_button_set_active (rform->auto_pick, setting->resolver.auto_pick);
	gtk_editable_set_text (GTK_EDITABLE (rform->path),
			setting->resolver.path ? setting->resolver.path : "");

	// Propagate enable state to option widgets
	on_resolver_enable_toggled (rform->enable, rform);
	resolver_refresh_status (rform);
}

void  ugtk_resolver_form_get (struct UgtkResolverForm* rform, UgtkSetting* setting)
{
	int idx;
	const char* text;

	setting->resolver.enable = gtk_check_button_get_active (rform->enable);

	idx = (int) gtk_drop_down_get_selected (rform->max_height);
	if (idx < 0 || idx >= n_resolver_heights)
		idx = 0;
	setting->resolver.max_height = resolver_heights[idx];

	idx = (int) gtk_drop_down_get_selected (rform->preferred_ext);
	ug_free (setting->resolver.preferred_ext);
	if (idx > 0 && resolver_exts[idx])
		setting->resolver.preferred_ext = ug_strdup (resolver_exts[idx]);
	else
		setting->resolver.preferred_ext = NULL;

	setting->resolver.auto_pick = gtk_check_button_get_active (rform->auto_pick);

	text = gtk_editable_get_text (GTK_EDITABLE (rform->path));
	ug_free (setting->resolver.path);
	setting->resolver.path = (text && text[0]) ? ug_strdup (text) : NULL;

	uget_resolver_set_path (setting->resolver.path);
}

