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

// response
static void on_confirm_to_exit_response (GtkWidget* dialog, gint response,
                                         UgtkConfirmDialog* cdialog);
static void on_confirm_to_delete_response (GtkWidget* dialog, gint response,
                                           UgtkConfirmDialog* cdialog);
static void on_confirm_to_delete_category_response (GtkWidget* dialog, gint response,
                                                    UgtkConfirmDialog* cdialog);

UgtkConfirmDialog*  ugtk_confirm_dialog_new (UgtkConfirmDialogMode mode, UgtkApp* app)
{
	UgtkConfirmDialog* cdialog;
	GtkWidget*  button;
	GtkBox*     hbox;
	GtkBox*     vbox;
	gchar*      temp;
	const char* title;
	const char* label;

	cdialog = g_malloc (sizeof (UgtkConfirmDialog));
	cdialog->app = app;
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
	cdialog->self = (GtkDialog*) gtk_dialog_new_with_buttons (temp,
			(GtkWindow*) app->window.self,
			GTK_DIALOG_DESTROY_WITH_PARENT,
			_("_No"),  GTK_RESPONSE_NO,
			_("_Yes"), GTK_RESPONSE_YES,
			NULL);
	g_free (temp);
#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION < 14
//	gtk_window_set_has_resize_grip ((GtkWindow*) cdialog->self, FALSE);
#endif

//	gtk_container_set_border_width (GTK_CONTAINER (cdialog->self), 4);
	vbox = (GtkBox*) gtk_dialog_get_content_area (cdialog->self);
	// image and label
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (hbox,
			gtk_image_new_from_icon_name ("dialog-question")
			);
	gtk_box_append (hbox,
			gtk_label_new (label)
			);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	// check button
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	button = gtk_check_button_new_with_label (_("Don't ask me again"));
	gtk_box_append (hbox, button);
	gtk_box_append (vbox, (GtkWidget*) hbox);
	cdialog->confirmation = (GtkToggleButton*) button;
	//
	gtk_widget_set_visible ((GtkWidget*) vbox, TRUE);

	switch (mode) {
	case UGTK_CONFIRM_DIALOG_EXIT:
		app->dialogs.exit_confirmation = (GtkWidget*) cdialog->self;
		g_signal_connect (cdialog->self, "response",
				G_CALLBACK (on_confirm_to_exit_response), cdialog);
		break;

	case UGTK_CONFIRM_DIALOG_DELETE:
		app->dialogs.delete_confirmation = (GtkWidget*) cdialog->self;
		g_signal_connect (cdialog->self, "response",
				G_CALLBACK (on_confirm_to_delete_response), cdialog);
		break;

	case UGTK_CONFIRM_DIALOG_DELETE_CATEGORY:
		app->dialogs.delete_category_confirmation = (GtkWidget*) cdialog->self;
		gtk_widget_hide ((GtkWidget*) cdialog->confirmation);
		g_signal_connect (cdialog->self, "response",
				G_CALLBACK (on_confirm_to_delete_category_response), cdialog);
		break;

	default:
		g_signal_connect (cdialog->self, "response",
				G_CALLBACK (gtk_window_destroy), NULL);
		break;
	}

	return cdialog;
}

void ugtk_confirm_dialog_free (UgtkConfirmDialog* cdialog)
{
	gtk_window_destroy ((GtkWindow*) cdialog->self);
	g_free (cdialog);
}

void  ugtk_confirm_dialog_run (UgtkConfirmDialog* cdialog)
{
	UgtkApp* app;

	app = cdialog->app;
	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, FALSE);
	gtk_widget_set_visible ((GtkWidget*) cdialog->self, TRUE);
}

static void on_confirm_to_exit_response (GtkWidget* dialog, gint response,
                                         UgtkConfirmDialog* cdialog)
{
	UgtkApp*  app;

	app = cdialog->app;
	app->dialogs.exit_confirmation = NULL;
	if (response == GTK_RESPONSE_YES) {
		if (gtk_check_button_get_active ((GtkCheckButton*) cdialog->confirmation) == FALSE)
			app->setting.ui.exit_confirmation = TRUE;
		else
			app->setting.ui.exit_confirmation = FALSE;
		ugtk_app_quit (app);
	}
	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, TRUE);
	ugtk_confirm_dialog_free (cdialog);
}

static void on_confirm_to_delete_response (GtkWidget* dialog, gint response,
                                           UgtkConfirmDialog* cdialog)
{
	UgtkApp*  app;

	app = cdialog->app;
	app->dialogs.delete_confirmation = NULL;
	if (response == GTK_RESPONSE_YES) {
		if (gtk_check_button_get_active ((GtkCheckButton*) cdialog->confirmation) == FALSE)
			app->setting.ui.delete_confirmation = TRUE;
		else
			app->setting.ui.delete_confirmation = FALSE;
		ugtk_app_delete_download (app, TRUE);
	}
	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, TRUE);
	ugtk_confirm_dialog_free (cdialog);
}

static void on_confirm_to_delete_category_response (GtkWidget* dialog, gint response,
                                                    UgtkConfirmDialog* cdialog)
{
	UgtkApp*  app;

	app = cdialog->app;
	app->dialogs.delete_category_confirmation = NULL;
	if (response == GTK_RESPONSE_YES) {
//		if (gtk_check_button_get_active ((GtkCheckButton*) cdialog->confirmation) == FALSE)
//			app->setting.ui.delete_category_confirmation = TRUE;
//		else
//			app->setting.ui.delete_category_confirmation = FALSE;
		ugtk_app_delete_category (app);
	}
	gtk_widget_set_sensitive ((GtkWidget*) app->window.self, TRUE);
	ugtk_confirm_dialog_free (cdialog);
}
