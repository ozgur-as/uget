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

#include <UgUri.h>
#include <UgString.h>
#include <UgtkDownloadForm.h>
#include <UgetData.h>
#include <UgtkApp.h>		// UGTK_APP_NAME

#include <glib/gi18n.h>


static void ugtk_download_form_init_page1 (UgtkDownloadForm* dform, UgtkProxyForm* proxy);
static void ugtk_download_form_init_page2 (UgtkDownloadForm* dform);
//	signal handler
static void on_spin_changed (GtkEditable *editable, UgtkDownloadForm* dform);
static void on_entry_changed (GtkEditable *editable, UgtkDownloadForm* dform);
static void on_uri_entry_changed (GtkEditable *editable, UgtkDownloadForm* dform);
static void on_http_entry_changed (GtkEditable *editable, UgtkDownloadForm* dform);
static void on_select_folder (GtkEntry* entry, GtkEntryIconPosition icon_pos, GdkEvent* event, UgtkDownloadForm* dform);
static void on_select_cookie (GtkEntry* entry, GtkEntryIconPosition icon_pos, GdkEvent* event, UgtkDownloadForm* dform);
static void on_select_post   (GtkEntry* entry, GtkEntryIconPosition icon_pos, GdkEvent* event, UgtkDownloadForm* dform);

static void on_dialog_run_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	gint *response = (gint*)user_data;
	*response = response_id;
}


void  ugtk_download_form_init (UgtkDownloadForm* dform, UgtkProxyForm* proxy, GtkWindow* parent)
{
	dform->changed.enable   = TRUE;
	dform->changed.uri      = FALSE;
	dform->changed.file     = FALSE;
	dform->changed.folder   = FALSE;
	dform->changed.user     = FALSE;
	dform->changed.password = FALSE;
	dform->changed.referrer = FALSE;
	dform->changed.cookie   = FALSE;
	dform->changed.post     = FALSE;
	dform->changed.agent    = FALSE;
	dform->changed.retry    = FALSE;
	dform->changed.delay    = FALSE;
	dform->changed.timestamp= FALSE;
	dform->parent = parent;

	ugtk_download_form_init_page1 (dform, proxy);
	ugtk_download_form_init_page2 (dform);

//	gtk_widget_show_all (dform->page1);
//	gtk_widget_show_all (dform->page2);
}

static void ugtk_download_form_init_page1 (UgtkDownloadForm* dform, UgtkProxyForm* proxy)
{
	GtkWidget*  widget;
	GtkGrid*    top_grid;
	GtkGrid*    grid;
	GtkWidget*  frame;
	GtkBox*	    top_vbox;
	GtkWidget*  vbox;
	GtkWidget*  hbox;

	dform->page1 = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	top_vbox = (GtkBox*) dform->page1;
	top_vbox = (GtkBox*) dform->page1;
	g_object_set (top_vbox, "margin-start", 2, "margin-end", 2, "margin-top", 2, "margin-bottom", 2, NULL);

	top_grid = (GtkGrid*) gtk_grid_new ();
	gtk_box_append (top_vbox, (GtkWidget*) top_grid);

	// URL - entry
	widget = gtk_entry_new ();
//	gtk_entry_set_width_chars (GTK_ENTRY (widget), 20); // remove for GTK+ 3.12
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	g_object_set (widget, "margin-left", 1, "margin-right", 1, NULL);
	g_object_set (widget, "margin-top", 2, "margin-bottom", 2, NULL);
	g_object_set (widget, "hexpand", TRUE, NULL);
	gtk_grid_attach (top_grid, widget, 1, 0, 2, 1);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_uri_entry_changed), dform);
	dform->uri_entry = widget;
	// URL - label
	widget = gtk_label_new_with_mnemonic (_("_URI:"));
	gtk_label_set_mnemonic_widget (GTK_LABEL(widget), dform->uri_entry);
	g_object_set (widget, "margin-left", 3, "margin-right", 3, NULL);
	g_object_set (widget, "margin-top", 2, "margin-bottom", 2, NULL);
	gtk_grid_attach (top_grid, widget, 0, 0, 1, 1);
	dform->uri_label = widget;

	// Mirrors - entry
	widget = gtk_entry_new ();
//	gtk_entry_set_width_chars (GTK_ENTRY (widget), 20); // remove for GTK+ 3.12
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	g_object_set (widget, "margin-left", 1, "margin-right", 1, NULL);
	g_object_set (widget, "margin-top", 2, "margin-bottom", 2, NULL);
	g_object_set (widget, "hexpand", TRUE, NULL);
	gtk_grid_attach (top_grid, widget, 1, 1, 2, 1);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_uri_entry_changed), dform);
	dform->mirrors_entry = widget;
	// Mirrors - label
	widget = gtk_label_new_with_mnemonic (_("Mirrors:"));
	gtk_label_set_mnemonic_widget (GTK_LABEL(widget), dform->mirrors_entry);
	g_object_set (widget, "margin-left", 3, "margin-right", 3, NULL);
	g_object_set (widget, "margin-top", 2, "margin-bottom", 2, NULL);
	gtk_grid_attach (top_grid, widget, 0, 1, 1, 1);
	dform->mirrors_label = widget;

	// File - entry
	widget = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	gtk_entry_set_placeholder_text (GTK_ENTRY (widget),
			_("Leave blank to use default filename ..."));
	g_object_set (widget, "margin", 1, "hexpand", TRUE, NULL);
	gtk_grid_attach (top_grid, widget,  1, 2, 2, 1);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_entry_changed), dform);
	dform->file_entry = widget;
	// File - label
	widget = gtk_label_new_with_mnemonic (_("File:"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (widget), dform->file_entry);
	g_object_set (widget, "margin-left", 3, "margin-right", 3, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (top_grid, widget,  0, 2, 1, 1);
	dform->file_label = widget;

	// Folder - combo entry + icon
	dform->folder_combo = gtk_combo_box_text_new_with_entry ();
	dform->folder_entry = gtk_combo_box_get_child (GTK_COMBO_BOX (dform->folder_combo));
	widget = dform->folder_entry;
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 10
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY (widget),
			GTK_ENTRY_ICON_SECONDARY, "folder");
#else
	gtk_entry_set_icon_from_stock (GTK_ENTRY (widget),
			GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_DIRECTORY);
#endif
	gtk_entry_set_icon_tooltip_text (GTK_ENTRY (widget),
			GTK_ENTRY_ICON_SECONDARY, _("Select Folder"));
	g_object_set (dform->folder_combo, "margin", 1, "hexpand", TRUE, NULL);
	gtk_grid_attach (top_grid, dform->folder_combo,  1, 3, 1, 1);
	g_signal_connect (widget, "icon-release",
			G_CALLBACK (on_select_folder), dform);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_entry_changed), dform);
	// Folder - label
	widget = gtk_label_new_with_mnemonic (_("_Folder:"));
	gtk_label_set_mnemonic_widget(GTK_LABEL (widget), dform->folder_combo);
	g_object_set (widget, "margin-left", 3, "margin-right", 3, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (top_grid, widget,  0, 3, 1, 1);

	// Referrer - entry
	widget = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	g_object_set (widget, "margin", 1, "hexpand", TRUE, NULL);
	gtk_grid_attach (top_grid, widget, 1, 4, 2, 1);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_http_entry_changed), dform);
	dform->referrer_entry = widget;
	// Referrer - label
	widget = gtk_label_new_with_mnemonic (_("Referrer:"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (widget), dform->referrer_entry);
	g_object_set (widget, "margin-left", 3, "margin-right", 3, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (top_grid, widget, 0, 4, 1, 1);
//	dform->referrer_label = widget;

	// ----------------------------------------------------
	// Connections
	// HBox for Connections
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (top_vbox, hbox);
	// connections - spin button
	widget = gtk_spin_button_new_with_range (1.0, 16.0, 1.0);
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	gtk_box_append (GTK_BOX (hbox), widget);
	dform->spin_connections = widget;
	// "Max Connections:" - title label
	widget = gtk_label_new_with_mnemonic (_("_Max Connections:"));
	gtk_label_set_mnemonic_widget ((GtkLabel*)widget, dform->spin_connections);
	gtk_box_append (GTK_BOX (hbox), widget);
	dform->title_connections = widget;

	// ----------------------------------------------------
	// HBox for "Status" and "Login"
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (top_vbox, hbox);

	// ----------------------------------------------------
	// frame for Status (start mode)
	frame = gtk_frame_new (_("Status"));
	vbox = (GtkWidget*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 2);
	g_object_set (vbox, "margin-start", 2, "margin-end", 2, "margin-top", 2, "margin-bottom", 2, NULL);
	gtk_frame_set_child (GTK_FRAME (frame), vbox);
	gtk_box_append (GTK_BOX (hbox), frame);
	dform->radio_runnable = (GtkWidget*) gtk_check_button_new_with_mnemonic (_("_Runnable"));
	dform->radio_pause = (GtkWidget*) gtk_check_button_new_with_mnemonic (_("P_ause"));
	gtk_check_button_set_group (GTK_CHECK_BUTTON (dform->radio_pause), GTK_CHECK_BUTTON (dform->radio_runnable));

	gtk_box_append (GTK_BOX (vbox), dform->radio_runnable);
	gtk_box_append (GTK_BOX (vbox), dform->radio_pause);

	// ----------------------------------------------------
	// frame for login
	frame = gtk_frame_new (_("Login"));
	grid  = (GtkGrid*) gtk_grid_new ();
	g_object_set (grid, "margin-start", 2, "margin-end", 2, "margin-top", 2, "margin-bottom", 2, NULL);
	gtk_frame_set_child (GTK_FRAME (frame), (GtkWidget*) grid);
	gtk_widget_set_hexpand (frame, TRUE);
	gtk_widget_set_vexpand (frame, TRUE);
	gtk_box_append (GTK_BOX (hbox), frame);
	// User - entry
	widget = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	g_object_set (widget, "margin", 1, "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, widget, 1, 0, 1, 1);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_entry_changed), dform);
	dform->username_entry = widget;
	// User - label
	widget = gtk_label_new (_("User:"));
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 0, 0, 1, 1);
//	dform->username_label = widget;

	// Password - entry
	widget = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (widget), FALSE);
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	g_object_set (widget, "margin", 1, "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, widget, 1, 1, 1, 1);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_entry_changed), dform);
	dform->password_entry = widget;
	// Password - label
	widget = gtk_label_new (_("Password:"));
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 0, 1, 1, 1);
//	dform->password_label = widget;

	// ----------------------------------------------------
	// proxy
//	ug_proxy_widget_init (&dform->proxy_dform);
	if (proxy) {
		widget = proxy->self;
		gtk_box_append (top_vbox, widget);
	}
}

static void ugtk_download_form_init_page2 (UgtkDownloadForm* dform)
{
	GtkWidget*  widget;
	GtkGrid*    grid;

	dform->page2 = gtk_grid_new ();
	grid = (GtkGrid*) dform->page2;
	g_object_set (grid, "margin-start", 2, "margin-end", 2, "margin-top", 2, "margin-bottom", 2, NULL);

	// label - cookie file
	widget = gtk_label_new (_("Cookie file:"));
	// GTK4: Use gtk_label_set_xalign/yalign instead of deprecated gtk_misc_set_alignment
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 0, 0, 1, 1);
	dform->cookie_label = widget;
	// entry - cookie file
	widget = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 10
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY (widget),
			GTK_ENTRY_ICON_SECONDARY, "text-x-generic");
#else
	gtk_entry_set_icon_from_stock (GTK_ENTRY (widget),
			GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_FILE);
#endif
	gtk_entry_set_icon_tooltip_text (GTK_ENTRY (widget),
			GTK_ENTRY_ICON_SECONDARY, _("Select Cookie File"));
	g_object_set (widget, "margin", 1, "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, widget, 1, 0, 3, 1);
	g_signal_connect (widget, "icon-release",
			G_CALLBACK (on_select_cookie), dform);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_http_entry_changed), dform);
	dform->cookie_entry = widget;
	// label - post file
	widget = gtk_label_new (_("Post file:"));
	// GTK4: Use gtk_label_set_xalign/yalign instead of deprecated gtk_misc_set_alignment
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 0, 1, 1, 1);
	dform->post_label = widget;
	// entry - post file
	widget = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
#if GTK_MAJOR_VERSION >= 3 && GTK_MINOR_VERSION >= 10
	gtk_entry_set_icon_from_icon_name (GTK_ENTRY (widget),
			GTK_ENTRY_ICON_SECONDARY, "text-x-generic");
#else
	gtk_entry_set_icon_from_stock (GTK_ENTRY (widget),
			GTK_ENTRY_ICON_SECONDARY, GTK_STOCK_FILE);
#endif
	gtk_entry_set_icon_tooltip_text (GTK_ENTRY (widget),
			GTK_ENTRY_ICON_SECONDARY, _("Select Post File"));
	g_object_set (widget, "margin", 1, "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, widget, 1, 1, 3, 1);
	g_signal_connect (widget, "icon-release",
			G_CALLBACK (on_select_post), dform);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_http_entry_changed), dform);
	dform->post_entry = widget;

	// label - user agent
	widget = gtk_label_new (_("User Agent:"));
	// GTK4: Use gtk_label_set_xalign/yalign instead of deprecated gtk_misc_set_alignment
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 0, 2, 1, 1);
	dform->agent_label = widget;
	// entry - user agent
	widget = gtk_entry_new ();
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	g_object_set (widget, "margin", 1, "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, widget, 1, 2, 3, 1);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_http_entry_changed), dform);
	dform->agent_entry = widget;

	// Retry limit - label
	widget = gtk_label_new_with_mnemonic (_("Retry _limit:"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (widget), dform->spin_retry);
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 0, 3, 2, 1);
	// Retry limit - spin button
	widget = gtk_spin_button_new_with_range (0.0, 99.0, 1.0);
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 2, 3, 1, 1);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_spin_changed), dform);
	dform->spin_retry = widget;
	// counts - label
	widget = gtk_label_new (_("counts"));
	// GTK4: Use gtk_label_set_xalign/yalign instead of deprecated gtk_misc_set_alignment
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 3, 3, 1, 1);

	// Retry delay - label
	widget = gtk_label_new_with_mnemonic (_("Retry _delay:"));
	gtk_label_set_mnemonic_widget (GTK_LABEL (widget), dform->spin_delay);
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 0, 4, 2, 1);
	// Retry delay - spin button
	widget = gtk_spin_button_new_with_range (0.0, 600.0, 1.0);
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 2, 4, 1, 1);
	g_signal_connect (GTK_EDITABLE (widget), "changed",
			G_CALLBACK (on_spin_changed), dform);
	dform->spin_delay = widget;
	// seconds - label
	widget = gtk_label_new (_("seconds"));
	// GTK4: Use gtk_label_set_xalign/yalign instead of deprecated gtk_misc_set_alignment
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 3, 4, 1, 1);

	// label - Max upload speed
	widget = gtk_label_new (_("Max upload speed:"));
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 0, 5, 2, 1);
	// spin - Max upload speed
	widget = gtk_spin_button_new_with_range (0, 99999999, 1);
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
//	gtk_entry_set_width_chars (GTK_ENTRY (widget), 8);
	g_object_set (widget, "margin", 1, NULL);
	gtk_grid_attach (grid, widget, 2, 5, 1, 1);
	dform->spin_upload_speed = (GtkSpinButton*) widget;
	// label - "KiB/s"
	widget = gtk_label_new (_("KiB/s"));
	// GTK4: Use gtk_label_set_xalign/yalign instead of deprecated gtk_misc_set_alignment
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin", 2, "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, widget, 3, 5, 1, 1);

	// label - Max download speed
	widget = gtk_label_new (_("Max download speed:"));
	g_object_set (widget, "margin-left", 2, "margin-right", 2, NULL);
	g_object_set (widget, "margin-top", 1, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 0, 6, 2, 1);
	// spin - Max download speed
	widget = gtk_spin_button_new_with_range (0, 99999999, 1);
	gtk_entry_set_activates_default (GTK_ENTRY (widget), TRUE);
//	gtk_entry_set_width_chars (GTK_ENTRY (widget), 8);
	g_object_set (widget, "margin", 1, NULL);
	gtk_grid_attach (grid, widget, 2, 6, 1, 1);
	dform->spin_download_speed = (GtkSpinButton*) widget;
	// label - "KiB/s"
	widget = gtk_label_new (_("KiB/s"));
	// GTK4: Use gtk_label_set_xalign/yalign instead of deprecated gtk_misc_set_alignment
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin", 2, "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, widget, 3, 6, 1, 1);

	// Retrieve timestamp
	widget = gtk_check_button_new_with_label (_("Retrieve timestamp"));
	g_object_set (widget, "margin-top", 3, "margin-bottom", 1, NULL);
	gtk_grid_attach (grid, widget, 0, 7, 3, 1);
	dform->timestamp = (GtkToggleButton*) widget;
}

void  ugtk_download_form_get (UgtkDownloadForm* dform, UgInfo* node_info)
{
	UgUri         uuri;
	const gchar*  text;
	gint          number;
	union {
		UgetRelation* relation;
		UgetCommon*   common;
		UgetHttp*     http;
	} temp;

	// ------------------------------------------
	// UgetCommon
	temp.common = ug_info_realloc(node_info, UgetCommonInfo);
	// folder
	text = gtk_editable_get_text (GTK_EDITABLE (dform->folder_entry));
	ug_free (temp.common->folder);
	temp.common->folder = (*text) ? ug_strdup (text) : NULL;
	ug_str_remove_crlf (temp.common->folder, temp.common->folder);
	// user
	text = gtk_editable_get_text (GTK_EDITABLE (dform->username_entry));
	ug_free (temp.common->user);
	temp.common->user = (*text) ? ug_strdup (text) : NULL;
	ug_str_remove_crlf (temp.common->user, temp.common->user);
	// password
	text = gtk_editable_get_text (GTK_EDITABLE (dform->password_entry));
	ug_free (temp.common->password);
	temp.common->password = (*text) ? ug_strdup (text) : NULL;
	ug_str_remove_crlf (temp.common->password, temp.common->password);
	// retry_limit
	number = gtk_spin_button_get_value_as_int ((GtkSpinButton*) dform->spin_retry);
	temp.common->retry_limit = number;
	// retry_delay
	number = gtk_spin_button_get_value_as_int ((GtkSpinButton*) dform->spin_delay);
	temp.common->retry_delay = number;

	// max_upload_speed
	number = gtk_spin_button_get_value_as_int ((GtkSpinButton*) dform->spin_upload_speed) * 1024;
	temp.common->max_upload_speed = number;
	// max_download_speed
	number = gtk_spin_button_get_value_as_int ((GtkSpinButton*) dform->spin_download_speed) * 1024;
	temp.common->max_download_speed = number;

	// max_connections
	number = gtk_spin_button_get_value_as_int ((GtkSpinButton*) dform->spin_connections);
	temp.common->max_connections = number;
	// timestamp
	temp.common->timestamp = gtk_check_button_get_active (GTK_CHECK_BUTTON (dform->timestamp));

	// URI
	if (gtk_widget_is_sensitive (dform->uri_entry) == TRUE) {
		text = gtk_editable_get_text (GTK_EDITABLE (dform->uri_entry));
		ug_free (temp.common->uri);
		temp.common->uri = (*text) ? ug_strdup (text) : NULL;
		ug_str_remove_crlf (temp.common->uri, temp.common->uri);
		if (temp.common->uri) {
			ug_uri_init (&uuri, temp.common->uri);
			// set user
			number = ug_uri_user (&uuri, &text);
			if (number > 0) {
				ug_free (temp.common->user);
				temp.common->user = ug_strndup (text, number);
			}
			// set password
			number = ug_uri_password (&uuri, &text);
			if (number > 0) {
				ug_free (temp.common->password);
				temp.common->password = ug_strndup (text, number);
			}
			// Remove user & password from URL
			if (uuri.authority != uuri.host) {
				memmove ((char*)uuri.uri + uuri.authority, (char*)uuri.uri + uuri.host,
						strlen (uuri.uri + uuri.host) + 1);
			}
		}
	}
	// mirrors
	if (gtk_widget_is_sensitive (dform->mirrors_entry) == TRUE) {
		text = gtk_editable_get_text (GTK_EDITABLE (dform->mirrors_entry));
		ug_free (temp.common->mirrors);
		temp.common->mirrors = (*text) ? ug_strdup (text) : NULL;
		ug_str_remove_crlf (temp.common->mirrors, temp.common->mirrors);
	}
	// file
	if (gtk_widget_is_sensitive (dform->file_entry) == TRUE) {
		text = gtk_editable_get_text (GTK_EDITABLE (dform->file_entry));
		ug_free (temp.common->file);
		temp.common->file = (*text) ? ug_strdup (text) : NULL;
		ug_str_remove_crlf (temp.common->file, temp.common->file);
	}

	// ------------------------------------------
	// UgetHttp
	temp.http = ug_info_realloc(node_info, UgetHttpInfo);
	// referrer
	text = gtk_editable_get_text (GTK_EDITABLE (dform->referrer_entry));
	ug_free (temp.http->referrer);
	temp.http->referrer = (*text) ? ug_strdup (text) : NULL;
	ug_str_remove_crlf (temp.http->referrer, temp.http->referrer);
	// cookie_file
	text = gtk_editable_get_text (GTK_EDITABLE (dform->cookie_entry));
	ug_free (temp.http->cookie_file);
	temp.http->cookie_file = (*text) ? ug_strdup (text) : NULL;
	ug_str_remove_crlf (temp.http->cookie_file, temp.http->cookie_file);
	// post_file
	text = gtk_editable_get_text (GTK_EDITABLE (dform->post_entry));
	ug_free (temp.http->post_file);
	temp.http->post_file = (*text) ? ug_strdup (text) : NULL;
	ug_str_remove_crlf (temp.http->post_file, temp.http->post_file);
	// user_agent
	text = gtk_editable_get_text (GTK_EDITABLE (dform->agent_entry));
	ug_free (temp.http->user_agent);
	temp.http->user_agent = (*text) ? ug_strdup (text) : NULL;
	ug_str_remove_crlf (temp.http->user_agent, temp.http->user_agent);

	// ------------------------------------------
	// UgetRelation
	if (gtk_widget_get_sensitive (dform->radio_pause)) {
		temp.relation = ug_info_realloc(node_info, UgetRelationInfo);
		if (gtk_check_button_get_active (GTK_CHECK_BUTTON (dform->radio_pause)))
			temp.relation->group |= UGET_GROUP_PAUSED;
		else
			temp.relation->group &= ~UGET_GROUP_PAUSED;
	}
}

void  ugtk_download_form_set (UgtkDownloadForm* dform, UgInfo* node_info, gboolean keep_changed)
{
	UgetRelation* relation;
	UgetCommon*   common;
	UgetHttp*     http;

	common = ug_info_realloc(node_info, UgetCommonInfo);
	http   = ug_info_get(node_info, UgetHttpInfo);

	// disable changed flags
	dform->changed.enable = FALSE;
	// ------------------------------------------
	// UgetCommon
	// set changed flags
	if (keep_changed == FALSE && common) {
		dform->changed.uri      = common->keeping.uri;
		dform->changed.mirrors  = common->keeping.mirrors;
		dform->changed.file     = common->keeping.file;
		dform->changed.folder   = common->keeping.folder;
		dform->changed.user     = common->keeping.user;
		dform->changed.password = common->keeping.password;
		dform->changed.retry    = common->keeping.retry_limit;
		dform->changed.delay    = common->keeping.retry_delay;
		dform->changed.max_upload_speed   = common->keeping.max_upload_speed;
		dform->changed.max_download_speed = common->keeping.max_download_speed;
		dform->changed.timestamp = common->keeping.timestamp;
	}
	// set data
	if (keep_changed==FALSE || dform->changed.uri==FALSE) {
		if (gtk_widget_is_sensitive (dform->uri_entry)) {
			gtk_editable_set_text (GTK_EDITABLE (dform->uri_entry),
					(common && common->uri)  ? common->uri : "");
		}
	}
	if (keep_changed==FALSE || dform->changed.mirrors==FALSE) {
		if (gtk_widget_is_sensitive (dform->mirrors_entry)) {
			gtk_editable_set_text (GTK_EDITABLE (dform->mirrors_entry),
					(common && common->mirrors)  ? common->mirrors : "");
		}
	}
	if (keep_changed==FALSE || dform->changed.file==FALSE) {
		if (gtk_widget_is_sensitive (dform->file_entry)) {
			gtk_editable_set_text (GTK_EDITABLE (dform->file_entry),
					(common && common->file) ? common->file : "");
			// set changed flags
			if (common && common->file)
				dform->changed.file = TRUE;
		}
	}
	if (keep_changed==FALSE || dform->changed.folder==FALSE) {
		g_signal_handlers_block_by_func (GTK_EDITABLE (dform->folder_entry),
				on_entry_changed, dform);
		gtk_editable_set_text (GTK_EDITABLE (dform->folder_entry),
				(common && common->folder) ? common->folder : "");
		g_signal_handlers_unblock_by_func (GTK_EDITABLE (dform->folder_entry),
				on_entry_changed, dform);
	}
	if (keep_changed==FALSE || dform->changed.user==FALSE) {
		gtk_editable_set_text (GTK_EDITABLE (dform->username_entry),
				(common && common->user) ? common->user : "");
	}
	if (keep_changed==FALSE || dform->changed.password==FALSE) {
		gtk_editable_set_text (GTK_EDITABLE (dform->password_entry),
				(common && common->password) ? common->password : "");
	}
	if (keep_changed==FALSE || dform->changed.retry==FALSE) {
		gtk_spin_button_set_value ((GtkSpinButton*) dform->spin_retry,
				(common) ? common->retry_limit : 99);
	}
	if (keep_changed==FALSE || dform->changed.delay==FALSE) {
		gtk_spin_button_set_value ((GtkSpinButton*) dform->spin_delay,
				(common) ? common->retry_delay : 6);
	}

	if (keep_changed==FALSE || dform->changed.max_upload_speed==FALSE) {
		gtk_spin_button_set_value ((GtkSpinButton*) dform->spin_upload_speed,
				(gdouble) (common->max_upload_speed / 1024));
	}
	if (keep_changed==FALSE || dform->changed.max_download_speed==FALSE) {
		gtk_spin_button_set_value ((GtkSpinButton*) dform->spin_download_speed,
				(gdouble) (common->max_download_speed / 1024));
	}

	if (keep_changed==FALSE || dform->changed.connections==FALSE) {
		gtk_spin_button_set_value ((GtkSpinButton*) dform->spin_connections,
			common->max_connections);
	}
	if (keep_changed==FALSE || dform->changed.timestamp==FALSE)
		gtk_check_button_set_active (GTK_CHECK_BUTTON (dform->timestamp), common->timestamp);

	// ------------------------------------------
	// UgetHttp
	// set data
	if (keep_changed==FALSE || dform->changed.referrer==FALSE) {
		gtk_editable_set_text (GTK_EDITABLE (dform->referrer_entry),
				(http && http->referrer) ? http->referrer : "");
	}
	if (keep_changed==FALSE || dform->changed.cookie==FALSE) {
		gtk_editable_set_text (GTK_EDITABLE (dform->cookie_entry),
				(http && http->cookie_file) ? http->cookie_file : "");
	}
	if (keep_changed==FALSE || dform->changed.post==FALSE) {
		gtk_editable_set_text (GTK_EDITABLE (dform->post_entry),
				(http && http->post_file) ? http->post_file : "");
	}
	if (keep_changed==FALSE || dform->changed.agent==FALSE) {
		gtk_editable_set_text (GTK_EDITABLE (dform->agent_entry),
				(http && http->user_agent) ? http->user_agent : "");
	}
	// set changed flags
	if (keep_changed==FALSE && http) {
		dform->changed.referrer = http->keeping.referrer;
		dform->changed.cookie   = http->keeping.cookie_file;
		dform->changed.post     = http->keeping.post_file;
		dform->changed.agent    = http->keeping.user_agent;
	}

	// ------------------------------------------
	// UgetRelation
	if (gtk_widget_get_sensitive (dform->radio_pause)) {
		relation = ug_info_realloc(node_info, UgetRelationInfo);
		if (relation->group & UGET_GROUP_PAUSED)
			gtk_check_button_set_active (GTK_CHECK_BUTTON (dform->radio_pause), TRUE);
		else
			gtk_check_button_set_active (GTK_CHECK_BUTTON (dform->radio_runnable), TRUE);
	}

	// enable changed flags
	dform->changed.enable = TRUE;
	// complete entry
	ugtk_download_form_complete_entry (dform);
}

void  ugtk_download_form_set_multiple (UgtkDownloadForm* dform, gboolean multiple_mode)
{
//	dform->multiple = multiple_mode;

	if (multiple_mode) {
		gtk_widget_hide (dform->uri_label);
		gtk_widget_hide (dform->uri_entry);
		gtk_widget_hide (dform->mirrors_label);
		gtk_widget_hide (dform->mirrors_entry);
		gtk_widget_hide (dform->file_label);
		gtk_widget_hide (dform->file_entry);
	}
	else {
		gtk_widget_show (dform->uri_label);
		gtk_widget_show (dform->uri_entry);
		gtk_widget_show (dform->mirrors_label);
		gtk_widget_show (dform->mirrors_entry);
		gtk_widget_show (dform->file_label);
		gtk_widget_show (dform->file_entry);
	}

	multiple_mode = !multiple_mode;
	gtk_widget_set_sensitive (dform->uri_label,  multiple_mode);
	gtk_widget_set_sensitive (dform->uri_entry,  multiple_mode);
	gtk_widget_set_sensitive (dform->mirrors_label, multiple_mode);
	gtk_widget_set_sensitive (dform->mirrors_entry, multiple_mode);
	gtk_widget_set_sensitive (dform->file_label, multiple_mode);
	gtk_widget_set_sensitive (dform->file_entry, multiple_mode);
}

void  ugtk_download_form_set_folders (UgtkDownloadForm* dform, UgtkSetting* setting)
{
	GtkComboBoxText* combo;
	UgLink*  link;

	dform->changed.enable = FALSE;
	g_signal_handlers_block_by_func (GTK_EDITABLE (dform->folder_entry),
			on_entry_changed, dform);
	combo = GTK_COMBO_BOX_TEXT (dform->folder_combo);
	for (link = setting->folder_history.head;  link;  link = link->next)
		gtk_combo_box_text_append_text (combo, link->data);
	// set default folder
	gtk_combo_box_set_active (GTK_COMBO_BOX (combo), 0);
	g_signal_handlers_unblock_by_func (GTK_EDITABLE (dform->folder_entry),
			on_entry_changed, dform);
	dform->changed.enable = TRUE;
}

void  ugtk_download_form_get_folders (UgtkDownloadForm* dform, UgtkSetting* setting)
{
	const gchar* current;

	current = gtk_editable_get_text (GTK_EDITABLE (dform->folder_entry));
	ugtk_setting_add_folder (setting, current);
}

void  ugtk_download_form_complete_entry (UgtkDownloadForm* dform)
{
	const gchar*  text;
//	gchar*    temp;
	UgUri     upart;
	gboolean  completed = FALSE;

	// URL
	// URL
	text = gtk_editable_get_text (GTK_EDITABLE (dform->uri_entry));
	ug_uri_init (&upart, text);
	if (upart.host != -1) {
		// disable changed flags
		dform->changed.enable = FALSE;
#if 0
		// complete file entry
		text = gtk_editable_get_text (GTK_EDITABLE (dform->file_entry));
		if (text[0] == 0 || dform->changed.file == FALSE) {
			temp = ug_uri_get_file (&upart);
			gtk_editable_set_text (GTK_EDITABLE (dform->file_entry),
					(temp) ? temp : "index.htm");
			g_free (temp);
		}
		// complete user entry
		text = gtk_entry_get_text ((GtkEntry*) dform->username_entry);
		if (text[0] == 0 || dform->changed.user == FALSE) {
			temp = ug_uri_get_user (&upart);
			gtk_entry_set_text ((GtkEntry*) dform->username_entry,
					(temp) ? temp : "");
			g_free (temp);
		}
		// complete password entry
		text = gtk_entry_get_text ((GtkEntry*) dform->password_entry);
		if (text[0] == 0 || dform->changed.password == FALSE) {
			temp = ug_uri_get_password (&upart);
			gtk_entry_set_text ((GtkEntry*) dform->password_entry,
					(temp) ? temp : "");
			g_free (temp);
		}
#endif
		// enable changed flags
		dform->changed.enable = TRUE;
		// status
		completed = TRUE;
	}
#if 1    // check existing for file name
	else if (ug_uri_part_file (&upart, &text) > 0) {
		completed = TRUE;
	}
#else    // file extension
	else if (ug_uri_part_file_ext (&upart, &text) > 0) {
		// torrent or metalink file path
		if (*text == 'm' || *text == 'M' || *text == 't' || *text == 'T')
			completed = TRUE;
	}
#endif
	else if (upart.path > 0 && upart.uri[upart.path] != 0)
		completed = TRUE;
	else if (gtk_widget_is_sensitive (dform->uri_entry) == FALSE)
		completed = TRUE;

	dform->completed = completed;
}

// ----------------------------------------------------------------------------
// signal handler
static void on_spin_changed (GtkEditable* editable, UgtkDownloadForm* dform)
{
	if (dform->changed.enable) {
		if (editable == GTK_EDITABLE (dform->spin_retry))
			dform->changed.retry = TRUE;
		else if (editable == GTK_EDITABLE (dform->spin_delay))
			dform->changed.delay = TRUE;
	}
}

static void on_entry_changed (GtkEditable* editable, UgtkDownloadForm* dform)
{
	if (dform->changed.enable) {
		if (editable == GTK_EDITABLE (dform->file_entry))
			dform->changed.file = TRUE;
		else if (editable == GTK_EDITABLE (dform->folder_entry))
			dform->changed.folder = TRUE;
		else if (editable == GTK_EDITABLE (dform->username_entry))
			dform->changed.user = TRUE;
		else if (editable == GTK_EDITABLE (dform->password_entry))
			dform->changed.password = TRUE;
	}
}

static void on_uri_entry_changed (GtkEditable* editable, UgtkDownloadForm* dform)
{
	if (dform->changed.enable) {
		dform->changed.uri = TRUE;
		ugtk_download_form_complete_entry (dform);
	}
}

static void on_http_entry_changed (GtkEditable* editable, UgtkDownloadForm* dform)
{
	if (dform->changed.enable) {
		if (editable == GTK_EDITABLE (dform->referrer_entry))
			dform->changed.referrer = TRUE;
		else if (editable == GTK_EDITABLE (dform->cookie_entry))
			dform->changed.cookie = TRUE;
		else if (editable == GTK_EDITABLE (dform->post_entry))
			dform->changed.post = TRUE;
		else if (editable == GTK_EDITABLE (dform->agent_entry))
			dform->changed.agent = TRUE;
	}
}

static void on_select_folder_response (GtkDialog* chooser, gint response, UgtkDownloadForm* dform)
{
	gchar*	file;
	gchar*	path;

	if (response == GTK_RESPONSE_OK ) {
		GFile *gfile = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (chooser));
		if (gfile) {
			path = g_file_get_path (gfile);
			gtk_editable_set_text (GTK_EDITABLE (dform->folder_entry), path);
			g_free (path);
			g_object_unref (gfile);
		}
	}
	gtk_window_destroy (GTK_WINDOW (chooser));

	if (dform->parent)
		gtk_widget_set_sensitive ((GtkWidget*) dform->parent, TRUE);
}

static void on_select_folder (GtkEntry* entry, GtkEntryIconPosition icon_pos, GdkEvent* event, UgtkDownloadForm* dform)
{
	GtkWidget*	chooser;
	gchar*		path;
	gchar*		title;

	// disable sensitive of parent window
	// enable sensitive in function on_file_chooser_response()
	if (dform->parent)
		gtk_widget_set_sensitive ((GtkWidget*) dform->parent, FALSE);

	title = g_strconcat (UGTK_APP_NAME " - ", _("Select Folder"), NULL);
	chooser = gtk_file_chooser_dialog_new (title, dform->parent,
			GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_OK"),     GTK_RESPONSE_OK,
			NULL);
	g_free (title);
	gtk_window_set_transient_for ((GtkWindow*) chooser, dform->parent);
	gtk_window_set_destroy_with_parent ((GtkWindow*) chooser, TRUE);

	path = (gchar*) gtk_editable_get_text (GTK_EDITABLE (dform->folder_entry));
	if (*path) {
		GFile *gfile = g_file_new_for_path (path);
		gtk_file_chooser_set_file (GTK_FILE_CHOOSER (chooser), gfile, NULL);
		g_object_unref (gfile);
		g_free (path);
	}
	g_signal_connect (chooser, "response",
			G_CALLBACK (on_select_folder_response), dform);

	if (gtk_window_get_modal (dform->parent)) {
		// gtk_dialog_run ((GtkDialog*) chooser);
		gint response = GTK_RESPONSE_NONE;
		g_signal_connect (chooser, "response", G_CALLBACK (on_dialog_run_response), &response);
		gtk_widget_show (GTK_WIDGET (chooser));
		while (response == GTK_RESPONSE_NONE)
			g_main_context_iteration (NULL, TRUE);
	}
	else {
		gtk_window_set_modal ((GtkWindow*) chooser, FALSE);
		gtk_widget_show (GTK_WIDGET (chooser));
	}
}

static void on_select_cookie_response (GtkDialog* chooser, gint response, UgtkDownloadForm* dform)
{
	gchar*	file;
	gchar*	path;

	if (response == GTK_RESPONSE_OK ) {
		GFile *gfile = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (chooser));
		if (gfile) {
			path = g_file_get_path (gfile);
			gtk_editable_set_text (GTK_EDITABLE (dform->cookie_entry), path);
			g_free (path);
			g_object_unref (gfile);
		}
	}
	gtk_window_destroy (GTK_WINDOW (chooser));

	if (dform->parent)
		gtk_widget_set_sensitive ((GtkWidget*) dform->parent, TRUE);
}

static void on_select_cookie (GtkEntry* entry, GtkEntryIconPosition icon_pos, GdkEvent* event, UgtkDownloadForm* dform)
{
	GtkWidget*	chooser;
	gchar*		path;
	gchar*		title;

	// disable sensitive of parent window
	// enable sensitive in function on_file_chooser_response()
	if (dform->parent)
		gtk_widget_set_sensitive ((GtkWidget*) dform->parent, FALSE);

	title = g_strconcat (UGTK_APP_NAME " - ", _("Select Cookie File"), NULL);
	chooser = gtk_file_chooser_dialog_new (title, dform->parent,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_OK"),     GTK_RESPONSE_OK,
			NULL);
	g_free (title);
	gtk_window_set_transient_for ((GtkWindow*) chooser, dform->parent);
	gtk_window_set_destroy_with_parent ((GtkWindow*) chooser, TRUE);

	path = (gchar*) gtk_editable_get_text (GTK_EDITABLE (dform->cookie_entry));
	if (*path) {
		GFile *gfile = g_file_new_for_path (path);
		gtk_file_chooser_set_file (GTK_FILE_CHOOSER (chooser), gfile, NULL);
		g_object_unref (gfile);
		g_free (path);
	}
	g_signal_connect (chooser, "response",
			G_CALLBACK (on_select_cookie_response), dform);

	if (gtk_window_get_modal (dform->parent)) {
		// gtk_dialog_run ((GtkDialog*) chooser);
		gint response = GTK_RESPONSE_NONE;
		g_signal_connect (chooser, "response", G_CALLBACK (on_dialog_run_response), &response);
		gtk_widget_show (GTK_WIDGET (chooser));
		while (response == GTK_RESPONSE_NONE)
			g_main_context_iteration (NULL, TRUE);
	}
	else {
		gtk_window_set_modal ((GtkWindow*) chooser, FALSE);
		// gtk_widget_show (chooser);
	}
}

static void on_select_post_response (GtkDialog* chooser, gint response, UgtkDownloadForm* dform)
{
	gchar*	file;
	gchar*	path;

	if (response == GTK_RESPONSE_OK ) {
		GFile *gfile = gtk_file_chooser_get_file (GTK_FILE_CHOOSER (chooser));
		if (gfile) {
			path = g_file_get_path (gfile);
			gtk_editable_set_text (GTK_EDITABLE (dform->post_entry), path);
			g_free (path);
			g_object_unref (gfile);
		}
	}
	gtk_window_destroy (GTK_WINDOW (chooser));

	if (dform->parent)
		gtk_widget_set_sensitive ((GtkWidget*) dform->parent, TRUE);
}

static void on_select_post (GtkEntry* entry, GtkEntryIconPosition icon_pos, GdkEvent* event, UgtkDownloadForm* dform)
{
	GtkWidget*	chooser;
	gchar*		path;
	gchar*		title;

	// disable sensitive of parent window
	// enable sensitive in function on_file_chooser_response()
	if (dform->parent)
		gtk_widget_set_sensitive ((GtkWidget*) dform->parent, FALSE);

	title = g_strconcat (UGTK_APP_NAME " - ", _("Select Post File"), NULL);
	chooser = gtk_file_chooser_dialog_new (title, dform->parent,
			GTK_FILE_CHOOSER_ACTION_OPEN,
			_("_Cancel"), GTK_RESPONSE_CANCEL,
			_("_OK"),     GTK_RESPONSE_OK,
			NULL);
	g_free (title);
	gtk_window_set_transient_for ((GtkWindow*) chooser, dform->parent);
	gtk_window_set_destroy_with_parent ((GtkWindow*) chooser, TRUE);

	path = (gchar*) gtk_editable_get_text (GTK_EDITABLE (dform->post_entry));
	if (*path) {
		GFile *gfile = g_file_new_for_path (path);
		gtk_file_chooser_set_file (GTK_FILE_CHOOSER (chooser), gfile, NULL);
		g_object_unref (gfile);
		g_free (path);
	}
	g_signal_connect (chooser, "response",
			G_CALLBACK (on_select_post_response), dform);

	if (gtk_window_get_modal (dform->parent)) {
		// gtk_dialog_run ((GtkDialog*) chooser);
		gint response = GTK_RESPONSE_NONE;
		g_signal_connect (chooser, "response", G_CALLBACK (on_dialog_run_response), &response);
		gtk_widget_show (GTK_WIDGET (chooser));
		while (response == GTK_RESPONSE_NONE)
			g_main_context_iteration (NULL, TRUE);
	}
	else {
		gtk_window_set_modal ((GtkWindow*) chooser, FALSE);
		gtk_widget_show (GTK_WIDGET (chooser));
	}
}

