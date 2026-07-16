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

#include <UgStdio.h>
#include <UgString.h>
#include <UgJsonFile.h>
#include <UgtkSetting.h>
#include <UgtkNodeView.h>

#define UGTK_APP_CLIPBOARD_PATTERN  "BIN|ZIP|GZ|7Z|XZ|Z|TAR|TGZ|BZ2|"   \
                                    "LZH|A[0-9]?|RAR|R[0-9][0-9]|ISO|"  \
                                    "RPM|DEB|EXE|MSI|APK|"              \
                                    "3GP|AAC|FLAC|M4A|M4P|MP3|OGG|WAV|WMA|" \
                                    "MP4|MKV|WEBM|OGV|AVI|MOV|WMV|FLV|F4V|MPG|MPEG|RMVB"

#define UGTK_ARIA2_PATH    "aria2c"

#define UGTK_ARIA2_ARGS    "--enable-rpc=true -D " \
                           "--disable-ipv6 "       \
                           "--check-certificate=false"

#define UGTK_ARIA2_URI     "http://localhost:6800/jsonrpc"

// ----------------------------------------------------------------------------
// WindowSetting

static const UgEntry  UgtkWindowSettingEntry[] =
{
	{"Toolbar",   offsetof (struct UgtkWindowSetting, toolbar),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"Statusbar", offsetof (struct UgtkWindowSetting, statusbar),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"Category",  offsetof (struct UgtkWindowSetting, category),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"Summary",   offsetof (struct UgtkWindowSetting, summary),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"Banner",    offsetof (struct UgtkWindowSetting, banner),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"x",         offsetof (struct UgtkWindowSetting, x),
			UG_ENTRY_INT,  NULL,  NULL},
	{"y",         offsetof (struct UgtkWindowSetting, y),
			UG_ENTRY_INT,  NULL,  NULL},
	{"width",     offsetof (struct UgtkWindowSetting, width),
			UG_ENTRY_INT,  NULL,  NULL},
	{"height",    offsetof (struct UgtkWindowSetting, height),
			UG_ENTRY_INT,  NULL,  NULL},
	{"maximized", offsetof (struct UgtkWindowSetting, maximized),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"NthCategory", offsetof (struct UgtkWindowSetting, nth_category),
			UG_ENTRY_INT,  NULL,  NULL},
	{"NthStatus",   offsetof (struct UgtkWindowSetting, nth_state),
			UG_ENTRY_INT,  NULL,  NULL},
	{"PanedPositionH", offsetof (struct UgtkWindowSetting, paned_position_h),
			UG_ENTRY_INT,  NULL,  NULL},
	{"PanedPositionV", offsetof (struct UgtkWindowSetting, paned_position_v),
			UG_ENTRY_INT,  NULL,  NULL},
	{NULL},    // null-terminated
};

// ----------------------------------------------------------------------------
// SummarySetting

static const UgEntry  UgtkSummarySettingEntry[] =
{
	{"name",     offsetof (struct UgtkSummarySetting, name),
			UG_ENTRY_BOOL,	NULL,	NULL},
	{"folder",   offsetof (struct UgtkSummarySetting, folder),
			UG_ENTRY_BOOL,	NULL,	NULL},
	{"category", offsetof (struct UgtkSummarySetting, category),
			UG_ENTRY_BOOL,	NULL,	NULL},
	{"uri",      offsetof (struct UgtkSummarySetting, uri),
			UG_ENTRY_BOOL,	NULL,	NULL},
	{"message",  offsetof (struct UgtkSummarySetting, message),
			UG_ENTRY_BOOL,	NULL,	NULL},
	{NULL}    // null-terminated
};

// ----------------------------------------------------------------------------
// DownloadColumnSetting

static const UgEntry  UgtkDownloadColumnWidthEntry[] =
{
	{"state",       offsetof (struct UgtkDownloadColumnWidth, state),
			UG_ENTRY_INT,  NULL,  NULL},
	{"name",        offsetof (struct UgtkDownloadColumnWidth, name),
			UG_ENTRY_INT,  NULL,  NULL},
	{"complete",    offsetof (struct UgtkDownloadColumnWidth, complete),
			UG_ENTRY_INT,  NULL,  NULL},
	{"total",       offsetof (struct UgtkDownloadColumnWidth, total),
			UG_ENTRY_INT,  NULL,  NULL},
	{"percent",     offsetof (struct UgtkDownloadColumnWidth, percent),
			UG_ENTRY_INT,  NULL,  NULL},
	{"elapsed",     offsetof (struct UgtkDownloadColumnWidth, elapsed),
			UG_ENTRY_INT,  NULL,  NULL},
	{"left",        offsetof (struct UgtkDownloadColumnWidth, left),
			UG_ENTRY_INT,  NULL,  NULL},
	{"speed",       offsetof (struct UgtkDownloadColumnWidth, speed),
			UG_ENTRY_INT,  NULL,  NULL},
	{"UploadSpeed", offsetof (struct UgtkDownloadColumnWidth, upload_speed),
			UG_ENTRY_INT,  NULL,  NULL},
	{"uploaded",    offsetof (struct UgtkDownloadColumnWidth, uploaded),
			UG_ENTRY_INT,  NULL,  NULL},
	{"ratio",       offsetof (struct UgtkDownloadColumnWidth, ratio),
			UG_ENTRY_INT,  NULL,  NULL},
	{"retry",       offsetof (struct UgtkDownloadColumnWidth, retry),
			UG_ENTRY_INT,  NULL,  NULL},
	{"category",    offsetof (struct UgtkDownloadColumnWidth, category),
			UG_ENTRY_INT,  NULL,  NULL},
	{"uri",         offsetof (struct UgtkDownloadColumnWidth, uri),
			UG_ENTRY_INT,  NULL,  NULL},
	{"AddedOn",     offsetof (struct UgtkDownloadColumnWidth, added_on),
			UG_ENTRY_INT,  NULL,  NULL},
	{"CompletedOn", offsetof (struct UgtkDownloadColumnWidth, completed_on),
			UG_ENTRY_INT,  NULL,  NULL},
	{NULL}    // null-terminated
};

static const UgEntry  UgtkDownloadColumnSettingEntry[] =
{
	{"complete",    offsetof (struct UgtkDownloadColumnSetting, complete),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"total",       offsetof (struct UgtkDownloadColumnSetting, total),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"percent",     offsetof (struct UgtkDownloadColumnSetting, percent),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"elapsed",     offsetof (struct UgtkDownloadColumnSetting, elapsed),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"left",        offsetof (struct UgtkDownloadColumnSetting, left),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"speed",       offsetof (struct UgtkDownloadColumnSetting, speed),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"UploadSpeed", offsetof (struct UgtkDownloadColumnSetting, upload_speed),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"uploaded",    offsetof (struct UgtkDownloadColumnSetting, uploaded),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"ratio",       offsetof (struct UgtkDownloadColumnSetting, ratio),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"retry",       offsetof (struct UgtkDownloadColumnSetting, retry),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"category",    offsetof (struct UgtkDownloadColumnSetting, category),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"uri",         offsetof (struct UgtkDownloadColumnSetting, uri),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"AddedOn",     offsetof (struct UgtkDownloadColumnSetting, added_on),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"CompletedOn", offsetof (struct UgtkDownloadColumnSetting, completed_on),
			UG_ENTRY_BOOL,  NULL,  NULL},

	{"SortType",    offsetof (struct UgtkDownloadColumnSetting, sort.type),
			UG_ENTRY_INT,  NULL,  NULL},
	{"SortNth",     offsetof (struct UgtkDownloadColumnSetting, sort.nth),
			UG_ENTRY_INT,  NULL,  NULL},

	{"Width",       offsetof (struct UgtkDownloadColumnSetting, width),
			UG_ENTRY_OBJECT, (void*) UgtkDownloadColumnWidthEntry, NULL},

	// deprecated
	{"completed",   offsetof (struct UgtkDownloadColumnSetting, complete),
			UG_ENTRY_BOOL,  NULL,  NULL},

	{NULL}    // null-terminated
};

// ----------------------------------------------------------------------------
// UserInterfaceSetting

static const UgEntry  UgtkUserInterfaceSettingEntry[] =
{
	{"ExitConfirmation",   offsetof (struct UgtkUserInterfaceSetting, exit_confirmation),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"DeleteConfirmation", offsetof (struct UgtkUserInterfaceSetting, delete_confirmation),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"ShowTrayIcon",       offsetof (struct UgtkUserInterfaceSetting, show_trayicon),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"StartInTray",        offsetof (struct UgtkUserInterfaceSetting, start_in_tray),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"CloseToTray",        offsetof (struct UgtkUserInterfaceSetting, close_to_tray),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"StartInOfflineMode", offsetof (struct UgtkUserInterfaceSetting, start_in_offline_mode),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"StartNotification",  offsetof (struct UgtkUserInterfaceSetting, start_notification),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"SoundNotification",  offsetof (struct UgtkUserInterfaceSetting, sound_notification),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"ApplyRecently",      offsetof (struct UgtkUserInterfaceSetting, apply_recent),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"ApplyRecent",        offsetof (struct UgtkUserInterfaceSetting, apply_recent),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"SkipExisting",       offsetof (struct UgtkUserInterfaceSetting, skip_existing),
			UG_ENTRY_BOOL,  NULL,  NULL},
	{"LargeIcon",          offsetof (struct UgtkUserInterfaceSetting, large_icon),
			UG_ENTRY_BOOL,  NULL,  NULL},
#ifdef HAVE_APP_INDICATOR
	{"AppIndicator",       offsetof (struct UgtkUserInterfaceSetting, app_indicator),
			UG_ENTRY_BOOL,  NULL,  NULL},
#endif
	{NULL},    // null-terminated
};

// ----------------------------------------------------------------------------
// ClipboardSetting

static const UgEntry  UgtkClipboardSettingEntry[] =
{
	{"pattern",      offsetof (struct UgtkClipboardSetting, pattern),
			UG_ENTRY_STRING, NULL,  NULL},
	{"monitor",      offsetof (struct UgtkClipboardSetting, monitor),
			UG_ENTRY_BOOL,   NULL,  NULL},
	{"quiet",        offsetof (struct UgtkClipboardSetting, quiet),
			UG_ENTRY_BOOL,   NULL,  NULL},
	{"NthCategory",  offsetof (struct UgtkClipboardSetting, nth_category),
			UG_ENTRY_INT,    NULL,  NULL},

	{"Website", offsetof (struct UgtkClipboardSetting, website),
			UG_ENTRY_BOOL,   NULL,  NULL},

	// remove this in future verison
	{"MediaWebsite", offsetof (struct UgtkClipboardSetting, website),
			UG_ENTRY_BOOL,   NULL,  NULL},
	{NULL},    // null-terminated
};

// ----------------------------------------------------------------------------
// BandwidthSetting

static const UgEntry  UgtkBandwidthSettingEntry[] =
{
	{"NormalUpload",      offsetof (struct UgtkBandwidthSetting, normal.upload),
			UG_ENTRY_INT, NULL, NULL},
	{"NormalDownload",    offsetof (struct UgtkBandwidthSetting, normal.download),
			UG_ENTRY_INT, NULL, NULL},
	{"SchedulerUpload",   offsetof (struct UgtkBandwidthSetting, scheduler.upload),
			UG_ENTRY_INT, NULL, NULL},
	{"SchedulerDownload", offsetof (struct UgtkBandwidthSetting, scheduler.download),
			UG_ENTRY_INT, NULL, NULL},
	{NULL},    // null-terminated
};

// ----------------------------------------------------------------------------
// SchedulerSetting

static const UgEntry  UgtkSchedulerSettingEntry[] =
{
	{"enable", offsetof (struct UgtkSchedulerSetting, enable),
			UG_ENTRY_BOOL,  NULL, NULL},
	{"state",  offsetof (struct UgtkSchedulerSetting, state),
			UG_ENTRY_ARRAY, ug_json_parse_array_int, ug_json_write_array_int},
	{NULL},    // null-terminated
};

// ----------------------------------------------------------------------------
// CommandlineSetting

static const UgEntry  UgtkCommandlineSettingEntry[] =
{
	{"quiet",         offsetof (struct UgtkCommandlineSetting, quiet),
			UG_ENTRY_BOOL, NULL, NULL},
	{"NthCategory",   offsetof (struct UgtkCommandlineSetting, nth_category),
			UG_ENTRY_INT,  NULL, NULL},
	{NULL},    // null-terminated
};

// ----------------------------------------------------------------------------
// PluginAria2Setting

static const UgEntry  UgtkPluginAria2SettingEntry[] =
{
	{"launch",    offsetof (struct UgtkPluginAria2Setting, launch),
			UG_ENTRY_BOOL,  NULL,   NULL},
	{"shutdown",  offsetof (struct UgtkPluginAria2Setting, shutdown),
			UG_ENTRY_BOOL,  NULL,   NULL},
	{"token",     offsetof (struct UgtkPluginAria2Setting, token),
			UG_ENTRY_STRING,  NULL,   NULL},
	{"path",      offsetof (struct UgtkPluginAria2Setting, path),
			UG_ENTRY_STRING,  NULL,   NULL},
	{"arguments", offsetof (struct UgtkPluginAria2Setting, args),
			UG_ENTRY_STRING,  NULL,   NULL},
	{"uri",       offsetof (struct UgtkPluginAria2Setting, uri),
			UG_ENTRY_STRING,  NULL,   NULL},
	{"MaxUploadSpeed",   offsetof (struct UgtkPluginAria2Setting, limit.upload),
			UG_ENTRY_INT,     NULL,   NULL},
	{"MaxDownloadSpeed", offsetof (struct UgtkPluginAria2Setting, limit.download),
			UG_ENTRY_INT,     NULL,   NULL},
	{NULL},    // null-terminated
};

// ----------------------------------------------------------------------------
// UgtkResolverSetting

static const UgEntry  UgtkResolverSettingEntry[] =
{
	{"enable",        offsetof (struct UgtkResolverSetting, enable),
			UG_ENTRY_BOOL,   NULL, NULL},
	{"max_height",    offsetof (struct UgtkResolverSetting, max_height),
			UG_ENTRY_INT,    NULL, NULL},
	{"preferred_ext", offsetof (struct UgtkResolverSetting, preferred_ext),
			UG_ENTRY_STRING, NULL, NULL},
	{"auto_pick",     offsetof (struct UgtkResolverSetting, auto_pick),
			UG_ENTRY_BOOL,   NULL, NULL},
	{"path",          offsetof (struct UgtkResolverSetting, path),
			UG_ENTRY_STRING, NULL, NULL},
	{NULL},
};

// ----------------------------------------------------------------------------
// UgtkCompletionSetting

static const UgEntry  UgtkCompletionSettingEntry[] =
{
	{"remember",  offsetof (struct UgtkCompletionSetting, remember),
			UG_ENTRY_BOOL,  NULL,   NULL},
	{"action",    offsetof (struct UgtkCompletionSetting, action),
			UG_ENTRY_INT,   NULL,   NULL},
	{"command",   offsetof (struct UgtkCompletionSetting, command),
			UG_ENTRY_STRING,  NULL,   NULL},
	{"OnError",   offsetof (struct UgtkCompletionSetting, on_error),
			UG_ENTRY_STRING,  NULL,   NULL},
	{NULL},    // null-terminated
};

// ----------------------------------------------------------------------------
// UgtkSetting

static const UgEntry  UgtkSettingEntry[] =
{
	{"Window",          offsetof (UgtkSetting, window),
			UG_ENTRY_OBJECT, (void*) UgtkWindowSettingEntry,  NULL},
	{"Summary",         offsetof (UgtkSetting, summary),
			UG_ENTRY_OBJECT, (void*) UgtkSummarySettingEntry, NULL},
	{"DownloadColumn",  offsetof (UgtkSetting, download_column),
			UG_ENTRY_OBJECT, (void*) UgtkDownloadColumnSettingEntry, NULL},
	{"UserInterface",   offsetof (UgtkSetting, ui),
			UG_ENTRY_OBJECT, (void*) UgtkUserInterfaceSettingEntry,  NULL},
	{"Clipboard",       offsetof (UgtkSetting, clipboard),
			UG_ENTRY_OBJECT, (void*) UgtkClipboardSettingEntry,   NULL},
	{"Bandwidth",       offsetof (UgtkSetting, bandwidth),
			UG_ENTRY_OBJECT, (void*) UgtkBandwidthSettingEntry,  NULL},
	{"Commandline",     offsetof (UgtkSetting, commandline),
			UG_ENTRY_OBJECT, (void*) UgtkCommandlineSettingEntry, NULL},

	{"PluginOrder",     offsetof (UgtkSetting, plugin_order),
			UG_ENTRY_INT,    NULL, NULL},
	{"PluginAria2",     offsetof (UgtkSetting, aria2),
			UG_ENTRY_OBJECT, (void*) UgtkPluginAria2SettingEntry, NULL},
	{"Resolver",        offsetof (UgtkSetting, resolver),
			UG_ENTRY_OBJECT, (void*) UgtkResolverSettingEntry, NULL},

	{"Completion",      offsetof (UgtkSetting, completion),
			UG_ENTRY_OBJECT, (void*) UgtkCompletionSettingEntry, NULL},

	{"FolderHistory",   offsetof (UgtkSetting, folder_history),
			UG_ENTRY_ARRAY, ug_json_parse_list_string,  ug_json_write_list_string},

	{"AutoSave",        offsetof (UgtkSetting, auto_save.enable),
			UG_ENTRY_INT,    NULL,  NULL},
	{"AutoSaveInterval",offsetof (UgtkSetting, auto_save.interval),
			UG_ENTRY_INT,    NULL,  NULL},
//	{"OfflineMode",     offsetof (UgtkSetting, offline_mode),
//			UG_ENTRY_BOOL,   NULL,  NULL},

	{"Scheduler",       offsetof (UgtkSetting, scheduler),
			UG_ENTRY_OBJECT, (void*) UgtkSchedulerSettingEntry,   NULL},
	{NULL},    // null-terminated
};

// ----------------------------------------------------------------------------
// "UgSetting" functions
//
void  ugtk_setting_init (UgtkSetting* setting)
{
	// "SchedulerSetting"
	ug_array_init (&setting->scheduler.state, sizeof (int), 7*24);
	memset (setting->scheduler.state.at, UGTK_SCHEDULE_NORMAL, 7*24);

	setting->clipboard.website = TRUE;

	// External URL resolver defaults
	setting->resolver.enable = TRUE;
	setting->resolver.max_height = 0;       // 0 = best
	setting->resolver.preferred_ext = NULL; // NULL = best
	setting->resolver.auto_pick = FALSE;
	setting->resolver.path = NULL;          // NULL = look up in PATH
}

void  ugtk_setting_reset (UgtkSetting* setting)
{
	unsigned int  weekdays, dayhours;

	// "WindowSetting"
	setting->window.toolbar   = TRUE;
	setting->window.statusbar = TRUE;
	setting->window.category  = TRUE;
	setting->window.summary   = TRUE;
	setting->window.banner    = TRUE;
	setting->window.x = 0;
	setting->window.y = 0;
	setting->window.width = 0;
	setting->window.height = 0;
	setting->window.maximized = FALSE;
	setting->window.paned_position_h = -1;
	setting->window.paned_position_v = -1;

	// "SummarySetting"
	setting->summary.name     = TRUE;
	setting->summary.folder   = TRUE;
	setting->summary.category = FALSE;
	setting->summary.uri      = FALSE;
	setting->summary.message  = TRUE;

	// "DownloadColumnSetting"
	setting->download_column.complete     = TRUE;
	setting->download_column.total        = TRUE;
	setting->download_column.percent      = TRUE;
	setting->download_column.elapsed      = TRUE;
	setting->download_column.left         = TRUE;
	setting->download_column.speed        = TRUE;
	setting->download_column.upload_speed = TRUE;
	setting->download_column.uploaded     = FALSE;
	setting->download_column.ratio        = TRUE;
	setting->download_column.retry        = TRUE;
	setting->download_column.category     = FALSE;
	setting->download_column.uri          = FALSE;
	setting->download_column.added_on     = TRUE;
	setting->download_column.completed_on = FALSE;
	// default sorted column
	setting->download_column.sort.type    = GTK_SORT_DESCENDING;
	setting->download_column.sort.nth     = UGTK_NODE_COLUMN_STATE;
	// "DownloadColumnWidth"
	memset (&setting->download_column.width, 0,
			sizeof (setting->download_column.width));

	// "UserInterfaceSetting"
	setting->ui.exit_confirmation = TRUE;
	setting->ui.delete_confirmation = TRUE;
	setting->ui.show_trayicon = TRUE;
	setting->ui.start_in_tray = FALSE;
	setting->ui.close_to_tray = TRUE;
	setting->ui.start_in_offline_mode = FALSE;
	setting->ui.start_notification = TRUE;
	setting->ui.sound_notification = TRUE;
	setting->ui.apply_recent = TRUE;
	setting->ui.skip_existing = FALSE;
	setting->ui.large_icon = FALSE;
#ifdef HAVE_APP_INDICATOR
	setting->ui.app_indicator = TRUE;
#endif

	// "ClipboardSetting"
	ug_free (setting->clipboard.pattern);
	setting->clipboard.pattern = ug_strdup (UGTK_APP_CLIPBOARD_PATTERN);
	setting->clipboard.monitor = TRUE;
	setting->clipboard.quiet = FALSE;
	setting->clipboard.nth_category = 0;
	setting->clipboard.website = TRUE;

	// "BandwidthSetting"
	setting->bandwidth.normal.upload = 0;
	setting->bandwidth.normal.download = 0;
	setting->bandwidth.scheduler.upload = 0;
	setting->bandwidth.scheduler.download = 0;

	// "SchedulerSetting"
	setting->scheduler.enable = FALSE;
	ug_array_init (&setting->scheduler.state, sizeof (int), 7*24);
	setting->scheduler.state.length = 7 * 24;
	for (weekdays = 0;  weekdays < 7;  weekdays++) {
		for (dayhours = 0;  dayhours < 24;  dayhours++) {
			setting->scheduler.state.at[weekdays*24 + dayhours] =
					UGTK_SCHEDULE_NORMAL;
		}
	}

	// "CommandlineSetting"
	setting->commandline.quiet = FALSE;
	setting->commandline.nth_category = 0;

	// "PluginSetting"
	setting->plugin_order = UGTK_PLUGIN_ORDER_CURL;
	// aria2 plug-in settings
	setting->aria2.limit.download = 0;
	setting->aria2.limit.upload = 0;
	setting->aria2.launch = TRUE;
	setting->aria2.shutdown = TRUE;
	setting->aria2.path = ug_strdup (UGTK_ARIA2_PATH);
	setting->aria2.args = ug_strdup (UGTK_ARIA2_ARGS);
	setting->aria2.uri  = ug_strdup (UGTK_ARIA2_URI);

	// External URL resolver
	setting->resolver.enable = TRUE;
	setting->resolver.max_height = 0;
	ug_free (setting->resolver.preferred_ext);
	setting->resolver.preferred_ext = NULL;
	setting->resolver.auto_pick = FALSE;
	ug_free (setting->resolver.path);
	setting->resolver.path = NULL;

	// Others
	setting->completion.remember = TRUE;
	setting->completion.action = 0;
	setting->completion.command = NULL;
	setting->completion.on_error = NULL;
	setting->auto_save.enable = TRUE;
	setting->auto_save.interval = 3;

	setting->offline_mode = FALSE;
}

// ----------------------------------------------------------------------------
// save/load settings

int  ugtk_setting_save (UgtkSetting* setting, const char* file)
{
	int          action;
	char*        path;
	UgJsonFile*  jfile;

	path = g_strconcat (file, ".temp", NULL);
	jfile = ug_json_file_new (4096);
	if (ug_json_file_begin_write (jfile, path, UG_JSON_FORMAT_ALL) == FALSE) {
		ug_json_file_free (jfile);
		g_free (path);
		return FALSE;
	}

	// save completion.action
	action = setting->completion.action;
	if (setting->completion.remember == FALSE)
		setting->completion.action = 0;

	ug_json_write_object_head (&jfile->json);
	ug_json_write_entry (&jfile->json, setting, UgtkSettingEntry);
	ug_json_write_object_tail (&jfile->json);

	// restore completion.action
	if (setting->completion.remember == FALSE)
		setting->completion.action = action;

	ug_json_file_end_write (jfile);
	ug_json_file_free (jfile);

	ug_remove (file);
	ug_rename (path, file);
	g_free (path);

	return TRUE;
}

int  ugtk_setting_load (UgtkSetting* setting, const char* path)
{
	int          file_ok;
	char*        path_temp;
	UgJsonFile*  jfile;

	path_temp = g_strconcat (path, ".temp", NULL);
	jfile = ug_json_file_new (4096);
	file_ok = ug_json_file_begin_parse (jfile, path);
	if (file_ok)
		ug_remove (path_temp);
	else if (ug_rename (path_temp, path) != -1)
		file_ok = ug_json_file_begin_parse (jfile, path);
	g_free (path_temp);

	if (file_ok == FALSE) {
		ug_json_file_free (jfile);
		return FALSE;
	}

	ug_json_push (&jfile->json, ug_json_parse_entry,
			setting, (void*) UgtkSettingEntry);
	ug_json_push (&jfile->json, ug_json_parse_object,
			NULL, NULL);

	ug_json_file_end_parse (jfile);
	ug_json_file_free (jfile);

	// check & fix settings
	ugtk_setting_fix_data (setting);
	return TRUE;
}

void  ugtk_setting_add_folder (UgtkSetting* setting, const char* folder)
{
	UgList* list;
	UgLink* link;

	if (folder == NULL || folder[0] == 0)
		return;

	list = &setting->folder_history;
	for (link = list->head;  link;  link = link->next) {
		if (strcmp (folder, link->data) == 0) {
			ug_list_remove (list, link);
			ug_list_prepend (list, link);
			return;
		}
	}

	if (list->size >= 8) {
		link = list->tail;
		ug_list_remove (list, link);
		ug_free (link->data);
		ug_link_free (link);
	}
	link = ug_link_new ();
	link->data = ug_strdup (folder);
	ug_list_prepend (list, link);
}

void  ugtk_setting_fix_data (UgtkSetting* setting)
{
	unsigned int  index;

	// "DownloadColumnSetting"
	// default sorted column
	if (setting->download_column.sort.type != GTK_SORT_ASCENDING &&
	    setting->download_column.sort.type != GTK_SORT_DESCENDING)
	{
		setting->download_column.sort.type = GTK_SORT_DESCENDING;
	}
	if (setting->download_column.sort.nth < 0 ||
	    setting->download_column.sort.nth >= UGTK_NODE_N_COLUMNS)
	{
		setting->download_column.sort.nth = UGTK_NODE_COLUMN_ADDED_ON;
	}

	// "ClipboardSetting"
	if (setting->clipboard.pattern == NULL ||
	    setting->clipboard.pattern[0] == 0)
	{
		ug_free (setting->clipboard.pattern);
		setting->clipboard.pattern = ug_strdup (UGTK_APP_CLIPBOARD_PATTERN);
	}

	// "SchedulerSetting"
	for (index = 0;  index < 7*24;  index++) {
		if (setting->scheduler.state.at[index] < 0 ||
		    setting->scheduler.state.at[index] >= UGTK_SCHEDULE_N_STATE)
		{
			setting->scheduler.state.at[index] = UGTK_SCHEDULE_NORMAL;
		}
	}

	// "PluginSetting"
	if (setting->plugin_order < 0 ||
	    setting->plugin_order >= UGTK_PLUGIN_N_ORDER)
	{
		setting->plugin_order = UGTK_PLUGIN_ORDER_CURL;
	}
	// aria2 plug-in settings
	if (setting->aria2.path == NULL || setting->aria2.path[0] == 0) {
		ug_free (setting->aria2.path);
		setting->aria2.path = ug_strdup (UGTK_ARIA2_PATH);
	}
	if (setting->aria2.args == NULL || setting->aria2.args[0] == 0) {
		ug_free (setting->aria2.args);
		setting->aria2.args = ug_strdup (UGTK_ARIA2_ARGS);
	}
	if (setting->aria2.uri == NULL || setting->aria2.uri[0] == 0) {
		ug_free (setting->aria2.uri);
		setting->aria2.uri  = ug_strdup (UGTK_ARIA2_URI);
	}
}
