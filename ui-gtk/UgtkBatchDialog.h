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


#ifndef UGTK_BATCH_DIALOG_H
#define UGTK_BATCH_DIALOG_H

#include <gtk/gtk.h>
#include <UgtkApp.h>
#include <UgtkNodeDialog.h>
#include <UgtkSequence.h>
#include <UgtkSelector.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  UgtkBatchDialog        UgtkBatchDialog;

struct UgtkBatchDialog
{
	UGTK_NODE_DIALOG_MEMBERS;

	GtkWidget*        back_button;
	GtkWidget*        forward_button;
	UgtkSelector      selector;
	UgtkSequence      sequencer;
};

UgtkBatchDialog*  ugtk_batch_dialog_new (const char* title,
                                         UgtkApp*    app);
void              ugtk_batch_dialog_free (UgtkBatchDialog* bdialog);

void  ugtk_batch_dialog_use_selector (UgtkBatchDialog* bdialog);
void  ugtk_batch_dialog_use_sequencer (UgtkBatchDialog* bdialog);

// enable/disable category node list view in left side
// return index of selected category
#define ugtk_batch_dialog_get_category(bdialog, cnode)    \
		ugtk_node_dialog_get_category((UgtkNodeDialog*) bdialog, cnode)
#define ugtk_batch_dialog_set_category(bdialog, cnode)    \
		ugtk_node_dialog_set_category((UgtkNodeDialog*) bdialog, cnode)

// void ugtk_batch_dialog_apply_recent (UgtkNodeDialog* ndialog, UgtkApp* app);
#define ugtk_batch_dialog_apply_recent(bdialog, app)    \
		ugtk_node_dialog_apply_recent((UgtkNodeDialog*) bdialog, app);

void  ugtk_batch_dialog_disable_batch (UgtkBatchDialog* bdialog);
void  ugtk_batch_dialog_run (UgtkBatchDialog* bdialog);

#ifdef __cplusplus
}
#endif

#endif  // End of UGTK_BATCH_DIALOG_H

