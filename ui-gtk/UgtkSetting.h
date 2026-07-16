/*
 *
 *   Copyright (C) 2012-2020 by C.H. Huang
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

#ifndef UGTK_SETTING_H
#define UGTK_SETTING_H

#ifdef	HAVE_CONFIG_H
#include <config.h>
#endif

#include <UgArray.h>
#include <UgList.h>
#include <UgEntry.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgtkSetting    UgtkSetting;

typedef enum
{
	UGTK_SCHEDULE_TURN_OFF,
	UGTK_SCHEDULE_UPLOAD_ONLY,	  // reserve
	UGTK_SCHEDULE_LIMITED_SPEED,
	UGTK_SCHEDULE_NORMAL,

	UGTK_SCHEDULE_N_STATE,
} UgtkScheduleState;

typedef enum
{
	UGTK_PLUGIN_ORDER_CURL,
	UGTK_PLUGIN_ORDER_ARIA2,
	UGTK_PLUGIN_ORDER_CURL_ARIA2,
	UGTK_PLUGIN_ORDER_ARIA2_CURL,

	UGTK_PLUGIN_N_ORDER,
} UgtkPluginOrder;

struct UgtkSetting
{
	// "WindowSetting"
	struct UgtkWindowSetting
	{
		// visible: boolean
		int    toolbar;
		int    statusbar;
		int    category;
		int    summary;
		int    banner;

		// window position: int
		int    x;
		int    y;
		int    width;
		int    height;
		// window ststus: boolean
		int    maximized;

		// user action
		int    nth_category;
		int    nth_state;
		int    paned_position_h;
		int    paned_position_v;
	} window;

	// "SummarySetting"
	struct UgtkSummarySetting
	{
		// visible: boolean
		int    name;
		int    folder;
		int    category;
		int    uri;
		int    message;
	} summary;

	// "DownloadColumn"
	struct UgtkDownloadColumnSetting
	{
		// visible: boolean
		int    complete;
		int    total;
		int    percent;
		int    elapsed;    // consuming time
		int    left;       // remaining time
		int    speed;
		int    upload_speed;
		int    uploaded;
		int    ratio;
		int    retry;
		int    category;
		int    uri;
		int    added_on;
		int    completed_on;

		// width: integer
		struct UgtkDownloadColumnWidth
		{
			int    state;      // state icon
			int    name;
			int    complete;
			int    total;
			int    percent;
			int    elapsed;    // consuming time
			int    left;       // remaining time
			int    speed;
			int    upload_speed;
			int    uploaded;
			int    ratio;
			int    retry;
			int    category;
			int    uri;
			int    added_on;
			int    completed_on;
		} width;

		struct
		{
			int    type;     // GtkSortType
			int    nth;      // UgtkNodeColumn
		} sort;
	} download_column;

	// "UserInterface"
	struct UgtkUserInterfaceSetting
	{
		// boolean
		int    exit_confirmation;
		int    delete_confirmation;
		int    show_trayicon;
		int    start_in_tray;
		int    close_to_tray;
		int    start_in_offline_mode;
		int    start_notification;
		int    sound_notification;
		int    apply_recent;
		int    skip_existing;
		int    large_icon;
#ifdef HAVE_APP_INDICATOR
		int    app_indicator;
#endif
	} ui;

	// "ClipboardSetting"
	struct UgtkClipboardSetting
	{
		char*  pattern;
		int    monitor;
		int    quiet;
		int    nth_category;
		int    website;
	} clipboard;

	// "BandwidthSetting" - global speed limits
	struct UgtkBandwidthSetting
	{
		struct {
			int     upload;    // KiB / second
			int     download;  // KiB / second
		} normal, scheduler;
	} bandwidth;

	// "SchedulerSetting"
	struct UgtkSchedulerSetting
	{
		int         enable;
		UgArrayInt  state;     // [7][24] 1 week, 7 days, 24 hours
	} scheduler;

	// "CommandlineSetting"
	struct UgtkCommandlineSetting
	{
		int    quiet;          // --quiet
		int    nth_category;   // --category-index
	} commandline;

	// "PluginOrder"
	int    plugin_order;           // UgtkPluginOrder: matching order

	// UgetPluginAria2 option
	struct UgtkPluginAria2Setting {
		// aria2 speed limit
		struct {
			int    download;    // KiB / second
			int    upload;      // KiB / second
		} limit;
		// aria2 options
		int    launch;
		int    shutdown;
		char*  token;
		char*  path;
		char*  args;
		char*  uri;
	} aria2;

	// External URL resolver (yt-dlp)
	struct UgtkResolverSetting {
		int    enable;          // master switch
		int    max_height;      // 0 = best, otherwise pixel height
		char*  preferred_ext;   // NULL/"" = best, else "mp4"/"webm"/"mkv"
		int    auto_pick;       // skip selector, just pick best matching
		char*  path;            // NULL/"" = look up in PATH
	} resolver;

	// Completion Auto-Actions
	struct UgtkCompletionSetting {
		int    remember;
		int    action;
		char*  command;
		char*  on_error;
	} completion;

	struct UgtkAutoSaveSetting
	{
		int    enable;
		int    interval;
	} auto_save;

	// "FolderHistory"
	UgList     folder_history;

	int        offline_mode;
};

void  ugtk_setting_init (UgtkSetting* setting);
int   ugtk_setting_save (UgtkSetting* setting, const char* file);
int   ugtk_setting_load (UgtkSetting* setting, const char* file);
void  ugtk_setting_reset (UgtkSetting* setting);

void  ugtk_setting_add_folder (UgtkSetting* setting, const char* folder);
void  ugtk_setting_fix_data (UgtkSetting* setting);

#ifdef __cplusplus
}
#endif

#endif // UGTK_SETTING_H
