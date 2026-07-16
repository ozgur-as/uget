/*
 *
 *   Copyright (C) 2026 by Ozgur As
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>

#include <UgString.h>
#include <UgtkFormatPicker.h>

typedef struct {
	GtkWindow*                self;
	GtkListBox*               list_box;
	UgetResolverResult*       result;
	UgetFormat*               picked;     // points to a format inside result
	UgtkFormatPickerCallback  callback;
	void*                     user_data;
} FormatPicker;

// ----------------------------------------------------------------------------
// Format presentation helpers

// Returns TRUE if a format is a real downloadable stream (skip storyboards
// and any other non-media entries).
static gboolean format_is_real (UgetFormat* f)
{
	gboolean has_v = (f->vcodec && strcmp (f->vcodec, "none") != 0);
	gboolean has_a = (f->acodec && strcmp (f->acodec, "none") != 0);
	return has_v || has_a;
}

static char* format_describe (UgetFormat* f)
{
	GString* s = g_string_new (NULL);
	gboolean has_v = (f->vcodec && strcmp (f->vcodec, "none") != 0);
	gboolean has_a = (f->acodec && strcmp (f->acodec, "none") != 0);

	// Kind
	if (has_v && has_a)
		g_string_append (s, "🎬 ");
	else if (has_v)
		g_string_append (s, "🎞 ");
	else if (has_a)
		g_string_append (s, "🎵 ");

	// Resolution
	if (f->height > 0) {
		g_string_append_printf (s, "%dp", f->height);
		if (f->fps > 0)
			g_string_append_printf (s, "%d", f->fps);
		g_string_append_c (s, ' ');
	}

	// Container
	if (f->ext)
		g_string_append_printf (s, "%s ", f->ext);

	// Codecs
	if (has_v && f->vcodec) {
		const char* dot = strchr (f->vcodec, '.');
		int len = dot ? (int)(dot - f->vcodec) : (int) strlen (f->vcodec);
		g_string_append_printf (s, "%.*s ", len, f->vcodec);
	}
	if (has_a && f->acodec) {
		const char* dot = strchr (f->acodec, '.');
		int len = dot ? (int)(dot - f->acodec) : (int) strlen (f->acodec);
		g_string_append_printf (s, "%.*s ", len, f->acodec);
	}

	// Audio-only kbps
	if (!has_v && has_a && f->tbr > 0)
		g_string_append_printf (s, "@%dkbps ", f->tbr);

	// Filesize
	if (f->filesize > 0) {
		double mb = f->filesize / (1024.0 * 1024.0);
		if (mb >= 1024.0)
			g_string_append_printf (s, "(%.2f GiB)", mb / 1024.0);
		else if (mb >= 1.0)
			g_string_append_printf (s, "(%.1f MiB)", mb);
		else
			g_string_append_printf (s, "(%.0f KiB)", f->filesize / 1024.0);
	}

	return g_string_free (s, FALSE);
}

// ----------------------------------------------------------------------------
// Picker lifecycle

static void picker_finish (FormatPicker* picker, UgetFormat* picked)
{
	UgtkFormatPickerCallback cb = picker->callback;
	void* data = picker->user_data;

	gtk_window_destroy (picker->self);

	if (cb)
		cb (picked, data);

	g_free (picker);
}

static void on_ok_clicked (GtkButton* button, FormatPicker* picker)
{
	GtkListBoxRow* row;
	UgetFormat*    f;

	row = gtk_list_box_get_selected_row (picker->list_box);
	if (row == NULL) {
		picker_finish (picker, NULL);
		return;
	}

	f = g_object_get_data (G_OBJECT (row), "uget-format");
	picker_finish (picker, f);
}

static void on_cancel_clicked (GtkButton* button, FormatPicker* picker)
{
	picker_finish (picker, NULL);
}

static gboolean on_close_request (GtkWindow* window, FormatPicker* picker)
{
	picker_finish (picker, NULL);
	return TRUE;
}

static void on_row_activated (GtkListBox* box, GtkListBoxRow* row, FormatPicker* picker)
{
	UgetFormat* f = g_object_get_data (G_OBJECT (row), "uget-format");
	picker_finish (picker, f);
}

// ----------------------------------------------------------------------------
// Public entry point

void ugtk_format_picker_show (GtkWindow*               parent,
                              UgetResolverResult*      result,
                              UgtkFormatPickerCallback callback,
                              void*                    user_data)
{
	FormatPicker* picker;
	GtkWidget*    vbox;
	GtkWidget*    title_label;
	GtkWidget*    scrolled;
	GtkWidget*    list_box;
	GtkWidget*    button_box;
	GtkWidget*    ok_button;
	GtkWidget*    cancel_button;
	UgetFormat*   f;

	picker = g_new0 (FormatPicker, 1);
	picker->result = result;
	picker->callback = callback;
	picker->user_data = user_data;

	// Window
	picker->self = GTK_WINDOW (gtk_window_new ());
	gtk_window_set_title (picker->self,
			result->title ? result->title : _("Select media format"));
	gtk_window_set_default_size (picker->self, 560, 420);
	gtk_window_set_modal (picker->self, TRUE);
	if (parent)
		gtk_window_set_transient_for (picker->self, parent);
	g_signal_connect (picker->self, "close-request",
			G_CALLBACK (on_close_request), picker);

	// Layout
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	g_object_set (vbox, "margin-start", 12, "margin-end", 12,
	                    "margin-top", 12,   "margin-bottom", 12, NULL);
	gtk_window_set_child (picker->self, vbox);

	// Title row
	if (result->title) {
		title_label = gtk_label_new (result->title);
		gtk_label_set_xalign (GTK_LABEL (title_label), 0.0);
		gtk_label_set_ellipsize (GTK_LABEL (title_label), PANGO_ELLIPSIZE_END);
		gtk_widget_add_css_class (title_label, "title-4");
		gtk_box_append (GTK_BOX (vbox), title_label);
	}

	// Scrolled list
	scrolled = gtk_scrolled_window_new ();
	gtk_widget_set_vexpand (scrolled, TRUE);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
			GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_box_append (GTK_BOX (vbox), scrolled);

	list_box = gtk_list_box_new ();
	picker->list_box = GTK_LIST_BOX (list_box);
	gtk_list_box_set_selection_mode (picker->list_box, GTK_SELECTION_BROWSE);
	gtk_widget_add_css_class (list_box, "boxed-list");
	gtk_scrolled_window_set_child (GTK_SCROLLED_WINDOW (scrolled), list_box);
	g_signal_connect (list_box, "row-activated",
			G_CALLBACK (on_row_activated), picker);

	// Populate rows
	for (f = (UgetFormat*) result->formats.head; f; f = f->next) {
		GtkWidget* row;
		GtkWidget* label;
		char*      desc;

		if (!format_is_real (f))
			continue;

		desc = format_describe (f);
		label = gtk_label_new (desc);
		gtk_label_set_xalign (GTK_LABEL (label), 0.0);
		gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
		g_object_set (label, "margin-start", 12, "margin-end", 12,
		                     "margin-top", 8,    "margin-bottom", 8, NULL);
		ug_free (desc);

		row = gtk_list_box_row_new ();
		gtk_list_box_row_set_child (GTK_LIST_BOX_ROW (row), label);
		g_object_set_data (G_OBJECT (row), "uget-format", f);
		gtk_list_box_append (picker->list_box, row);
	}

	// Auto-select first row so the user can hit Enter immediately
	{
		GtkListBoxRow* first = gtk_list_box_get_row_at_index (picker->list_box, 0);
		if (first)
			gtk_list_box_select_row (picker->list_box, first);
	}

	// Button row
	button_box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_halign (button_box, GTK_ALIGN_END);
	gtk_box_append (GTK_BOX (vbox), button_box);

	cancel_button = gtk_button_new_with_mnemonic (_("_Cancel"));
	gtk_box_append (GTK_BOX (button_box), cancel_button);
	g_signal_connect (cancel_button, "clicked",
			G_CALLBACK (on_cancel_clicked), picker);

	ok_button = gtk_button_new_with_mnemonic (_("_Download"));
	gtk_widget_add_css_class (ok_button, "suggested-action");
	gtk_box_append (GTK_BOX (button_box), ok_button);
	g_signal_connect (ok_button, "clicked",
			G_CALLBACK (on_ok_clicked), picker);

	gtk_window_present (picker->self);
}
