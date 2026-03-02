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


#ifndef UGTK_NODE_DIALOG_H
#define UGTK_NODE_DIALOG_H

#include <gtk/gtk.h>
#include <UgtkApp.h>
#include <UgtkNodeTree.h>
#include <UgtkCategoryForm.h>
#include <UgtkDownloadForm.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  UgtkNodeDialog        UgtkNodeDialog;

typedef enum {
	UGTK_NODE_DIALOG_NEW_DOWNLOAD,
	UGTK_NODE_DIALOG_NEW_CATEGORY,
	UGTK_NODE_DIALOG_EDIT_DOWNLOAD,
	UGTK_NODE_DIALOG_EDIT_CATEGORY,
} UgtkNodeDialogMode;

#define UGTK_NODE_DIALOG_MEMBERS  \
	GtkWindow*    self;           \
	GtkBox*       hbox;           \
	GtkWidget*    notebook;       \
	GtkBox*       button_box;     \
	GtkWidget*    ok_button;      \
	GtkWidget*    cancel_button;  \
	GtkWidget*    node_view;       \
	UgtkNodeTree* node_tree;      \
	gulong        handler_id[3];  \
	UgtkApp*      app;            \
	UgetNode*     node;           \
	UgInfo*       node_info;      \
	UgtkProxyForm     proxy;      \
	UgtkDownloadForm  download;   \
	UgtkCategoryForm  category

struct UgtkNodeDialog
{
	UGTK_NODE_DIALOG_MEMBERS;
/*	// ------ UgtkNodeDialog members ------
	GtkWindow*    self;
	GtkBox*       hbox;
	GtkWidget*    notebook;
	GtkBox*       button_box;
	GtkWidget*    ok_button;
	GtkWidget*    cancel_button;
	...
 */

	// handler_id[0] : "items-changed" on category model
};

UgtkNodeDialog*  ugtk_node_dialog_new (const char* title,
                                       UgtkApp*    app,
                                       gboolean    has_category_form);

void             ugtk_node_dialog_free (UgtkNodeDialog* ndialog);

// enable/disable category node list view in left side
// return index of selected category
int   ugtk_node_dialog_get_category (UgtkNodeDialog* ndialog, UgetNode** cnode);
void  ugtk_node_dialog_set_category (UgtkNodeDialog* ndialog, UgetNode* cnode);

// set/get node's info to/from UgtkNodeDialog
void  ugtk_node_dialog_get (UgtkNodeDialog* ndialog, UgInfo* node_info);
void  ugtk_node_dialog_set (UgtkNodeDialog* ndialog, UgInfo* node_info);

void  ugtk_node_dialog_run (UgtkNodeDialog* ndialog,
                            UgtkNodeDialogMode mode,
                            UgetNode* node4edit);

void  ugtk_node_dialog_store_recent (UgtkNodeDialog* ndialog, UgtkApp* app);
void  ugtk_node_dialog_apply_recent (UgtkNodeDialog* ndialog, UgtkApp* app);

void  ugtk_node_dialog_monitor_uri (UgtkNodeDialog* ndialog);
gboolean  ugtk_node_dialog_confirm_existing (UgtkNodeDialog* ndialog, const char* uri);

// initialize notebook and it's form
void  ugtk_node_dialog_init (UgtkNodeDialog* ndialog,
                             const char*     title,
                             UgtkApp*        app,
                             gboolean        has_category_form);

#ifdef __cplusplus
}
#endif

#endif  // End of UGTK_NODE_DIALOG_H

