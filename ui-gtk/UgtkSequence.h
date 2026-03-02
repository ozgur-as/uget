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

#ifndef UGTK_SEQUENCE_H
#define UGTK_SEQUENCE_H

#include <UgetSequence.h>
#include <gtk/gtk.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct  UgtkSequence      UgtkSequence;
typedef struct  UgtkSeqRange      UgtkSeqRange;
typedef	void  (*UgtkSequenceNotify) (gpointer user_data, gboolean completed);

enum UgtkSeqType
{
	UGTK_SEQ_TYPE_NONE,
	UGTK_SEQ_TYPE_NUMBER,
	UGTK_SEQ_TYPE_CHARACTER
};

// -----------------------------------------------------------------------------
// UgtkSeqRange

struct UgtkSeqRange
{
	GtkWidget*  self;

	GtkWidget*  type;    // GtkDropDown - None, Number, and Character
	GtkWidget*  label_to;

	// digit mode
	GtkWidget*  spin_from;
	GtkWidget*  spin_to;
	GtkWidget*  spin_digits;
	GtkWidget*  label_digits;

	// character mode
	GtkWidget*  entry_from;
	GtkWidget*  entry_to;
	GtkWidget*  label_case;
};

void   ugtk_seq_range_init (UgtkSeqRange* range, UgtkSequence* seq, GtkSizeGroup* size_group);
void   ugtk_seq_range_set_type (UgtkSeqRange* range, enum UgtkSeqType type);
enum UgtkSeqType  ugtk_seq_range_get_type (UgtkSeqRange* range);

// -----------------------------------------------------------------------------
// UgtkSequence

struct UgtkSequence
{
	GtkWidget*    self;       // GtkGrid
	GtkEntry*     entry;      // URI + wildcard character (*)
	UgtkSeqRange  range[3];   // range x 3
	UgetSequence  sequence;

	// preview
	struct UgtkSequencePreview
	{
		GtkWidget*        self;    // GtkScrolledWindow
		GtkListView*      view;
		GtkStringList*    store;
		guint             status;
	} preview;

	// callback
	struct
	{
		UgtkSequenceNotify  func;
		gpointer            data;
	} notify;
};

void   ugtk_sequence_init (UgtkSequence* seq);
void   ugtk_sequence_show_preview (UgtkSequence* seq);
int    ugtk_sequence_get_list (UgtkSequence* seq, UgList* result);

#ifdef __cplusplus
}
#endif

#endif  // End of UGTK_SEQUENCE_H
