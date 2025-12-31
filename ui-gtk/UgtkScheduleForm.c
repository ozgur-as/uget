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

#include <memory.h>
#include <UgtkScheduleForm.h>

#include <glib/gi18n.h>

#define	COLOR_DISABLE_R     0.5
#define	COLOR_DISABLE_G     0.5
#define	COLOR_DISABLE_B     0.5

static const gdouble  colors[UGTK_SCHEDULE_N_STATE][3] =
{
	{1.0,   1.0,   1.0},        // UGTK_SCHEDULE_TURN_OFF
	{1.0,   0.752, 0.752},      // UGTK_SCHEDULE_UPLOAD_ONLY - reserve
	{0.552, 0.807, 0.552},      // UGTK_SCHEDULE_LIMITED_SPEED
//	{0.0,   0.658, 0.0},        // UGTK_SCHEDULE_NORMAL
	{0.0,   0.758, 0.0},        // UGTK_SCHEDULE_NORMAL
};

static const gchar*  week_days[7] =
{
	N_("Mon"),
	N_("Tue"),
	N_("Wed"),
	N_("Thu"),
	N_("Fri"),
	N_("Sat"),
	N_("Sun"),
};

// UgtkGrid
static struct {
	int  width;
	int  height;
	int  width_and_line;
	int  height_and_line;
	int  width_all;
	int  height_all;
} UgtkGrid;

static void       ugtk_grid_global_init (int width, int height);
static GtkWidget* ugtk_grid_new (const gdouble* rgb_array);
// static gboolean   ugtk_grid_draw (GtkWidget* widget, cairo_t* cr, const gdouble* rgb_array);

// signal handler
// signal handler
static void     on_enable_toggled (GtkCheckButton* togglebutton, struct UgtkScheduleForm* sform);
// static gboolean on_draw_callback (GtkWidget* widget, cairo_t* cr, struct UgtkScheduleForm* sform);
static void on_draw_callback (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data);
static void ugtk_grid_draw (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data);
// static gboolean on_button_press_event (GtkWidget* widget, GdkEventMotion* event, struct UgtkScheduleForm* sform);
// static gboolean on_motion_notify_event (GtkWidget* widget, GdkEventMotion* event, struct UgtkScheduleForm* sform);
// static gboolean on_leave_notify_event (GtkWidget* menu, GdkEventCrossing* event, struct UgtkScheduleForm* sform);
static void on_drag_begin (GtkGestureDrag *gesture, double start_x, double start_y, gpointer user_data);
static void on_drag_update (GtkGestureDrag *gesture, double offset_x, double offset_y, gpointer user_data);


void  ugtk_schedule_form_init (struct UgtkScheduleForm* sform)
{
	PangoContext*  context;
	PangoLayout*   layout;
	GtkWidget*  widget;
	GtkGrid*    caption;
	GtkBox*     hbox;
	GtkBox*     vbox;
	int         text_width, text_height;

	sform->self = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	vbox = (GtkBox*) sform->self;

	// Enable Scheduler
	hbox = (GtkBox*) gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 2);
	gtk_box_append (vbox, (GtkWidget*)hbox);
	widget = gtk_check_button_new_with_mnemonic (_("_Enable Scheduler"));
	gtk_box_append (hbox, widget);
	gtk_box_append (hbox, gtk_separator_new (GTK_ORIENTATION_HORIZONTAL));
	g_signal_connect (widget, "toggled",
			G_CALLBACK (on_enable_toggled), sform);
	sform->enable = widget;

	// initialize UgtkGrid
	context = gtk_widget_get_pango_context (widget);
	layout = pango_layout_new (context);
	pango_layout_set_text (layout, gettext (week_days[0]), -1);
	pango_layout_get_pixel_size (layout, &text_width, &text_height);
	g_object_unref (layout);
	ugtk_grid_global_init (text_height, text_height + 2);

	// drawing area
	widget = gtk_drawing_area_new ();
	gtk_box_append (vbox, widget);
//	gtk_widget_set_has_window (widget, FALSE);
	gtk_widget_set_size_request (widget,
			UgtkGrid.width_all + text_width + 32, UgtkGrid.height_all);
	// gtk_widget_add_events (widget,
	// 		GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK);
	// g_signal_connect (widget, "draw",
	// 		G_CALLBACK (on_draw_callback), sform);
    gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (widget), on_draw_callback, sform, NULL);
	
	// GTK4: Gesture based input
	GtkGesture *drag = gtk_gesture_drag_new ();
	g_signal_connect (drag, "drag-begin", G_CALLBACK (on_drag_begin), sform);
	g_signal_connect (drag, "drag-update", G_CALLBACK (on_drag_update), sform);
	gtk_widget_add_controller (widget, GTK_EVENT_CONTROLLER (drag));
	
//	g_signal_connect (widget, "button-press-event",
//			G_CALLBACK (on_button_press_event), sform);
//	g_signal_connect (widget, "motion-notify-event",
//			G_CALLBACK (on_motion_notify_event), sform);
//	g_signal_connect (widget, "leave-notify-event",
//			G_CALLBACK (on_leave_notify_event), sform);
	sform->drawing = widget;

	// grid for tips, SpinButton
	sform->caption = gtk_grid_new ();
	gtk_box_append (vbox, sform->caption);
	caption = (GtkGrid*) sform->caption;
	// time tips
	widget = gtk_label_new ("");
	// GTK4: Use gtk_label_set_xalign/yalign
	gtk_label_set_xalign ((GtkLabel*)widget, 0.4);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin", 2, NULL);
	gtk_grid_attach (caption, widget, 0, 0, 5, 1);
	sform->time_tips = GTK_LABEL (widget);

	// Turn off
	widget = ugtk_grid_new (colors[UGTK_SCHEDULE_TURN_OFF]);
	g_object_set (widget, "margin", 3, NULL);
	gtk_grid_attach (caption, widget, 0, 1, 1, 1);
	// Turn off - label
	widget = gtk_label_new (_("Turn off"));
	// GTK4: Use gtk_label_set_xalign/yalign
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin", 2, NULL);
	gtk_grid_attach (caption, widget, 1, 1, 1, 1);
	// Turn off - help label
	widget = gtk_label_new (_("- stop all task"));
	// GTK4: Use gtk_label_set_xalign/yalign
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin", 2, NULL);
	gtk_grid_attach (caption, widget, 2, 1, 2, 1);

	// Normal
	widget = ugtk_grid_new (colors[UGTK_SCHEDULE_NORMAL]);
	g_object_set (widget, "margin", 3, NULL);
	gtk_grid_attach (caption, widget, 0, 2, 1, 1);
	// Normal - label
	widget = gtk_label_new (_("Normal"));
	// GTK4: Use gtk_label_set_xalign/yalign
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin", 2, NULL);
	gtk_grid_attach (caption, widget, 1, 2, 1, 1);
	// Normal - help label
	widget = gtk_label_new (_("- run task normally"));
	// GTK4: Use gtk_label_set_xalign/yalign
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin", 2, NULL);
	gtk_grid_attach (caption, widget, 2, 2, 2, 1);
	// Speed limit
	widget = ugtk_grid_new (colors[UGTK_SCHEDULE_LIMITED_SPEED]);
	g_object_set (widget, "margin", 3, NULL);
	gtk_grid_attach (caption, widget, 0, 3, 1, 1);
	// Speed limit - label
	widget = gtk_label_new (_("Limited speed"));
	// GTK4: Use gtk_label_set_xalign/yalign
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin", 2, NULL);
	gtk_grid_attach (caption, widget, 1, 3, 1, 1);
    
    // Note: sform->spin_speed usage needs to be checked.
	// Speed limit - SpinButton
	widget = gtk_spin_button_new_with_range (5, 99999999, 1);
	g_object_set (widget, "margin", 2, NULL);
	gtk_grid_attach (caption, widget, 2, 3, 1, 1);
	sform->spin_speed = (GtkSpinButton*) widget;
    
	// Speed limit - KiB/s label
	widget = gtk_label_new (_("KiB/s"));
	// GTK4: Use gtk_label_set_xalign/yalign
	gtk_label_set_xalign ((GtkLabel*)widget, 0.0);
	gtk_label_set_yalign ((GtkLabel*)widget, 0.5);	// left, center
	g_object_set (widget, "margin", 2, NULL);
	gtk_grid_attach (caption, widget, 3, 3, 1, 1);
	// change sensitive state
	gtk_check_button_set_active (GTK_CHECK_BUTTON (sform->enable), FALSE);
	on_enable_toggled (GTK_CHECK_BUTTON (sform->enable), sform);
	gtk_widget_set_visible (sform->self, TRUE);
}

void  ugtk_schedule_form_get (struct UgtkScheduleForm* sform, UgtkSetting* setting)
{
	gint  value;

	memcpy (setting->scheduler.state.at, sform->state, sizeof (sform->state));
	setting->scheduler.enable = gtk_check_button_get_active (
			GTK_CHECK_BUTTON (sform->enable));

	value = gtk_spin_button_get_value_as_int (sform->spin_speed);
	setting->bandwidth.scheduler.download = value * 1024;
}

void  ugtk_schedule_form_set (struct UgtkScheduleForm* sform, UgtkSetting* setting)
{
	gint  value;

	memcpy (sform->state, setting->scheduler.state.at, sizeof (sform->state));
	gtk_check_button_set_active (GTK_CHECK_BUTTON (sform->enable), setting->scheduler.enable);
	on_enable_toggled (GTK_CHECK_BUTTON (sform->enable), sform);


	value = setting->bandwidth.scheduler.download / 1024;
	if (value <= 0) value = 10; // Default safety
	gtk_spin_button_set_value (sform->spin_speed, value);
}


// ----------------------------------------------------------------------------
// Interaction logic

static gboolean ugtk_schedule_get_cell (struct UgtkScheduleForm* sform, double x, double y, int *row, int *col)
{
	int c, r;
	
	if (x < sform->drawing_offset)
		return FALSE;
		
	c = (int) ((x - sform->drawing_offset) / UgtkGrid.width_and_line);
	r = (int) (y / UgtkGrid.height_and_line);
	
	if (c >= 0 && c < 24 && r >= 0 && r < 7) {
		*col = c;
		*row = r;
		return TRUE;
	}
	return FALSE;
}

static void on_drag_begin (GtkGestureDrag *gesture, double start_x, double start_y, gpointer user_data)
{
	struct UgtkScheduleForm* sform = (struct UgtkScheduleForm*) user_data;
	int row, col;
	
	if (!gtk_widget_get_sensitive(sform->drawing))
		return;

	if (ugtk_schedule_get_cell (sform, start_x, start_y, &row, &col)) {
		UgtkScheduleState next_state;
		next_state = (sform->state[row][col] + 1) % UGTK_SCHEDULE_N_STATE;
		// Skip UPLOAD_ONLY (1) if it's reserved/not used? (Logic copied from colors array comment?)
		// The colors array has 3 entries? Wait, definition is UGTK_SCHEDULE_N_STATE.
		// Let's assume N_STATE matches colors size.
		// If index 1 is reserved, we check logic. Old code:
		if (next_state == UGTK_SCHEDULE_UPLOAD_ONLY) // If this logic existed
			next_state++;
		if (next_state >= UGTK_SCHEDULE_N_STATE)
			next_state = 0;
			
		sform->state[row][col] = next_state;
		sform->last_state = next_state;
		gtk_widget_queue_draw (sform->drawing);
	}
}

static void on_drag_update (GtkGestureDrag *gesture, double offset_x, double offset_y, gpointer user_data)
{
	struct UgtkScheduleForm* sform = (struct UgtkScheduleForm*) user_data;
	double start_x, start_y;
	int row, col;

	if (!gtk_widget_get_sensitive(sform->drawing))
		return;

	if (gtk_gesture_drag_get_start_point (gesture, &start_x, &start_y)) {
		double current_x = start_x + offset_x;
		double current_y = start_y + offset_y;
		
		if (ugtk_schedule_get_cell (sform, current_x, current_y, &row, &col)) {
			if (sform->state[row][col] != sform->last_state) {
				sform->state[row][col] = sform->last_state;
				gtk_widget_queue_draw (sform->drawing);
			}
		}
	}
}
static void on_enable_toggled (GtkCheckButton* togglebutton, struct UgtkScheduleForm* sform)
{
	gboolean  active;

	active = gtk_check_button_get_active (togglebutton);
	gtk_widget_set_sensitive (sform->drawing, active);
	gtk_widget_set_sensitive (sform->caption, active);
}

static void on_draw_callback (GtkDrawingArea *area, cairo_t *cr, int width, int height, gpointer user_data)
{
    struct UgtkScheduleForm* sform = (struct UgtkScheduleForm*) user_data;
 	int            x, y;
 	gdouble        cx, cy, ox;
 	gboolean       sensitive;
 	PangoContext*  context;
 	PangoLayout*   layout;

 	sensitive = gtk_widget_get_sensitive (GTK_WIDGET(area));
 	if (sensitive == FALSE) {
// 		return FALSE;
 	}

 	// draw background
 	cairo_set_source_rgb (cr, 1, 1, 1);     // white
// 	cairo_paint (cr);                       // full
 	cairo_rectangle (cr, 0, 0, width, height); 
 	cairo_fill (cr);

 	cairo_set_line_width (cr, 1);

 	// draw lines
 	if (sensitive)
 		cairo_set_source_rgb (cr, 0, 0, 0);
 	else
 		cairo_set_source_rgb (cr,
 				COLOR_DISABLE_R,
 				COLOR_DISABLE_G,
 				COLOR_DISABLE_B);

 	ox = 0;
 	for (cx = 0.5, x = 0;  x < 26;  x++, cx+=UgtkGrid.width_and_line) {
 		if (x == 1) {
 			// skip 1st column
 			cx -= (UgtkGrid.width_and_line - 30);
 			ox = 30 - UgtkGrid.width_and_line;
 			continue;
 		}
 		cairo_move_to (cr, cx, 0);
 		cairo_line_to (cr, cx, UgtkGrid.height_all);
 		cairo_stroke (cr);
 	}
 	for (cy = 0.5, y = 0;  y < 8;  y++, cy+=UgtkGrid.height_and_line) {
 		cairo_move_to (cr, 0, cy);
 		cairo_line_to (cr, UgtkGrid.width_all + ox + 1, cy);
 		cairo_stroke (cr);
 	}

    sform->drawing_offset = ox;

 	// draw text
 	// context = gdk_pango_context_get ();
 	context = gtk_widget_get_pango_context (GTK_WIDGET(area));
 	layout = pango_layout_new (context);
// 	g_object_unref (context);
 	for (cy = 1.0, y = 0;  y < 7;  y++, cy+=UgtkGrid.height_and_line) {
 		pango_layout_set_text (layout, gettext (week_days[y]), -1);
 		cairo_move_to (cr, 2, cy);
 		pango_cairo_show_layout (cr, layout);
 	}
 	g_object_unref (layout);

 	// draw rects
 	if (sensitive == FALSE) {
 		cairo_set_source_rgb (cr,
 				COLOR_DISABLE_R,
 				COLOR_DISABLE_G,
 				COLOR_DISABLE_B);
 	}
 	for (cy = 1.5, y = 0;  y < 7;  y++, cy+=UgtkGrid.height_and_line) {
 		for (cx = 1.5+ox, x = 0;  x < 24;  x++, cx+=UgtkGrid.width_and_line) {
 			if (sensitive) {
 				cairo_set_source_rgb (cr,
 						colors [sform->state[y][x]][0],
 						colors [sform->state[y][x]][1],
 						colors [sform->state[y][x]][2]);
 			}
 			cairo_rectangle (cr,
 					cx,
 					cy,
 					UgtkGrid.width  - 0.5,
 					UgtkGrid.height - 0.5);
 			cairo_fill (cr);
 		}
 	}

// 	return FALSE;
}

/*
static gboolean on_button_press_event (GtkWidget *widget, GdkEventMotion *event, struct UgtkScheduleForm* sform)
{
    // ... code ...
    return TRUE;
}

static gboolean on_motion_notify_event (GtkWidget *widget, GdkEventMotion *event, struct UgtkScheduleForm* sform)
{
    // ... code ...
    return TRUE;
}

static gboolean on_leave_notify_event (GtkWidget* menu, GdkEventCrossing* event, struct UgtkScheduleForm* sform)
{
	gtk_label_set_text (sform->time_tips, "");
	return TRUE;
}
*/


// ----------------------------------------------------------------------------
// UgtkGrid
//
static void  ugtk_grid_global_init (int width, int height)
{
	UgtkGrid.width  = width;
	UgtkGrid.height = height;
	UgtkGrid.width_and_line  = UgtkGrid.width  + 1;
	UgtkGrid.height_and_line = UgtkGrid.height + 1;
	UgtkGrid.width_all  = UgtkGrid.width_and_line * 24 + 1;
	UgtkGrid.height_all = UgtkGrid.height_and_line * 7 + 1;
}

static GtkWidget*  ugtk_grid_new (const gdouble* rgb_array)
{
	GtkWidget*  widget;

	widget = gtk_drawing_area_new ();
	gtk_widget_set_size_request (widget, UgtkGrid.width + 2, UgtkGrid.height + 2);
	// gtk_widget_add_events (widget, GDK_POINTER_MOTION_MASK);

	// gtk_widget_add_events (widget, GDK_POINTER_MOTION_MASK);

	// g_signal_connect (widget, "draw",
	// 		G_CALLBACK (ugtk_grid_draw), (gpointer) rgb_array);
    gtk_drawing_area_set_draw_func (GTK_DRAWING_AREA (widget), ugtk_grid_draw, (gpointer) rgb_array, NULL);

	return widget;
}

static void ugtk_grid_draw (GtkDrawingArea *area, cairo_t *cr, int width_in, int height_in, gpointer user_data)
{
    const gdouble* rgb_array = (const gdouble*) user_data;
 	gdouble        x, y, width, height;

 	x = 0.5;
 	y = 0.5;
 	width  = (gdouble) (width_in - 1);
 	height = (gdouble) (height_in - 1);
 	cairo_set_line_width (cr, 1);
 	cairo_rectangle (cr, x, y, width, height);
 	cairo_stroke (cr);
 	if (gtk_widget_get_sensitive (GTK_WIDGET(area))) {
 		cairo_set_source_rgb (cr,
 				rgb_array [0],
 				rgb_array [1],
 				rgb_array [2]);
 	}
 	else {
 		cairo_set_source_rgb (cr,
 				COLOR_DISABLE_R,
 				COLOR_DISABLE_G,
 				COLOR_DISABLE_B);
 	}
 	cairo_rectangle (cr, x + 1.0, y + 1.0, width - 2.0, height - 2.0);
 	cairo_fill (cr);
}


