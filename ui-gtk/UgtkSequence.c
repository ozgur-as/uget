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

static inline void set_margin_all(GtkWidget* w, int m) {
	gtk_widget_set_margin_start(w, m);
	gtk_widget_set_margin_end(w, m);
	gtk_widget_set_margin_top(w, m);
	gtk_widget_set_margin_bottom(w, m);
}

// ----------------------------------------------------------------------------
// UgtkSeqRange
static void on_type_changed (GObject* object, GParamSpec* pspec, UgtkSeqRange* range);
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
	{
		const char* seq_types[] = { _("None"), _("Num"), _("Char"), NULL };
		range->type = (GtkWidget*) gtk_drop_down_new_from_strings (seq_types);
	}
	gtk_box_append (box, range->type);
	g_signal_connect (range->type, "notify::selected",
			G_CALLBACK (on_type_changed), range);
	g_signal_connect_swapped (range->type, "notify::selected",
			G_CALLBACK (ugtk_sequence_show_preview), seq);

	// SpinButton - From
	adjustment = (GtkAdjustment *) gtk_adjustment_new (0.0, 0.0,
				99999.0, 1.0, 5.0, 0.0);
	range->spin_from = gtk_spin_button_new (adjustment, 1.0, 0);
	gtk_size_group_add_widget (size_group, range->spin_from);
	gtk_box_append (box, range->spin_from);
	g_signal_connect_swapped (range->spin_from, "value-changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);
	// Entry - From
	range->entry_from = gtk_entry_new ();
	gtk_editable_set_text (GTK_EDITABLE (range->entry_from), "a");
	gtk_entry_set_max_length (GTK_ENTRY (range->entry_from), 1);
	gtk_size_group_add_widget (size_group, range->entry_from);
	gtk_box_append (box, range->entry_from);
	g_signal_connect_swapped (GTK_EDITABLE (range->entry_from), "changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);

	// Label - To
	range->label_to = gtk_label_new (_("To:"));
	gtk_box_append (box, range->label_to);

	// SpinButton - To
	adjustment = (GtkAdjustment *) gtk_adjustment_new (10.0, 1.0,
				99999.0, 1.0, 5.0, 0.0);
	range->spin_to = gtk_spin_button_new (adjustment, 1.0, 0);
	gtk_box_append (box, range->spin_to);
	gtk_size_group_add_widget (size_group, range->spin_to);
	g_signal_connect_swapped (range->spin_to, "value-changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);

	// label - digits
	range->label_digits = gtk_label_new (_("digits:"));
	gtk_box_append (box, range->label_digits);

	// SpinButton - digits
	adjustment = (GtkAdjustment *) gtk_adjustment_new (2.0, 1.0,
			20.0, 1.0, 5.0, 0.0);
	range->spin_digits = gtk_spin_button_new (adjustment, 1.0, 0);
	gtk_box_append (box, range->spin_digits);
	g_signal_connect_swapped (range->spin_digits, "value-changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);

	// Entry - To
	range->entry_to = gtk_entry_new ();
	gtk_editable_set_text (GTK_EDITABLE (range->entry_to), "z");
	gtk_entry_set_max_length (GTK_ENTRY (range->entry_to), 1);
	gtk_size_group_add_widget (size_group, range->entry_to);
	gtk_box_append (box, range->entry_to);
	g_signal_connect_swapped (GTK_EDITABLE (range->entry_to), "changed",
			G_CALLBACK(ugtk_sequence_show_preview), seq);

	// label - case-sensitive
	range->label_case = gtk_label_new (_("case-sensitive"));
	gtk_box_append (box, range->label_case);
}

void   ugtk_seq_range_set_type (UgtkSeqRange* range, enum UgtkSeqType type)
{
	gtk_drop_down_set_selected (GTK_DROP_DOWN (range->type), type);
}

enum UgtkSeqType  ugtk_seq_range_get_type (UgtkSeqRange* range)
{
	return gtk_drop_down_get_selected (GTK_DROP_DOWN (range->type));
}

// signal handler
static void on_show (GtkWidget *widget, UgtkSeqRange* range)
{
	ugtk_seq_range_set_type (range, UGTK_SEQ_TYPE_NONE);
}

// signal handler
static void on_type_changed (GObject* object, GParamSpec* pspec, UgtkSeqRange* range)
{
	gint      type;

	type = gtk_drop_down_get_selected (GTK_DROP_DOWN (object));
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
	set_margin_all (label, 3);
	set_margin_all (entry, 3);
	g_object_set (entry, "hexpand", TRUE, NULL);
	gtk_grid_attach (grid, label, 0, 0, 1, 1);
	gtk_grid_attach (grid, entry, 1, 0, 1, 1);
	g_signal_connect_swapped (GTK_EDITABLE (entry), "changed",
			G_CALLBACK (ugtk_sequence_show_preview), seq);
	// e.g.
	label = gtk_label_new (_("e.g."));
	set_margin_all (label, 3);
	gtk_grid_attach (grid, label, 0, 1, 1, 1);
	label = gtk_label_new ("http://for.example/path/pre*.jpg");
	set_margin_all (label, 3);
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
	set_margin_all (seq->range[0].self, 3);
	set_margin_all (seq->range[1].self, 3);
	set_margin_all (seq->range[2].self, 3);
	gtk_grid_attach (grid, seq->range[0].self, 0, 3, 2, 1);
	gtk_grid_attach (grid, seq->range[1].self, 0, 4, 2, 1);
	gtk_grid_attach (grid, seq->range[2].self, 0, 5, 2, 1);
	g_object_unref (size_group);

	// ------------------------------------------------
	// preview
	ugtk_sequence_preview_init (&seq->preview);
	set_margin_all (seq->preview.self, 3);
	g_object_set (seq->preview.self, "hexpand", TRUE, "vexpand", TRUE, NULL);
	gtk_grid_attach (grid, seq->preview.self, 0, 6, 2, 1);

	ugtk_sequence_show_preview (seq);
	gtk_widget_set_visible(seq->self, TRUE);
}

void  ugtk_sequence_show_preview (UgtkSequence* seq)
{
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
		if (seq->notify.func)
			seq->notify.func (seq->notify.data, FALSE);
		return;
	}
	if (strpbrk (string, "*") == NULL) {
		ugtk_sequence_preview_show (&seq->preview,
				_("No wildcard(*) character in URI entry."));
		if (seq->notify.func)
			seq->notify.func (seq->notify.data, FALSE);
		return;
	}
	if (uget_sequence_count (&seq->sequence, string) == 0) {
		ugtk_sequence_preview_show (&seq->preview,
				_("No character in 'From' or 'To' entry."));
		if (seq->notify.func)
			seq->notify.func (seq->notify.data, FALSE);
		return;
	}

	ug_list_init (&result);
	uget_sequence_get_preview (&seq->sequence, string, &result);

	// Clear and repopulate the string list
	gtk_string_list_splice (seq->preview.store, 0,
			g_list_model_get_n_items (G_LIST_MODEL (seq->preview.store)), NULL);
	for (link = result.head;  link;  link = link->next)
		gtk_string_list_append (seq->preview.store, link->data);

	uget_sequence_clear_result(&result);
	if (seq->notify.func)
		seq->notify.func (seq->notify.data, TRUE);
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

// Factory callbacks for preview list
static void preview_setup_cb (GtkSignalListItemFactory* factory,
                               GtkListItem* list_item, gpointer user_data)
{
	GtkWidget* label = gtk_label_new (NULL);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_list_item_set_child (list_item, label);
}

static void preview_bind_cb (GtkSignalListItemFactory* factory,
                              GtkListItem* list_item, gpointer user_data)
{
	GtkStringObject* obj = gtk_list_item_get_item (list_item);
	GtkWidget* label = gtk_list_item_get_child (list_item);
	gtk_label_set_text (GTK_LABEL (label), gtk_string_object_get_string (obj));
}

static void ugtk_sequence_preview_init (struct UgtkSequencePreview* preview)
{
	GtkScrolledWindow*          scrolled;
	GtkSignalListItemFactory*   factory;
	GtkNoSelection*             no_sel;

	preview->store = gtk_string_list_new (NULL);

	factory = GTK_SIGNAL_LIST_ITEM_FACTORY (gtk_signal_list_item_factory_new ());
	g_signal_connect (factory, "setup", G_CALLBACK (preview_setup_cb), NULL);
	g_signal_connect (factory, "bind", G_CALLBACK (preview_bind_cb), NULL);

	no_sel = gtk_no_selection_new (G_LIST_MODEL (preview->store));
	preview->view = GTK_LIST_VIEW (gtk_list_view_new (GTK_SELECTION_MODEL (no_sel),
	                               GTK_LIST_ITEM_FACTORY (factory)));
	gtk_widget_set_size_request (GTK_WIDGET (preview->view), 140, 140);

	preview->self = gtk_scrolled_window_new ();
	gtk_widget_set_size_request (preview->self, 140, 140);
	scrolled = GTK_SCROLLED_WINDOW (preview->self);
	gtk_scrolled_window_set_policy (scrolled,
			GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_scrolled_window_set_child (scrolled,
			GTK_WIDGET (preview->view));
}

static void ugtk_sequence_preview_show (struct UgtkSequencePreview* preview, const gchar* message)
{
	const char* items[] = { "", NULL };

	// Clear and show message
	gtk_string_list_splice (preview->store, 0,
			g_list_model_get_n_items (G_LIST_MODEL (preview->store)), items);
	gtk_string_list_append (preview->store, message);
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
