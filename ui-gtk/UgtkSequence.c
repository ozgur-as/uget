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

#include <UgtkSequence.h>
#include <UgUri.h>

#include <glib/gi18n.h>

// ----------------------------------------------------------------------------
// UgtkSeqRange
static void on_type_changed (GtkComboBox* widget, UgtkSeqRange* range);
static void on_show (GtkWidget *widget, UgtkSeqRange* range);

void   ugtk_seq_range_init (UgtkSeqRange* range, UgtkSequence* seq, GtkSizeGroup* size_group)
{
	GtkBox*        box;
	GtkAdjustment* adjustment;

	range->self = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 3);
	box = (GtkBox*) range->self;
	g_signal_connect (range->self, "show",
			G_CALLBACK (on_show), range);
	// Type
	range->type = gtk_combo_box_text_new ();
	gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (range->type),
			UGTK_SEQ_TYPE_NONE, _("None"));
	gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (range->type),
			UGTK_SEQ_TYPE_NUMBER, _("Num"));
	gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (range->type),
			UGTK_SEQ_TYPE_CHARACTER, _("Char"));
	gtk_box_append (box, range->type);
	// gtk_box_pack_start (box, range->type, FALSE, FALSE, 2);
	g_signal_connect (range->type, "changed",
			G_CALLBACK (on_type_changed), range);
	g_signal_connect_swapped (range->type, "changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);

	// SpinButton - From
	adjustment = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0,
				99999.0, 1.0, 5.0, 0.0);
	range->spin_from = gtk_spin_button_new (adjustment, 1.0, 0);
	gtk_size_group_add_widget (size_group, range->spin_from);
	gtk_box_append (box, range->spin_from);
	// gtk_box_pack_start (box, range->spin_from, FALSE, FALSE, 2);
	g_signal_connect_swapped (range->spin_from, "value-changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);
	// Entry - From
	range->entry_from = gtk_entry_new ();
	gtk_editable_set_text (GTK_EDITABLE (range->entry_from), "a");
	gtk_entry_set_max_length (GTK_ENTRY (range->entry_from), 1);
//	gtk_entry_set_width_chars (GTK_ENTRY (range->entry_from), 2);
	gtk_size_group_add_widget (size_group, range->entry_from);
	gtk_box_append (box, range->entry_from);
	// gtk_box_pack_start (box, range->entry_from, FALSE, FALSE, 2);
	g_signal_connect_swapped (GTK_EDITABLE (range->entry_from), "changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);

	// Label - To
	range->label_to = gtk_label_new (_("To:"));
//	range->label_to = gtk_label_new_with_mnemonic (_("To:"));
	gtk_box_append (box, range->label_to);
	// gtk_box_pack_start (box, range->label_to, FALSE, FALSE, 2);
//	gtk_label_set_mnemonic_widget (GTK_LABEL (widget), range->spin_to);

	// SpinButton - To
	adjustment = (GtkAdjustment *) gtk_adjustment_new (10.0, 1.0,
				99999.0, 1.0, 5.0, 0.0);
	range->spin_to = gtk_spin_button_new (adjustment, 1.0, 0);
	gtk_box_append (box, range->spin_to);
	// gtk_box_pack_start (box, range->spin_to, FALSE, FALSE, 2);
	gtk_size_group_add_widget (size_group, range->spin_to);
	g_signal_connect_swapped (range->spin_to, "value-changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);

	// label - digits
	range->label_digits = gtk_label_new (_("digits:"));
//	range->label_digits = gtk_label_new_with_mnemonic (_("digits:"));
	gtk_box_append (box, range->label_digits);
	// gtk_box_pack_start (box, range->label_digits, FALSE, FALSE, 2);
//	gtk_label_set_mnemonic_widget (GTK_LABEL (range->label_digits), range->spin_digits);

	// SpinButton - digits
	adjustment = (GtkAdjustment *) gtk_adjustment_new (2.0, 1.0,
			20.0, 1.0, 5.0, 0.0);
	range->spin_digits = gtk_spin_button_new (adjustment, 1.0, 0);
	gtk_box_append (box, range->spin_digits);
	// gtk_box_pack_start (box, range->spin_digits, FALSE, FALSE, 2);
	g_signal_connect_swapped (range->spin_digits, "value-changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);

	// Entry - To
	range->entry_to = gtk_entry_new ();
	gtk_editable_set_text (GTK_EDITABLE (range->entry_to), "z");
	gtk_entry_set_max_length (GTK_ENTRY (range->entry_to), 1);
//	gtk_entry_set_width_chars (GTK_ENTRY (range->entry_to), 2);
	gtk_size_group_add_widget (size_group, range->entry_to);
	gtk_box_append (box, range->entry_to);
	// gtk_box_pack_start (box, range->entry_to, FALSE, FALSE, 2);
	g_signal_connect_swapped (GTK_EDITABLE (range->entry_to), "changed",
			G_CALLBACK(ugtk_sequence_show_preview), seq);

	// label - case-sensitive
	range->label_case = gtk_label_new (_("case-sensitive"));
	gtk_box_append (box, range->label_case);
	// gtk_box_pack_start (box, range->label_case, FALSE, FALSE, 2);

//	gtk_widget_show_all (range->self);
}

void   ugtk_seq_range_set_type (UgtkSeqRange* range, enum UgtkSeqType type)
{
	gtk_combo_box_set_active ((GtkComboBox*) range->type, type);
}

enum UgtkSeqType  ugtk_seq_range_get_type (UgtkSeqRange* range)
{
	return gtk_combo_box_get_active ((GtkComboBox*) range->type);
}

// signal handler
static void on_show (GtkWidget *widget, UgtkSeqRange* range)
{
	ugtk_seq_range_set_type (range, UGTK_SEQ_TYPE_NONE);
}

// signal handler
static void on_type_changed (GtkComboBox* widget, UgtkSeqRange* range)
{
	gint      type;

	type = gtk_combo_box_get_active (widget);
	switch (type) {
	case UGTK_SEQ_TYPE_NONE:
		gtk_widget_set_sensitive (range->label_to, FALSE);
		gtk_widget_set_sensitive (range->spin_from, FALSE);
		gtk_widget_set_sensitive (range->spin_to, FALSE);
		gtk_widget_set_sensitive (range->spin_digits, FALSE);
		gtk_widget_set_sensitive (range->label_digits, FALSE);
		gtk_widget_set_sensitive (range->entry_from, FALSE);
		gtk_widget_set_sensitive (range->entry_to, FALSE);
		gtk_widget_set_sensitive (range->label_case, FALSE);
		if (gtk_widget_get_visible (range->spin_from) == TRUE) {
			gtk_widget_set_visible (range->entry_from, FALSE);
			gtk_widget_set_visible (range->entry_to, FALSE);
			gtk_widget_set_visible (range->label_case, FALSE);
		}
		break;

	case UGTK_SEQ_TYPE_NUMBER:
		gtk_widget_set_sensitive (range->spin_from, TRUE);
		gtk_widget_set_sensitive (range->label_to, TRUE);
		gtk_widget_set_sensitive (range->spin_to, TRUE);
		gtk_widget_set_sensitive (range->spin_digits, TRUE);
		gtk_widget_set_sensitive (range->label_digits, TRUE);
		gtk_widget_set_visible (range->spin_from, TRUE);
		gtk_widget_set_visible (range->spin_to, TRUE);
		gtk_widget_set_visible (range->spin_digits, TRUE);
		gtk_widget_set_visible (range->label_digits, TRUE);
		gtk_widget_set_visible (range->entry_from, FALSE);
		gtk_widget_set_visible (range->entry_to, FALSE);
		gtk_widget_set_visible (range->label_case, FALSE);
		break;

	case UGTK_SEQ_TYPE_CHARACTER:
		gtk_widget_set_sensitive (range->entry_from, TRUE);
		gtk_widget_set_sensitive (range->label_to, TRUE);
		gtk_widget_set_sensitive (range->entry_to, TRUE);
		gtk_widget_set_sensitive (range->label_case, TRUE);
		gtk_widget_set_visible (range->spin_from, FALSE);
		gtk_widget_set_visible (range->spin_to, FALSE);
		gtk_widget_set_visible (range->spin_digits, FALSE);
		gtk_widget_set_visible (range->label_digits, FALSE);
		gtk_widget_set_visible (range->entry_from, TRUE);
		gtk_widget_set_visible (range->entry_to, TRUE);
		gtk_widget_set_visible (range->label_case, TRUE);
		break;
	}
}

// ---------------------------------------------------------------------------
// UgtkSequence

// static functions
static void ugtk_sequence_add_range (UgtkSequence* seq, UgtkSeqRange* range);
static void ugtk_sequence_preview_init (struct UgtkSequencePreview* preview);
static void ugtk_sequence_preview_show (struct UgtkSequencePreview* preview, const gchar* message);
// signal handlers
static void on_realize (GtkWidget *widget, UgtkSequence* seq);
static void on_destroy (GtkWidget *widget, UgtkSequence* seq);

void  ugtk_sequence_init (UgtkSequence* seq)
{
	GtkGrid*        grid;
	GtkWidget*      label;
	GtkWidget*      entry;
	GtkWidget*      widget;
	GtkSizeGroup*   size_group;

	// UgetSequence: call uget_sequence_final() in on_destroy()
	uget_sequence_init (&seq->sequence);

	// top widget
	seq->self = gtk_grid_new ();
	grid = (GtkGrid*) seq->self;
	gtk_grid_set_row_homogeneous (grid, FALSE);
	g_signal_connect (seq->self, "realize", G_CALLBACK (on_realize), seq);
	g_signal_connect (seq->self, "destroy", G_CALLBACK (on_destroy), seq);

	// URI entry
	entry = gtk_entry_new ();
	label = gtk_label_new_with_mnemonic (_("_URI:"));
	seq->entry = GTK_ENTRY (entry);
	gtk_label_set_mnemonic_widget(GTK_LABEL (label), entry);
	gtk_entry_set_activates_default (seq->entry, TRUE);
	g_object_set (label, "margin", 3, NULL);
	g_object_set (entry, "margin", 3, "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, label, 0, 0, 1, 1);
	gtk_grid_attach (grid, entry, 1, 0, 1, 1);
	g_signal_connect_swapped (GTK_EDITABLE (entry), "changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);
	// e.g.
	label = gtk_label_new (_("e.g."));
	g_object_set (label, "margin", 3, NULL);
	gtk_grid_attach (grid, label, 0, 1, 1, 1);
	label = gtk_label_new ("http://for.example/path/pre*.jpg");
	g_object_set (label, "margin", 3, NULL);
	gtk_grid_attach (grid, label, 1, 1, 1, 1);
	// separator
	widget = gtk_separator_new (GTK_ORIENTATION_HORIZONTAL);
	gtk_grid_attach (grid, widget, 0, 2, 2, 1);

	// ------------------------------------------------
	// UgtkSeqRange
	size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);
	ugtk_seq_range_init (&seq->range[0], seq, size_group);
	ugtk_seq_range_init (&seq->range[1], seq, size_group);
	ugtk_seq_range_init (&seq->range[2], seq, size_group);
	g_object_set (seq->range[0].self, "margin", 3, NULL);
	g_object_set (seq->range[1].self, "margin", 3, NULL);
	g_object_set (seq->range[2].self, "margin", 3, NULL);
	gtk_grid_attach (grid, seq->range[0].self, 0, 3, 2, 1);
	gtk_grid_attach (grid, seq->range[1].self, 0, 4, 2, 1);
	gtk_grid_attach (grid, seq->range[2].self, 0, 5, 2, 1);
	g_object_unref (size_group);

	// ------------------------------------------------
	// preview
	ugtk_sequence_preview_init (&seq->preview);
	g_object_set (seq->preview.self, "margin", 3, "expand", TRUE, NULL);
	gtk_grid_attach (grid, seq->preview.self, 0, 6, 2, 1);

	ugtk_sequence_show_preview (seq);
	gtk_widget_show (seq->self);
}

void  ugtk_sequence_show_preview (UgtkSequence* seq)
{
	GtkTreeIter  iter;
	UgList       result;
	UgLink*      link;
	const char*  string;

	uget_sequence_clear (&seq->sequence);
	ugtk_sequence_add_range (seq, &seq->range[0]);
	ugtk_sequence_add_range (seq, &seq->range[1]);
	ugtk_sequence_add_range (seq, &seq->range[2]);

	string = gtk_editable_get_text (GTK_EDITABLE (seq->entry));
	if (ug_uri_init (NULL, string) == 0) {
		ugtk_sequence_preview_show (&seq->preview,
				_("URI is not valid."));
		// notify
		if (seq->notify.func)
			seq->notify.func (seq->notify.data, FALSE);
		return;
	}
	if (strpbrk (string, "*") == NULL) {
		ugtk_sequence_preview_show (&seq->preview,
				_("No wildcard(*) character in URI entry."));
		// notify
		if (seq->notify.func)
			seq->notify.func (seq->notify.data, FALSE);
		return;
	}
	if (uget_sequence_count (&seq->sequence, string) == 0) {
		ugtk_sequence_preview_show (&seq->preview,
				_("No character in 'From' or 'To' entry."));
		// notify
		if (seq->notify.func)
			seq->notify.func (seq->notify.data, FALSE);
		return;
	}

	ug_list_init (&result);
	uget_sequence_get_preview (&seq->sequence, string, &result);

	gtk_list_store_clear (seq->preview.store);
	for (link = result.head;  link;  link = link->next) {
		gtk_list_store_append (seq->preview.store, &iter);
		gtk_list_store_set (seq->preview.store, &iter, 0, link->data, -1);
	}
	uget_sequence_clear_result(&result);
	// notify
	if (seq->notify.func)
		seq->notify.func (seq->notify.data, TRUE);
	return;
}

int  ugtk_sequence_get_list (UgtkSequence* seq, UgList* result)
{
	const char*  string;

	string = gtk_editable_get_text (GTK_EDITABLE (seq->entry));
	return uget_sequence_get_list (&seq->sequence, string, result);
}

// ----------------------------------------------------------------------------
//	static functions
//
static void ugtk_sequence_add_range (UgtkSequence* seq, UgtkSeqRange* range)
{
	uint32_t  first, last;
	int       type, digits;

	type = ugtk_seq_range_get_type (range);
	switch (type) {
	case UGTK_SEQ_TYPE_NUMBER:
		first  = gtk_spin_button_get_value_as_int ((GtkSpinButton*) range->spin_from);
		last   = gtk_spin_button_get_value_as_int ((GtkSpinButton*) range->spin_to);
		digits = gtk_spin_button_get_value_as_int ((GtkSpinButton*) range->spin_digits);
		break;

	case UGTK_SEQ_TYPE_CHARACTER:
		first  = *gtk_editable_get_text (GTK_EDITABLE (range->entry_from));
		last   = *gtk_editable_get_text (GTK_EDITABLE (range->entry_to));
		digits = 0;
		break;

	default:
		return;
	}

	uget_sequence_add (&seq->sequence, first, last, digits);
}

static void ugtk_sequence_preview_init (struct UgtkSequencePreview* preview)
{
	GtkScrolledWindow*	scrolled;
	GtkCellRenderer*	renderer;
	GtkTreeViewColumn*	column;
	GtkTreeSelection*	selection;
	PangoContext*  context;
	PangoLayout*   layout;
	int            height;

	preview->view  = (GtkTreeView*) gtk_tree_view_new ();
	preview->store = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_tree_view_set_model (preview->view, (GtkTreeModel*) preview->store);
//	gtk_tree_view_set_fixed_height_mode (preview->view, TRUE);
	gtk_widget_set_size_request ((GtkWidget*) preview->view, 140, 140);
	selection = gtk_tree_view_get_selection (preview->view);
	gtk_tree_selection_set_mode (selection, GTK_SELECTION_NONE);
	// It will free UgtkSequence.preview_store when UgtkSequence.preview_view destroy.
	g_object_unref (preview->store);

	renderer = gtk_cell_renderer_text_new ();
	column   = gtk_tree_view_column_new_with_attributes (
			_("Preview"), renderer, "text", 0, NULL);
//	gtk_tree_view_column_set_sizing (column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_append_column (preview->view, column);

	// calc text height
	context = gtk_widget_get_pango_context ((GtkWidget*)preview->view);
	layout = pango_layout_new (context);
	pango_layout_set_text (layout, "Xy", -1);
	pango_layout_get_pixel_size (layout, NULL, &height);
	g_object_unref (layout);
	height *= 10;
	if (height < 140)
		height = 140;

	preview->self = gtk_scrolled_window_new ();
	gtk_widget_set_size_request (preview->self, 140, height);
	scrolled = GTK_SCROLLED_WINDOW (preview->self);
	// gtk_scrolled_window_set_shadow_type (scrolled, GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (scrolled,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (scrolled,
			GTK_WIDGET (preview->view));
}

static void ugtk_sequence_preview_show (struct UgtkSequencePreview* preview, const gchar* message)
{
	GtkTreeIter		iter;

	gtk_list_store_clear (preview->store);
	// skip first row
	gtk_list_store_append (preview->store, &iter);
	// show message in second row
	gtk_list_store_append (preview->store, &iter);
	gtk_list_store_set (preview->store, &iter, 0, message, -1);
}

// ----------------------------------------------------------------------------
// signal handler
static void on_realize (GtkWidget *widget, UgtkSequence* seq)
{
	ugtk_seq_range_set_type (&seq->range[0], UGTK_SEQ_TYPE_NUMBER);
}

static void on_destroy (GtkWidget *widget, UgtkSequence* seq)
{
	uget_sequence_final (&seq->sequence);
}


