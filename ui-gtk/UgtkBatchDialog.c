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

#include <UgString.h>
#include <UgtkBatchDialog.h>

#include <glib/gi18n.h>

// Callback
static void ugtk_batch_dialog_set_completed (UgtkBatchDialog* bdialog,
                                             gboolean       completed);
static void on_response (GtkDialog *dialog, gint response_id,
                         UgtkBatchDialog* bdialog);

// ----------------------------------------------------------------------------
// UgtkBatchDialog
UgtkBatchDialog*  ugtk_batch_dialog_new (const char* title,
                                         UgtkApp*    app)
{
	UgtkBatchDialog* bdialog;

	bdialog = g_malloc0 (sizeof (UgtkBatchDialog));
	ugtk_node_dialog_init ((UgtkNodeDialog*) bdialog, title, app, FALSE);
	ugtk_download_form_set_multiple (&bdialog->download, TRUE);

#if GTK_MAJOR_VERSION <= 3 && GTK_MINOR_VERSION < 14
	gtk_window_set_has_resize_grip ((GtkWindow*)bdialog->self, FALSE);
#endif
	gtk_window_set_default_size ((GtkWindow*)bdialog->self, 500, 350);
	// back button
	gtk_dialog_add_button (bdialog->self, _("_Back"),
	                       GTK_RESPONSE_REJECT);
	// forward button
	gtk_dialog_add_button (bdialog->self, _("_Forward"),
	                       GTK_RESPONSE_ACCEPT);
	// OK & cancel buttons
	gtk_dialog_add_button (bdialog->self, _("_Cancel"),
	                       GTK_RESPONSE_CANCEL);
	gtk_dialog_add_button (bdialog->self, _("_OK"),
	                       GTK_RESPONSE_OK);
	gtk_dialog_set_default_response (bdialog->self, GTK_RESPONSE_OK);

	// set button sensitive
	gtk_dialog_set_response_sensitive (bdialog->self,
			GTK_RESPONSE_OK, FALSE);
	gtk_dialog_set_response_sensitive (bdialog->self,
			GTK_RESPONSE_ACCEPT, FALSE);
	// response handler
	g_signal_connect (bdialog->self, "response",
			G_CALLBACK (on_response), bdialog);
	return bdialog;
}

void  ugtk_batch_dialog_free (UgtkBatchDialog* bdialog)
{
	// selector
	if (bdialog->selector.self)
		ugtk_selector_finalize (&bdialog->selector);
	// dialog
	ugtk_node_dialog_free ((UgtkNodeDialog*) bdialog);
}

void  ugtk_batch_dialog_use_selector (UgtkBatchDialog* bdialog)
{
	GtkRequisition  requisition;

	gtk_dialog_set_response_sensitive (bdialog->self,
			GTK_RESPONSE_REJECT, FALSE);
	// add Page 1
	ugtk_selector_init (&bdialog->selector, (GtkWindow*) bdialog->self);
	gtk_widget_get_preferred_size (bdialog->notebook, &requisition, NULL);
	gtk_widget_set_size_request (bdialog->selector.self,
			requisition.width, requisition.height);
	gtk_widget_set_hexpand (bdialog->selector.self, TRUE);
	gtk_widget_set_vexpand (bdialog->selector.self, TRUE);
	gtk_box_append (GTK_BOX(bdialog->hbox), bdialog->selector.self);
	// hide Page 2
	gtk_widget_hide (bdialog->notebook);
	// set focus
	gtk_window_set_focus (GTK_WINDOW (bdialog->self),
			GTK_WIDGET (bdialog->selector.notebook));
	// set notify function & data
	bdialog->selector.notify.func = (void*) ugtk_batch_dialog_set_completed;
	bdialog->selector.notify.data = bdialog;
}

void  ugtk_batch_dialog_use_sequencer (UgtkBatchDialog* bdialog)
{
	GtkRequisition  requisition;

	gtk_dialog_set_response_sensitive (bdialog->self,
			GTK_RESPONSE_REJECT, FALSE);
	// add Page 1
	ugtk_sequence_init (&bdialog->sequencer);
	gtk_widget_get_preferred_size (bdialog->notebook, &requisition, NULL);
	gtk_widget_set_size_request (bdialog->sequencer.self,
			requisition.width, requisition.height);
	gtk_widget_set_hexpand (bdialog->sequencer.self, TRUE);
	gtk_widget_set_vexpand (bdialog->sequencer.self, TRUE);
	gtk_box_append (GTK_BOX(bdialog->hbox), bdialog->sequencer.self);
	// hide Page 2
	gtk_widget_hide (bdialog->notebook);
	// set focus
	gtk_window_set_focus (GTK_WINDOW (bdialog->self),
			GTK_WIDGET (bdialog->sequencer.entry));
	// set notify function & data
	bdialog->sequencer.notify.func = (void*) ugtk_batch_dialog_set_completed;
	bdialog->sequencer.notify.data = bdialog;
}

void  ugtk_batch_dialog_disable_batch (UgtkBatchDialog* bdialog)
{
	GtkWidget*  widget;

	ugtk_download_form_set_multiple (&bdialog->download, FALSE);
	ugtk_node_dialog_monitor_uri ((UgtkNodeDialog*) bdialog);
	// forward to next page.
	gtk_dialog_response (bdialog->self, GTK_RESPONSE_ACCEPT);
	// disable forward and back button
	gtk_dialog_set_response_sensitive (bdialog->self,
	                                   GTK_RESPONSE_REJECT, FALSE);
	gtk_dialog_set_response_sensitive (bdialog->self,
	                                   GTK_RESPONSE_ACCEPT, FALSE);
	// hide forward and back button
	widget = gtk_dialog_get_widget_for_response (bdialog->self,
	                                             GTK_RESPONSE_REJECT);
	gtk_widget_set_visible (widget, FALSE);
	widget = gtk_dialog_get_widget_for_response (bdialog->self,
	                                             GTK_RESPONSE_ACCEPT);
	gtk_widget_set_visible (widget, FALSE);
}

void  ugtk_batch_dialog_run (UgtkBatchDialog* bdialog)
{
	ugtk_node_dialog_apply_recent ((UgtkNodeDialog*) bdialog,
	                               bdialog->app);
	// emit notify and call ugtk_batch_dialog_set_completed()
	if (bdialog->selector.self)
		ugtk_selector_count_marked (&bdialog->selector);

//	gtk_dialog_run (ndialog->self);
	gtk_widget_show ((GtkWidget*) bdialog->self);
}

// ----------------------------------------------------------------------------
// Callback

static void ugtk_batch_dialog_set_completed (UgtkBatchDialog* bdialog,
                                             gboolean       completed)
{
	gtk_dialog_set_response_sensitive (bdialog->self,
			GTK_RESPONSE_OK, completed);
	gtk_dialog_set_response_sensitive (bdialog->self,
			GTK_RESPONSE_ACCEPT, completed);
}

static void on_no_batch_response (UgtkBatchDialog* bdialog)
{
	UgtkApp*    app;
	UgetNode*   dnode;
	UgetNode*   cnode;
	const char* uri;

	app = bdialog->app;
	ugtk_batch_dialog_get_category (bdialog, &cnode);
	ugtk_download_form_get_folders (&bdialog->download,
	                                &app->setting);

	uri = gtk_editable_get_text ((GtkEditable*)bdialog->download.uri_entry);
	if (ugtk_node_dialog_confirm_existing((UgtkNodeDialog*) bdialog, uri)) {
		dnode = uget_node_new (NULL);
		ugtk_node_dialog_get ((UgtkNodeDialog*) bdialog, dnode->info);
		uget_app_add_download ((UgetApp*) app, dnode, cnode, FALSE);
	}
}

static void on_sequencer_response (UgtkBatchDialog* bdialog)
{
	UgtkApp*    app;
	UgetNode*   dnode;
	UgetNode*   cnode;
	UgetCommon* common;
	UgList  result;
	UgLink* link;

	app = bdialog->app;
	ugtk_batch_dialog_get_category (bdialog, &cnode);
	ugtk_download_form_get_folders (&bdialog->download,
	                                &app->setting);
	// sequencer batch
	ug_list_init (&result);
	ugtk_sequence_get_list (&bdialog->sequencer, &result);

	for (link = result.head;  link;  link = link->next) {
		dnode = uget_node_new (NULL);
		common = ug_info_realloc (dnode->info, UgetCommonInfo);
		ugtk_node_dialog_get ((UgtkNodeDialog*) bdialog, dnode->info);
		common->uri = ug_strdup (link->data);
		uget_app_add_download ((UgetApp*) app, dnode, cnode, FALSE);
	}

	uget_sequence_clear_result(&result);
}

static void on_selector_response (UgtkBatchDialog* bdialog)
{
	UgtkApp*    app;
	UgetNode*   dnode;
	UgetNode*   cnode;
	UgetCommon* common;
	GList*      uri_list;
	GList*      link;

	app = bdialog->app;
	ugtk_batch_dialog_get_category (bdialog, &cnode);
	ugtk_download_form_get_folders (&bdialog->download,
	                                &app->setting);
	// selector batch
	uri_list = ugtk_selector_get_marked_uris (&bdialog->selector);

	for (link = uri_list;  link;  link = link->next) {
		dnode = uget_node_new (NULL);
		common = ug_info_realloc (dnode->info, UgetCommonInfo);
		ugtk_node_dialog_get ((UgtkNodeDialog*) bdialog, dnode->info);
#if 0
		common->uri = link->data;
		link->data = NULL;
#else
		common->uri = ug_strdup (link->data);
		g_free (link->data);
#endif
		uget_app_add_download ((UgetApp*) app, dnode, cnode, FALSE);
	}

	g_list_free (uri_list);
}

static void on_response (GtkDialog *dialog, gint response_id,
                         UgtkBatchDialog* bdialog)
{
	switch (response_id) {
	case GTK_RESPONSE_REJECT:  // back button
		gtk_dialog_set_response_sensitive (bdialog->self,
				GTK_RESPONSE_REJECT, FALSE);
		gtk_dialog_set_response_sensitive (bdialog->self,
				GTK_RESPONSE_ACCEPT, TRUE);
		// switch page
		gtk_widget_hide (bdialog->notebook);
		if (bdialog->selector.self)
			gtk_widget_show (bdialog->selector.self);
		else if (bdialog->sequencer.self)
			gtk_widget_show (bdialog->sequencer.self);
		break;

	case GTK_RESPONSE_ACCEPT:  // forward button
		gtk_dialog_set_response_sensitive (bdialog->self,
				GTK_RESPONSE_REJECT, TRUE);
		gtk_dialog_set_response_sensitive (bdialog->self,
				GTK_RESPONSE_ACCEPT, FALSE);
		// switch page
		gtk_widget_show (bdialog->notebook);
		if (bdialog->selector.self)
			gtk_widget_hide (bdialog->selector.self);
		else if (bdialog->sequencer.self)
			gtk_widget_hide (bdialog->sequencer.self);
		break;

	case GTK_RESPONSE_CANCEL:
	default:
		ugtk_batch_dialog_free (bdialog);
		break;

	case GTK_RESPONSE_OK:
		ugtk_node_dialog_store_recent ((UgtkNodeDialog*) bdialog, bdialog->app);
		if (gtk_widget_get_sensitive (bdialog->download.uri_entry))
			on_no_batch_response (bdialog);
		if (bdialog->sequencer.self)
			on_sequencer_response (bdialog);
		else if (bdialog->selector.self)
			on_selector_response (bdialog);
		ugtk_batch_dialog_free (bdialog);
		break;
	}
}
