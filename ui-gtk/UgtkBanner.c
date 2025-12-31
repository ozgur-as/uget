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

#include <UgUtil.h>
#include <UgtkUtil.h>
#include <UgtkBanner.h>

#include <glib/gi18n.h>

static GdkCursor* hand_cursor = NULL;
static GdkCursor* regular_cursor = NULL;


// static gboolean
// motion_notify_event (GtkWidget* tv_widget, GdkEventMotion* event, UgtkBanner* banner);
//
// static gboolean
// event_after (GtkWidget* text_view, GdkEvent* ev, UgtkBanner* banner);


static GtkWidget* create_x_button (UgtkBanner* banner);

void ugtk_banner_init (struct UgtkBanner* banner)
{
	GtkStyleContext* style_context;
	GdkRGBA    rgba;

	hand_cursor = gdk_cursor_new_from_name ("pointer", NULL);
	regular_cursor = gdk_cursor_new_from_name ("text", NULL);
	// hand_cursor = gdk_cursor_new (GDK_HAND2);
	// regular_cursor = gdk_cursor_new (GDK_XTERM);

	banner->self = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	banner->buffer = gtk_text_buffer_new (NULL);
	banner->tag_link = gtk_text_buffer_create_tag (banner->buffer, NULL,
			"underline", PANGO_UNDERLINE_SINGLE,
			NULL);

	banner->text_view = (GtkTextView*) gtk_text_view_new_with_buffer (banner->buffer);
	g_object_unref (banner->buffer);
	gtk_text_view_set_cursor_visible (banner->text_view, FALSE);
	gtk_text_view_set_editable (banner->text_view, FALSE);
	// gtk_box_pack_start (GTK_BOX (banner->self),
	//		GTK_WIDGET (banner->text_view), TRUE, TRUE, 0);
	gtk_box_append (GTK_BOX (banner->self), GTK_WIDGET (banner->text_view));

	// g_signal_connect (banner->text_view, "event-after",
	//		G_CALLBACK (event_after), banner);
	// g_signal_connect (banner->text_view, "motion-notify-event",
	//		G_CALLBACK (motion_notify_event), banner);
	// style: color
	style_context = gtk_widget_get_style_context (GTK_WIDGET (banner->text_view));
	// gtk_style_context_get_background_color (style_context,
	//		GTK_STATE_FLAG_SELECTED, &rgba);
//	gtk_widget_override_background_color (
//			GTK_WIDGET (banner->text_view), GTK_STATE_FLAG_NORMAL, &rgba);
	// gtk_style_context_get_color (style_context,
	//		GTK_STATE_FLAG_SELECTED, &rgba);
//	gtk_widget_override_color (
//			GTK_WIDGET (banner->text_view), GTK_STATE_FLAG_NORMAL, &rgba);
	// close button
	// gtk_box_pack_end (GTK_BOX (banner->self),
	//		create_x_button (banner), FALSE, FALSE, 0);
	gtk_box_prepend (GTK_BOX (banner->self), create_x_button (banner));

	banner->show_builtin = 2;
	banner->rss.self = NULL;
	banner->rss.feed = NULL;
	banner->rss.item = NULL;
}

int  ugtk_banner_show_rss (UgtkBanner* banner, UgetRss* urss)
{
	banner->rss.self = urss;
	banner->rss.feed = NULL;
	banner->rss.item = NULL;

	banner->rss.feed = uget_rss_find_updated (urss, NULL);
	if (banner->rss.feed)
		banner->rss.item = uget_rss_feed_find (banner->rss.feed, banner->rss.feed->checked);
	if (banner->rss.item)
		ugtk_banner_show (banner, banner->rss.item->title, banner->rss.item->link);
	else {
		gtk_widget_set_visible (banner->self, FALSE);
		// gtk_widget_hide (banner->self);
		return FALSE;
	}
	return TRUE;
}

void  ugtk_banner_show (UgtkBanner* banner, const char* title, const char* url)
{
	GtkTextIter iter;

	gtk_text_buffer_set_text(banner->buffer, "", 0);
	gtk_text_buffer_get_iter_at_offset (banner->buffer, &iter, 0);
	gtk_text_buffer_insert (banner->buffer, &iter, "  ", 2);

	g_free (banner->link);
	if (url == NULL) {
		banner->link = NULL;
		gtk_text_buffer_insert (banner->buffer, &iter, title, -1);
	}
	else {
		banner->link = g_strdup (url);
		gtk_text_buffer_insert_with_tags (banner->buffer, &iter,
				title, -1, banner->tag_link, NULL);
	}
	gtk_widget_show (banner->self);
}

void  ugtk_banner_show_donation (UgtkBanner* banner)
{
	GtkTextIter iter;

	gtk_text_buffer_set_text(banner->buffer, "", 0);
	gtk_text_buffer_get_iter_at_offset (banner->buffer, &iter, 0);
	gtk_text_buffer_insert (banner->buffer, &iter, "  ", 2);
	gtk_text_buffer_insert (banner->buffer, &iter, _("Attention uGetters:"), -1);
	gtk_text_buffer_insert (banner->buffer, &iter, " ", 1);
	gtk_text_buffer_insert (banner->buffer, &iter,
			_("we are running a Donation Drive "
			  "for uGet's Future Development, please click "), -1);
	gtk_text_buffer_insert_with_tags (banner->buffer, &iter,
			_("HERE"), -1, banner->tag_link, NULL);

	g_free (banner->link);
	banner->link = g_strdup ("http://ugetdm.com/donate");
	gtk_widget_show (banner->self);
}

void  ugtk_banner_show_survey (UgtkBanner* banner)
{
	GtkTextIter iter;

	gtk_text_buffer_set_text(banner->buffer, "", 0);
	gtk_text_buffer_get_iter_at_offset (banner->buffer, &iter, 0);
	gtk_text_buffer_insert (banner->buffer, &iter, "  ", 2);
	gtk_text_buffer_insert (banner->buffer, &iter, _("Attention uGetters:"), -1);
	gtk_text_buffer_insert (banner->buffer, &iter, " ", 1);
	gtk_text_buffer_insert (banner->buffer, &iter,
			_("please fill out this quick User Survey for uGet."), -1);
	gtk_text_buffer_insert (banner->buffer, &iter, " - ", 3);
	gtk_text_buffer_insert_with_tags (banner->buffer, &iter,
			_("click here to take survey"), -1, banner->tag_link, NULL);

	g_free (banner->link);
	banner->link = g_strdup ("http://ugetdm.com/survey");
	gtk_widget_show (banner->self);
}

// ----------------------------------------------------------------------------
// static functions

/* Links can also be activated by clicking.
 */
/*
static gboolean
event_after (GtkWidget* text_view, GdkEvent* ev, UgtkBanner* banner)
{
	return FALSE;
}
*/

/* Update the cursor image if the pointer moved.
 */
/* Looks at all tags covering the position (x, y) in the text view,
 * and if one of them is a link, change the cursor to the "hands" cursor
 * typically used by web browsers.
 */
/*
static gboolean
motion_notify_event (GtkWidget* tv_widget, GdkEventMotion* event, UgtkBanner* banner)
{
	return FALSE;
}
*/

// ------------------------------------

static void
on_x_button_clicked (GtkButton* button, UgtkBanner* banner)
// on_x_button_release (GtkWidget* text_view, GdkEvent* ev, UgtkBanner* banner)
{
/*
	GdkEventButton* event;

	event = (GdkEventButton*) ev;
	if (event->button != GDK_BUTTON_PRIMARY)
		return FALSE;
*/

	if (banner->rss.self) {
		if (banner->rss.feed && banner->rss.item)
			banner->rss.feed->checked = banner->rss.item->updated;
		// if (ugtk_banner_show_rss (banner, banner->rss.self))
		//	return FALSE;
		ugtk_banner_show_rss (banner, banner->rss.self);
		return;
	}

	if (banner->show_builtin > 0) {
		banner->show_builtin--;
		if (banner->show_builtin == 1)
			ugtk_banner_show_donation (banner);
		else {
			ugtk_banner_show_survey (banner);
			banner->show_builtin = 0;
		}
		return;
		// return FALSE;
	}

	gtk_widget_set_visible (banner->self, FALSE);
	// gtk_widget_hide (banner->self);
	// return FALSE;
}

static GtkWidget* create_x_button (UgtkBanner* banner)
{
	GtkWidget* event_box;
	GtkWidget* label;

	label = gtk_label_new (" X ");
	// event_box = gtk_event_box_new ();
	// gtk_container_add (GTK_CONTAINER (event_box), label);
	event_box = gtk_button_new ();
	gtk_button_set_has_frame (GTK_BUTTON (event_box), FALSE);
	gtk_button_set_child (GTK_BUTTON (event_box), label);

	g_signal_connect (event_box, "clicked",
			G_CALLBACK (on_x_button_clicked), banner);
	return event_box;
}

