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

#include <UgtkConfirmDialog.h>

#include <glib/gi18n.h>

static void on_confirm_yes (GtkWidget* button, UgtkConfirmDialog* cdialog);
static void on_confirm_no (GtkWidget* button, UgtkConfirmDialog* cdialog);
static gboolean on_confirm_close_request (GtkWindow* window, UgtkConfirmDialog* cdialog);

UgtkConfirmDialog*  ugtk_confirm_dialog_new (UgtkConfirmDialogMode mode, UgtkApp* app)
{
	UgtkConfirmDialog* cdialog;
	GtkWidget*  button;
	GtkWidget*  button_no;
	GtkWidget*  button_yes;
	GtkBox*     hbox;
	GtkBox*     vbox;
	GtkBox*     button_box;
	gchar*      temp;
	const char* title;
	const char* label;

	cdialog = g_malloc (sizeof (UgtkConfirmDialog));
	cdialog->app = app;
	cdialog->mode = mode;
	// create confirmation dialog
	switch (mode) {
	case UGTK_CONFIRM_DIALOG_EXIT:
		title = _("Really Quit?");
		label = _("Are you sure you want to quit?");
		break;

	case UGTK_CONFIRM_DIALOG_DELETE:
		title = _("Really delete files?");
		label = _("Are you sure you want to delete files?");
		break;

	case UGTK_CONFIRM_DIALOG_DELETE_CATEGORY:
		title = _("Really delete category?");
		label = _("Are you sure you want to delete category?");
		break;

	default:
		title = NULL;
		label = NULL;
		break;
	}

	temp = g_strconcat (UGTK_APP_NAME " - ", title, NULL);
	cdialog->self = (GtkWindow*) gtk_window_new ();
	gtk_window_set_title (cdialog->self, temp);
	gtk_window_set_transient_for (cdialog->self, app->window.self);
	gtk_window_set_destroy_with_parent (cdialog->self, TRUE);
	gtk_window_set_modal (cdialog->self, TRUE);
	g_free (temp);

	// main layout
	vbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_widget_set_margin_start ((GtkWidget*) vbox, 12);
	gtk_widget_set_margin_end ((GtkWidget*) vbox, 12);
	gtk_widget_set_margin_top ((GtkWidget*) vbox, 12);
	gtk_widget_set_margin_bottom ((GtkWidget*) vbox, 12);
	gtk_window_set_child (cdialog->self, (GtkWidget*) vbox);

	// image and label
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_box_append (hbox,
			gtk_image_new_from_icon_name ("dialog-question"));
	gtk_box_append (hbox,
			gtk_label_new (label));
	gtk_box_append (vbox, (GtkWidget*) hbox);

	// check button
	button = gtk_check_button_new_with_label (_("Don't ask me again"));
	gtk_box_append (vbox, button);
	cdialog->confirmation = (GtkCheckButton*) button;

	// button box
	button_box = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 6);
	gtk_widget_set_halign ((GtkWidget*) button_box, GTK_ALIGN_END);
	gtk_widget_set_margin_top ((GtkWidget*) button_box, 6);
	gtk_box_append (vbox, (GtkWidget*) button_box);

	button_no = gtk_button_new_with_mnemonic (_("_No"));
	gtk_box_append (button_box, button_no);
	button_yes = gtk_button_new_with_mnemonic (_("_Yes"));
	gtk_box_append (button_box, button_yes);

	switch (mode) {
	case UGTK_CONFIRM_DIALOG_EXIT:
		app->dialogs.exit_confirmation = (GtkWidget*) cdialog->self;
		break;

	case UGTK_CONFIRM_DIALOG_DELETE:
		app->dialogs.delete_confirmation = (GtkWidget*) cdialog->self;
		break;

	case UGTK_CONFIRM_DIALOG_DELETE_CATEGORY:
		app->dialogs.delete_category_confirmation = (GtkWidget*) cdialog->self;
		gtk_widget_set_visible ((GtkWidget*) cdialog->confirmation, FALSE);
		break;

	default:
		break;
	}

	g_signal_connect (button_yes, "clicked",
			G_CALLBACK (on_confirm_yes), cdialog);
	g_signal_connect (button_no, "clicked",
			G_CALLBACK (on_confirm_no), cdialog);
	g_signal_connect (cdialog->self, "close-request",
			G_CALLBACK (on_confirm_close_request), cdialog);

	return cdialog;
}

void ugtk_confirm_dialog_free (UgtkConfirmDialog* cdialog)
{
	gtk_window_destroy (cdialog->self);
	g_free (cdialog);
}

void  ugtk_confirm_dialog_run (UgtkConfirmDialog* cdialog)
{
	UgtkApp* app;

	app = cdialog->app;
	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, FALSE);
	gtk_widget_set_visible ((GtkWidget*) cdialog->self, TRUE);
}

static void on_confirm_yes (GtkWidget* button, UgtkConfirmDialog* cdialog)
{
	UgtkApp*  app;

	app = cdialog->app;
	switch (cdialog->mode) {
	case UGTK_CONFIRM_DIALOG_EXIT:
		app->dialogs.exit_confirmation = NULL;
		if (gtk_check_button_get_active ((GtkCheckButton*) cdialog->confirmation) == FALSE)
			app->setting.ui.exit_confirmation = TRUE;
		else
			app->setting.ui.exit_confirmation = FALSE;
		ugtk_app_quit (app);
		break;

	case UGTK_CONFIRM_DIALOG_DELETE:
		app->dialogs.delete_confirmation = NULL;
		if (gtk_check_button_get_active ((GtkCheckButton*) cdialog->confirmation) == FALSE)
			app->setting.ui.delete_confirmation = TRUE;
		else
			app->setting.ui.delete_confirmation = FALSE;
		ugtk_app_delete_download (app, TRUE);
		break;

	case UGTK_CONFIRM_DIALOG_DELETE_CATEGORY:
		app->dialogs.delete_category_confirmation = NULL;
		ugtk_app_delete_category (app);
		break;

	default:
		break;
	}
	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, TRUE);
	ugtk_confirm_dialog_free (cdialog);
}

static void on_confirm_no (GtkWidget* button, UgtkConfirmDialog* cdialog)
{
	UgtkApp*  app;

	app = cdialog->app;
	switch (cdialog->mode) {
	case UGTK_CONFIRM_DIALOG_EXIT:
		app->dialogs.exit_confirmation = NULL;
		break;
	case UGTK_CONFIRM_DIALOG_DELETE:
		app->dialogs.delete_confirmation = NULL;
		break;
	case UGTK_CONFIRM_DIALOG_DELETE_CATEGORY:
		app->dialogs.delete_category_confirmation = NULL;
		break;
	default:
		break;
	}
	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, TRUE);
	ugtk_confirm_dialog_free (cdialog);
}

static gboolean on_confirm_close_request (GtkWindow* window, UgtkConfirmDialog* cdialog)
{
	on_confirm_no (NULL, cdialog);
	return TRUE;
}
