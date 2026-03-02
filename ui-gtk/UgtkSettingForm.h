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


#ifndef UGTK_SETTING_FORM_H
#define UGTK_SETTING_FORM_H

#include <gtk/gtk.h>
#include <UgtkSetting.h>

#ifdef __cplusplus
extern "C" {
#endif


// ----------------------------------------------------------------------------
// UgtkClipboardForm
struct UgtkClipboardForm
{
	GtkWidget*    self;

	GtkWidget*          pattern;    // GtkTextView
	GtkTextBuffer*      buffer;
	GtkCheckButton*     monitor;
	GtkCheckButton*     quiet;
	// add download to Nth category
	GtkWidget*			nth_label;
	GtkSpinButton*		nth_spin;

	// Monitor media website address
	GtkCheckButton*     website;
};

void  ugtk_clipboard_form_init (struct UgtkClipboardForm* cbform);
void  ugtk_clipboard_form_set (struct UgtkClipboardForm* cbform, UgtkSetting* setting);
void  ugtk_clipboard_form_get (struct UgtkClipboardForm* cbform, UgtkSetting* setting);

// ----------------------------------------------------------------------------
// UgtkUserInterfaceForm
struct UgtkUserInterfaceForm
{
	GtkWidget*    self;

	GtkCheckButton*     confirm_exit;
	GtkCheckButton*     confirm_delete;
	GtkCheckButton*     show_trayicon;
	GtkCheckButton*     start_in_tray;
	GtkCheckButton*     close_to_tray;
	GtkCheckButton*     start_in_offline_mode;
	GtkCheckButton*     start_notification;
	GtkCheckButton*     sound_notification;
	GtkCheckButton*     apply_recent;
	GtkCheckButton*     skip_existing;
	GtkCheckButton*     large_icon;
};

void  ugtk_user_interface_form_init (struct UgtkUserInterfaceForm* uiform);
void  ugtk_user_interface_form_set (struct UgtkUserInterfaceForm* uiform, UgtkSetting* setting);
void  ugtk_user_interface_form_get (struct UgtkUserInterfaceForm* uiform, UgtkSetting* setting);

// ----------------------------------------------------------------------------
// UgtkBandwidthForm

struct UgtkBandwidthForm
{
	GtkWidget*  self;

	GtkSpinButton*    upload;
	GtkSpinButton*    download;
};

void  ugtk_bandwidth_form_init (struct UgtkBandwidthForm* bwform);
void  ugtk_bandwidth_form_set (struct UgtkBandwidthForm* bwform, UgtkSetting* setting);
void  ugtk_bandwidth_form_get (struct UgtkBandwidthForm* bwform, UgtkSetting* setting);

// ----------------------------------------------------------------------------
// UgtkCompletionForm
struct UgtkCompletionForm
{
	GtkWidget*  self;

	GtkEntry*   command;
	GtkEntry*   on_error;
};

void  ugtk_completion_form_init (struct UgtkCompletionForm* csform);
void  ugtk_completion_form_set (struct UgtkCompletionForm* csform, UgtkSetting* setting);
void  ugtk_completion_form_get (struct UgtkCompletionForm* csform, UgtkSetting* setting);

// ----------------------------------------------------------------------------
// UgtkAutoSaveForm
struct UgtkAutoSaveForm
{
	GtkWidget*  self;

	// auto save and interval
	GtkCheckButton*   enable;
	GtkWidget*        interval_label;
	GtkSpinButton*    interval_spin;
	GtkWidget*        minutes_label;  // minutes
};

void  ugtk_auto_save_form_init (struct UgtkAutoSaveForm* asform);
void  ugtk_auto_save_form_set (struct UgtkAutoSaveForm* asform, UgtkSetting* setting);
void  ugtk_auto_save_form_get (struct UgtkAutoSaveForm* asform, UgtkSetting* setting);

// ----------------------------------------------------------------------------
// UgtkCommandlineForm
struct UgtkCommandlineForm
{
	GtkWidget*  self;

	// --quiet
	GtkCheckButton*     quiet;
	// --category-index
	GtkWidget*          index_label;
	GtkSpinButton*      index_spin;
};

void  ugtk_commandline_form_init (struct UgtkCommandlineForm* clform);
void  ugtk_commandline_form_set (struct UgtkCommandlineForm* clform, UgtkSetting* setting);
void  ugtk_commandline_form_get (struct UgtkCommandlineForm* clform, UgtkSetting* setting);

// ----------------------------------------------------------------------------
// UgtkPluginForm
struct UgtkPluginForm
{
	GtkWidget*  self;

	GtkDropDown*        order;

	// Aria2 options
	GtkWidget*          aria2_opts;
	GtkCheckButton*     launch;
	GtkCheckButton*     shutdown;
	GtkEntry*           uri;
	GtkEntry*           token;
	GtkWidget*          local;  // GtkFrame
	GtkEntry*           path;
	GtkWidget*          args;   // GtkTextView
	GtkTextBuffer*      args_buffer;
	// Speed Limits for Aria2
	GtkSpinButton*      upload;    // KiB/s
	GtkSpinButton*      download;  // KiB/s
};

void  ugtk_plugin_form_init (struct UgtkPluginForm* psform);
void  ugtk_plugin_form_set (struct UgtkPluginForm* psform, UgtkSetting* setting);
void  ugtk_plugin_form_get (struct UgtkPluginForm* psform, UgtkSetting* setting);

// ----------------------------------------------------------------------------
// UgtkMediaWebsiteForm
struct UgtkMediaWebsiteForm
{
	GtkWidget*  self;

	GtkDropDown*        match_mode;
	GtkDropDown*        quality;
	GtkDropDown*        type;
};

void  ugtk_media_website_form_init (struct UgtkMediaWebsiteForm* mwform);
void  ugtk_media_website_form_set (struct UgtkMediaWebsiteForm* mwform, UgtkSetting* setting);
void  ugtk_media_website_form_get (struct UgtkMediaWebsiteForm* mwform, UgtkSetting* setting);


#ifdef __cplusplus
}
#endif

#endif // End of UGTK_SETTING_FORM_H
