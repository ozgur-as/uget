/*
 *
 *   Copyright (C) 2011-2020 by C.H. Huang
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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <UgUtil.h>
#include <UgStdio.h>
#include <UgValue.h>
#include <UgString.h>
#include <UgJson-custom.h>
#include <UgetData.h>
#include <UgetAria2.h>
#include <UgetPluginAria2.h>

#ifdef HAVE_LIBPWMD
#include "pwmd.h"
static gboolean	uget_plugin_aria2_set_proxy_pwmd (UgetPluginAria2 *plugin, UgInfo* info, UgValue* options);
#endif

#if defined _WIN32 || defined _WIN64
#include <windows.h> // Sleep()
#define  ug_sleep       Sleep
#else
#include <unistd.h>  // sleep(), usleep()
#define  ug_sleep(millisecond)    usleep (millisecond * 1000)
#endif // _WIN32 || _WIN64

#ifdef HAVE_GLIB
#include <glib/gi18n.h>
#undef  printf
#else
#define N_(x)   x
#define  _(x)   x
#endif

static UgJsonrpcObject*  alloc_speed_request(UgetPluginAria2* plugin);
static void              recycle_speed_request(UgJsonrpcObject* object);
static UgJsonrpcObject*  alloc_status_request(UgValue** gid);
static void              recycle_status_request(UgJsonrpcObject* object);

static void* ug_file_to_base64(const char* file, int* length);
static int   decide_file_type(UgetPluginAria2* plugin);
static void  add_uri_mirrors(UgValue* varray, const char* mirrors);

enum UgetPluginAria2UriType {
	URI_UNSUPPORTED,
	URI_NET,
	URI_MAGNET,    // magnet:
	URI_TORRENT,
	URI_METALINK,
};

typedef enum Aria2Status {
	ARIA2_STATUS_ACTIVE,
	ARIA2_STATUS_WAITING, // used by Aria2Uri.status and Aria2Telled.status
	ARIA2_STATUS_PAUSED,
	ARIA2_STATUS_ERROR,
	ARIA2_STATUS_COMPLETE,
	ARIA2_STATUS_REMOVED,

	ARIA2_STATUS_USED,    // used by Aria2Uri.status

	ARIA2_N_STATUS,
} Aria2Status;

// ----------------------------------------------------------------------------
// UgetPluginInfo (derived from UgTypeInfo)

static void plugin_init (UgetPluginAria2* plugin);
static void plugin_final(UgetPluginAria2* plugin);
static int  plugin_ctrl (UgetPluginAria2* plugin, int code, void* data);
static int  plugin_accept(UgetPluginAria2* plugin, UgInfo* node_info);
static int  plugin_sync  (UgetPluginAria2* plugin, UgInfo* node_info);
static UgetResult  global_set(int code, void* parameter);
static UgetResult  global_get(int code, void* parameter);

static const char* schemes[] = {"http", "https", "ftp", "magnet", NULL};
static const char* types[]   = {"torrent", "metalink", "meta4", NULL};

static const UgetPluginInfo UgetPluginAria2InfoStatic =
{
	.name       = "aria2",
	.size       = sizeof(UgetPluginAria2),
	.init       = (UgInitFunc)   plugin_init,
	.final      = (UgFinalFunc)  plugin_final,
	.accept     = (UgetPluginSyncFunc) plugin_accept,
	.sync       = (UgetPluginSyncFunc) plugin_sync,
	.ctrl       = (UgetPluginCtrlFunc) plugin_ctrl,
	.hosts      = NULL,
	.schemes    = schemes,
	.file_exts  = types,
	.global_set = (UgetPluginGlobalFunc) global_set,
	.global_get = (UgetPluginGlobalFunc) global_get,
};
// extern
const UgetPluginInfo* UgetPluginAria2Info = &UgetPluginAria2InfoStatic;

// ----------------------------------------------------------------------------
// global data and it's functions.

static struct
{
	UgetAria2* data;
	int        ref_count;
} global = {NULL, 0};

static UgetResult  global_init(void)
{
	if (global.data == NULL) {
		global.data = uget_aria2_new();
		uget_aria2_start_thread(global.data);
	}
	global.ref_count++;
	return UGET_RESULT_OK;
}

static void  global_ref(void)
{
	global.ref_count++;
}

static void  global_unref(void)
{
	if (global.data == NULL)
		return;

	global.ref_count--;
	if (global.ref_count == 0) {
		if (global.data->shutdown)
			uget_aria2_shutdown(global.data);
		uget_aria2_stop_thread(global.data);
		uget_aria2_unref(global.data);
		global.data = NULL;
	}
}

static UgetResult  global_set(int option, void* parameter)
{
	UgetPluginAria2Setting*  setting;

	switch (option) {
	case UGET_PLUGIN_GLOBAL_INIT:
		// do global initialize/uninitialize here
		if (parameter)
			return global_init();
		else
			global_unref();
		break;

	case UGET_PLUGIN_GLOBAL_SPEED_LIMIT:
		if (global.data) {
			uget_aria2_set_speed(global.data,
					((int*)parameter)[0], ((int*)parameter)[1]);
		}
		break;

	case UGET_PLUGIN_ARIA2_GLOBAL_URI:
		if (parameter)
			uget_aria2_set_uri(global.data, (char*) parameter);
		break;

	case UGET_PLUGIN_ARIA2_GLOBAL_PATH:
		uget_aria2_set_path(global.data, (char*) parameter);
		break;

	case UGET_PLUGIN_ARIA2_GLOBAL_ARGUMENT:
		uget_aria2_set_args(global.data, (char*) parameter);
		break;

	case UGET_PLUGIN_ARIA2_GLOBAL_TOKEN:
		uget_aria2_set_token(global.data, (char*) parameter);
		break;

	case UGET_PLUGIN_ARIA2_GLOBAL_LAUNCH:
		if (parameter != NULL)
			if (uget_aria2_launch(global.data) == FALSE)
				return UGET_RESULT_ERROR;
		break;

	case UGET_PLUGIN_ARIA2_GLOBAL_SHUTDOWN:
		if (parameter)
			global.data->shutdown = TRUE;
		else
			global.data->shutdown = FALSE;
		break;

	case UGET_PLUGIN_ARIA2_GLOBAL_SHUTDOWN_NOW:
		if (parameter && global.data)
			uget_aria2_shutdown(global.data);
		break;

	case UGET_PLUGIN_GLOBAL_SETTING:
		setting = parameter;
		global.data->polling_interval = setting->polling_interval;
		global.data->shutdown = setting->shutdown;
		uget_aria2_set_uri (global.data, setting->uri);
		uget_aria2_set_path(global.data, setting->path);
		uget_aria2_set_args(global.data, setting->arguments);
		if (setting->launch)
			uget_aria2_launch(global.data);
		break;

	default:
		return UGET_RESULT_UNSUPPORT;
	}

	return UGET_RESULT_OK;
}

static UgetResult  global_get(int option, void* parameter)
{
	switch (option) {
	case UGET_PLUGIN_GLOBAL_INIT:
		if (parameter)
			*(int*)parameter = global.data ? TRUE : FALSE;
		break;

	case UGET_PLUGIN_GLOBAL_ERROR_CODE:
		if (parameter)
			*(int*)parameter = global.data->error;
		break;

	case UGET_PLUGIN_ARIA2_GLOBAL_LAUNCH:
		if (parameter)
			*(int*)parameter = global.data->launched;
		break;

	default:
		return UGET_RESULT_UNSUPPORT;
	}

	return UGET_RESULT_OK;
}

// ----------------------------------------------------------------------------
// control functions

static const char* error_string[] =
{
	NULL,
	// 1 - 10
	N_("aria2: an unknown error occurred."),
	N_("aria2: time out occurred."),
	N_("aria2: resource was not found."),
	N_("aria2 saw the specified number of 'resource not found' error. See --max-file-not-found option"),
	N_("aria2: speed was too slow."),
	N_("aria2: network problem occurred."),
	N_("aria2: unfinished downloads."),
	N_("Not Resumable"),    // _("Not Resumable"),
	N_("Out of resource"),  // _(),
	N_("aria2: piece length was different from one in .aria2 control file."),
	// 11 - 20
	N_("aria2 was downloading same file."),
	N_("aria2 was downloading same info hash torrent."),
	N_("aria2: file already existed. See --allow-overwrite option."),
	N_("Output file can't be renamed."),  // _("Output file can't be renamed."),
	N_("aria2: could not open existing file."),
	N_("aria2: could not create new file or truncate existing file."),
	N_("aria2: file I/O error occurred."),
	N_("Folder can't be created."),    // UGET_EVENT_ERROR_FOLDER_CREATE_FAILED
	N_("aria2: name resolution failed."),
	N_("aria2: could not parse Metalink document."),
	// 21 - 30
	N_("aria2: FTP command failed."),
	N_("aria2: HTTP response header was bad or unexpected."),
	N_("Too many redirections."),
	N_("aria2: HTTP authorization failed."),
	N_("aria2: could not parse bencoded file(usually .torrent file)."),
	N_("aria2: torrent file was corrupted or missing information."),
	N_("aria2: Magnet URI was bad."),
	N_("aria2: bad/unrecognized option was given or unexpected option argument was given."),
	N_("aria2: remote server was unable to handle the request."),
	N_("aria2: could not parse JSON-RPC request."),
};

static const char* aria2_no_response = N_("No response. Is aria2 shutdown?");

static void  plugin_init(UgetPluginAria2* plugin)
{
	if (global.data == NULL)
		global_init();
	else
		global_ref();

	ug_array_init(&plugin->gids, sizeof(char*), 16);
	plugin->stopped = TRUE;
	plugin->paused = TRUE;
	plugin->synced = TRUE;
}

static void  plugin_final(UgetPluginAria2* plugin)
{
	ug_array_foreach_str(&plugin->gids, (UgForeachFunc) ug_free, NULL);
	ug_array_clear(&plugin->gids);
	// clear UgetFiles
	if (plugin->files)
		ug_data_free(plugin->files);
	// clear and recycle start_request object
	if (plugin->start_request) {
		ug_value_foreach(&plugin->start_request->params, ug_value_set_name, NULL);
		uget_aria2_recycle(global.data, plugin->start_request);
	}

	global_unref();
}

// ----------------------------------------------------------------------------
// plugin_ctrl

static int  plugin_ctrl_speed(UgetPluginAria2* plugin, int* speed);
static int  plugin_start(UgetPluginAria2* plugin);

static int  plugin_ctrl(UgetPluginAria2* plugin, int code, void* data)
{
	switch (code) {
	case UGET_PLUGIN_CTRL_START:
		if (plugin->start_request)
			return plugin_start(plugin);
		break;

	case UGET_PLUGIN_CTRL_STOP:
		plugin->paused = TRUE;
		return TRUE;

	case UGET_PLUGIN_CTRL_SPEED:
		// speed control
		return plugin_ctrl_speed(plugin, data);

	// state ----------------
	case UGET_PLUGIN_GET_STATE:
		*(int*)data = (plugin->stopped) ? FALSE : TRUE;
		return TRUE;

	default:
		break;
	}

	return FALSE;
}

static int  plugin_ctrl_speed(UgetPluginAria2* plugin, int* speed)
{
	int   value;

	// notify plug-in that speed limit has been changed
	if (plugin->limit[0] != speed[0] || plugin->limit[1] != speed[1])
		plugin->limit_changed = TRUE;
	// decide speed limit by user specified data.
	value = speed[0];
	if (plugin->limit_upper[0] > 0) {
		if (value > plugin->limit_upper[0] || value == 0) {
			value = plugin->limit_upper[0];
			plugin->limit_changed = TRUE;
		}
	}
	plugin->limit[0] = value;

	value = speed[1];
	if (plugin->limit_upper[1] > 0) {
		if (value > plugin->limit_upper[1] || value == 0) {
			value = plugin->limit_upper[1];
			plugin->limit_changed = TRUE;
		}
	}
	plugin->limit[1] = value;

	return plugin->limit_changed;
}

// ----------------------------------------------------------------------------
// plugin_sync

// return FALSE if plug-in was stopped.
static int  plugin_sync(UgetPluginAria2* plugin, UgInfo* node_info)
{
	int          index;
	UgetEvent*   event;
	UgetFiles*   files;
	UgetFile*    file1;
	struct {
		UgetCommon*      common;
		UgetProgress*    progress;
	} temp;

	if (plugin->stopped) {
		if (plugin->synced)
			return FALSE;
		plugin->synced = TRUE;
	}
	else if (plugin->synced == TRUE)    // maybe thread is accessing plugin->gids
		return TRUE;
	// avoid crash if plug-in failed to start.
	if (plugin->start_request == NULL)
		return FALSE;
	// sync data between plug-in and foreign UgData
	// ------------------------------------------------
	// update progress
	temp.progress = ug_info_realloc(node_info, UgetProgressInfo);
	temp.progress->complete       = plugin->completedLength;
	temp.progress->total          = plugin->totalLength;
	temp.progress->download_speed = plugin->downloadSpeed;
	temp.progress->upload_speed   = plugin->uploadSpeed;
	temp.progress->uploaded       = plugin->uploadLength;
	temp.progress->elapsed        = time(NULL) - plugin->start_time;
	// ratio
	if (temp.progress->uploaded && temp.progress->complete)
		temp.progress->ratio = (double)temp.progress->uploaded / (double)temp.progress->complete;
	else
		temp.progress->ratio = 0.0;
	// If total size is unknown, don't calculate percent.
	if (temp.progress->total)
		temp.progress->percent = (int) (temp.progress->complete * 100 / temp.progress->total);
	else
		temp.progress->percent = 0;
	// If total size and average speed is unknown, don't calculate remain time.
	if (temp.progress->download_speed > 0 && temp.progress->total > 0)
		temp.progress->left = (temp.progress->total - temp.progress->complete) / temp.progress->download_speed;

	temp.common = ug_info_realloc(node_info, UgetCommonInfo);
	// ------------------------------------------------
	// sync changed limit from foreign UgInfo
	if (plugin->limit_upper[1] != temp.common->max_upload_speed ||
		plugin->limit_upper[0] != temp.common->max_download_speed)
	{
		// speed control
		plugin->limit_upper[1] = temp.common->max_upload_speed;
		plugin->limit_upper[0] = temp.common->max_download_speed;
		plugin_ctrl_speed(plugin, plugin->limit_upper);
	}

	// update UgetFiles
	files = ug_info_realloc(node_info, UgetFilesInfo);
	uget_plugin_lock(plugin);
	uget_files_sync(files, plugin->files);
	uget_plugin_unlock(plugin);

	// change name.
	if (files->list.size > 0 &&
	    plugin->named == FALSE && plugin->files_per_gid > 0)
	{
		plugin->named  = TRUE;
		if (plugin->uri_type == URI_NET && temp.common->file == NULL) {
			uget_plugin_lock(plugin);
			file1 = (UgetFile*) plugin->files->list.head;
			ug_uri_init(&plugin->uri_part, file1->path);
			uget_plugin_unlock(plugin);
			index = plugin->uri_part.file;
			if (index != -1) {
				ug_free(temp.common->name);
				temp.common->name = ug_uri_get_file(&plugin->uri_part);
				event = uget_event_new(UGET_EVENT_NAME);
				uget_plugin_post((UgetPlugin*) plugin, event);
#ifndef NDEBUG
				// debug
				if (temp.common->debug_level)
					printf("base name = %s\n", temp.common->name);
#endif
			}
		}
	}

	switch (plugin->status) {
	case ARIA2_STATUS_ACTIVE:
		if (plugin->completedLength > 0 &&
		    plugin->completedLength == plugin->totalLength)
		{
			event = uget_event_new(UGET_EVENT_UPLOADING);
			uget_plugin_post((UgetPlugin*) plugin, event);
		}
		break;

	case ARIA2_STATUS_WAITING:
		// clear uploading state
		event = uget_event_new(UGET_EVENT_STOP_UPLOADING);
		uget_plugin_post((UgetPlugin*) plugin, event);
		break;

	case ARIA2_STATUS_COMPLETE:
		// clear uploading state
		event = uget_event_new(UGET_EVENT_STOP_UPLOADING);
		uget_plugin_post((UgetPlugin*) plugin, event);
		// remove completed gid
		if (plugin->gids.length > 0) {
			ug_free(plugin->gids.at[0]);
			ug_array_erase(&plugin->gids, 0, 1);
		}
		// If there is only one followed gid and file, change uri.
		if (plugin->gids.length == 1 && plugin->files_per_gid == 1) {
			// If URI scheme is not "magnet" and aria2 runs in local device
			if (global.data->uri_remote == FALSE &&
				plugin->uri_type != URI_MAGNET)
			{
				// change URI
				ug_free(temp.common->uri);
				ug_free(temp.common->name);
				ug_free(temp.common->file);
				file1 = (UgetFile*) plugin->files->list.head;
				temp.common->uri  = ug_strdup(file1->path);
				temp.common->name = uget_name_from_uri_str(temp.common->uri);
				temp.common->file = NULL;
#ifndef NDEBUG
				// debug
				if (temp.common->debug_level)
					printf("uri followed to %s\n", temp.common->uri);
#endif
			}
		}
		// If no followed gid, it was completed.
		else if (plugin->gids.length == 0) {
			event = uget_event_new(UGET_EVENT_COMPLETED);
			uget_plugin_post((UgetPlugin*)plugin, event);
		}
		// clear files
		uget_plugin_lock(plugin);
		uget_files_clear(plugin->files);
		uget_plugin_unlock(plugin);
		plugin->files_per_gid = 0;
		break;

	case ARIA2_STATUS_ERROR:
		// clear uploading state
		event = uget_event_new(UGET_EVENT_STOP_UPLOADING);
		uget_plugin_post((UgetPlugin*) plugin, event);
#ifdef NO_RETRY_IF_CONNECT_FAILED
		// download speed was too slow
		if (plugin->errorCode == 5) {
#else
		// download speed was too slow  or  name resolution failed
		if (plugin->errorCode == 5 || plugin->errorCode == 19) {
#endif
			// retry
			if (temp.common->retry_count < temp.common->retry_limit ||
			    temp.common->retry_limit == 0)
			{
				temp.common->retry_count++;
				plugin->restart = TRUE;
#ifndef NDEBUG
				// debug
				if (temp.common->debug_level)
					printf("retry %d\n", temp.common->retry_count);
#endif
			}
			else {
				event = uget_event_new_error(
						UGET_EVENT_ERROR_TOO_MANY_RETRIES, NULL);
				uget_plugin_post((UgetPlugin*) plugin, event);
			}
		}
		else {
			if (plugin->errorCode > 30)
				plugin->errorCode = 1;
			// if this is last gid.
			if (plugin->gids.length == 1) {
#ifdef HAVE_GLIB
				event = uget_event_new_error(0,
						gettext(error_string[plugin->errorCode]));
#else
				event = uget_event_new_error(0,
						error_string[plugin->errorCode]);
#endif
				uget_plugin_post((UgetPlugin*)plugin, event);
			}
		}

		// remove stopped gid
		ug_free(plugin->gids.at[0]);
		plugin->gids.length -= 1;
		memmove(plugin->gids.at, plugin->gids.at + 1,
		        sizeof(char*) *  plugin->gids.length);
		break;

	case ARIA2_STATUS_REMOVED:
		// clear uploading state
		event = uget_event_new(UGET_EVENT_STOP_UPLOADING);
		uget_plugin_post((UgetPlugin*) plugin, event);
		// debug
		event = uget_event_new_normal(0, _("aria2: gid was removed."));
		uget_plugin_post((UgetPlugin*)plugin, event);
		// remove completed gid
		ug_free(plugin->gids.at[0]);
		plugin->gids.length -= 1;
		memmove(plugin->gids.at, plugin->gids.at + 1,
		        sizeof(char*) *  plugin->gids.length);
		break;
	}

	// If we have followed gid, don't restart.
	if (plugin->gids.length > 0)
		plugin->restart = FALSE;
	else {
#ifndef NDEBUG
		// debug
		if (temp.common->debug_level)
			printf("gids.length = %d\n", plugin->gids.length);
#endif
		// If no followed gid and no need to retry, it must stop.
		if (plugin->restart == FALSE)
			plugin->paused = TRUE;
	}

	// if plug-in was stopped, don't sync data with thread
	if (plugin->stopped == FALSE)
		plugin->synced = TRUE;
	return TRUE;
}

// ----------------------------------------------------------------------------
// plugin_thread

static void  add_gids_by_value_array(UgArrayStr* gids, UgValueArray* varray)
{
	UgValue*  value;
	int       index;

#ifndef NDEBUG
	// debug
	printf("add %d gids\n", varray->length);
#endif
	for (index = 0;  index < varray->length;  index++) {
		value = varray->at + index;
		*(char**) ug_array_alloc(gids, 1) = value->c.string;
		value->c.string = NULL;
		value->type = UG_VALUE_NONE;
	}
}

static int  send_start_request(UgetPluginAria2* plugin)
{
	UgJsonrpcObject*  res;

	uget_aria2_request(global.data, plugin->start_request);
	res = uget_aria2_respond(global.data, plugin->start_request);
	if (res == NULL) {
#ifdef HAVE_GLIB
		uget_plugin_post((UgetPlugin*) plugin,
				uget_event_new_error(0, gettext(aria2_no_response)));
#else
		uget_plugin_post((UgetPlugin*) plugin,
				uget_event_new_error(0, aria2_no_response));
#endif
		return FALSE;
	}
	if (res->error.code) {
		uget_plugin_post((UgetPlugin*)plugin,
				uget_event_new_error(0, res->error.message));
		uget_aria2_recycle(global.data, res);
		return FALSE;
	}

	// add gid from response
	if (plugin->uri_type == URI_METALINK)
		add_gids_by_value_array(&plugin->gids, res->result.c.array);
	else {
		*(char**) ug_array_alloc(&plugin->gids, 1) =
				ug_strdup(res->result.c.string);
	}
	// recycle response
	uget_aria2_recycle(global.data, res);
	return TRUE;
}

static int  plugin_delay(UgetPluginAria2* plugin)
{
	unsigned int  count;

	for (count = 0;  count < plugin->retry_delay;  count++) {
		if (plugin->paused)
			return FALSE;
		// sleep 1 second every time
		ug_sleep(1000);
	}
	return TRUE;
}

static UgThreadResult  plugin_thread(UgetPluginAria2* plugin)
{
	UgJsonrpcObject*  req;
	UgJsonrpcObject*  res;
	UgJsonrpcObject*  status_req;
	UgValue*          status_gid;
	UgValue*          value;
	UgValue*          member;
	int               count;

	// create status_req and initialize status_gid
	status_req = alloc_status_request(&status_gid);
	// send start_request to server
	plugin->restart = FALSE;
	if (send_start_request(plugin) == FALSE)
		goto exit;

	while (plugin->paused == FALSE) {
		// retry
		if (plugin->restart == TRUE) {
			if (plugin_delay(plugin) == FALSE)
				continue;
#ifndef NDEBUG
			// debug
			printf("retry\n");
#endif
			// send start_request to server again
			plugin->restart = FALSE;
			if (send_start_request(plugin) == FALSE)
				goto exit;
		}
		// Don't update status until user call plugin_sync()
		if (plugin->synced == FALSE) {
			// sleep 0.5 second
			ug_sleep(500);
			continue;
		}

		// set gid for status request
//		status_req->params.c.array->at[0].c.string = plugin->gids.at[0];
		status_gid->c.string = plugin->gids.at[0];
		// status request
		uget_aria2_request(global.data, status_req);
		// speed control : speed request & response
		if (plugin->limit_changed) {
			plugin->limit_changed = FALSE;
			// request & response
			req = alloc_speed_request(plugin);
			uget_aria2_request(global.data, req);
			res = uget_aria2_respond(global.data, req);
			// recycle
			uget_aria2_recycle(global.data, res);
			recycle_speed_request(req);
		}
		// status respond
		res = uget_aria2_respond(global.data, status_req);
		if (res == NULL) {
#ifdef HAVE_GLIB
			uget_plugin_post((UgetPlugin*) plugin,
					uget_event_new_error(0, gettext(aria2_no_response)));
#else
			uget_plugin_post((UgetPlugin*) plugin,
					uget_event_new_error(0, aria2_no_response));
#endif
			goto exit;
		}
		if (res->error.code) {
			uget_plugin_post((UgetPlugin*)plugin,
					uget_event_new_error(0, res->error.message));
			uget_aria2_recycle(global.data, res);
			goto exit;
		}

		// parse status response --- start ---
		ug_value_sort_name(&res->result);
		value = ug_value_find_name(&res->result, "status");
		switch (value->c.string[0]) {
		case 'a':
			plugin->status = ARIA2_STATUS_ACTIVE;
			break;
		case 'w':
			plugin->status = ARIA2_STATUS_WAITING;
			break;
		case 'p':
			plugin->status = ARIA2_STATUS_PAUSED;
			break;
		case 'e':
			plugin->status = ARIA2_STATUS_ERROR;
			break;
		case 'c':
			plugin->status = ARIA2_STATUS_COMPLETE;
			break;
		case 'r':
			plugin->status = ARIA2_STATUS_REMOVED;
			break;
		default:
			plugin->status = ARIA2_N_STATUS;
			break;
		}
		value = ug_value_find_name(&res->result, "errorCode");
		plugin->errorCode = (value) ? ug_value_get_int(value) : 0;
		value = ug_value_find_name(&res->result, "totalLength");
		plugin->totalLength = ug_value_get_int64(value);
		value = ug_value_find_name(&res->result, "completedLength");
		plugin->completedLength = ug_value_get_int64(value);
		value = ug_value_find_name(&res->result, "uploadLength");
		plugin->uploadLength = ug_value_get_int64(value);
		value = ug_value_find_name(&res->result, "downloadSpeed");
		plugin->downloadSpeed = ug_value_get_int(value);
		value = ug_value_find_name(&res->result, "uploadSpeed");
		plugin->uploadSpeed = ug_value_get_int(value);
		value = ug_value_find_name(&res->result, "followedBy");
		if (value)
			add_gids_by_value_array(&plugin->gids, value->c.array);
		value = ug_value_find_name(&res->result, "files");
		if (value && ug_value_length(value) != plugin->files_per_gid) {
			UgValueArray*  array;
			UgetFile*      ufile;
			char*          string;

			array = value->c.array;
			plugin->files_per_gid = ug_value_length(value);
			for (count = 0;  count < array->length;  count++) {
				value = array->at + count;
				ug_value_sort_name(value);
				member = ug_value_find_name(value, "path");
				if (member == NULL || member->c.string[0] == '\0') {
					plugin->files_per_gid--;
					continue;
				}
				uget_plugin_lock(plugin);
				// add .aria2 control file first
				if (plugin->files_per_gid == 1) {
					string = ug_strdup_printf("%s.aria2", member->c.string);
					ufile = uget_files_realloc(plugin->files, string);
					ufile->type = UGET_FILE_TEMPORARY;
					ug_free(string);
				}
				// add downloading file
				ufile = uget_files_realloc(plugin->files, member->c.string);
				member = ug_value_find_name(value, "completedLength");
				ufile->complete = ug_value_get_int64(member);
				member = ug_value_find_name(value, "length");
				ufile->total = ug_value_get_int64(member);
				uget_plugin_unlock(plugin);
			}
		}
		// parse status response --- end ---

		// recycle status response
		uget_aria2_recycle(global.data, res);
		// plugin_sync() will exchange data
		plugin->synced = FALSE;
	}

	if (plugin->gids.length) {
		req = uget_aria2_alloc(global.data, TRUE, TRUE);
		req->method_static = "aria2.remove";
		// if there is no secret token in params.
		if (req->params.type == UG_VALUE_NONE)
			ug_value_init_array(&req->params, plugin->gids.length);
		// add gids to params.
		value = ug_value_alloc(&req->params, plugin->gids.length);
		for (count = 0;  count < plugin->gids.length;  count++, value++) {
			value->type = UG_VALUE_STRING;
			value->c.string = ug_strdup(plugin->gids.at[count]);
		}
		// call "aria2.remove"
		uget_aria2_request(global.data, req);
		res = uget_aria2_respond(global.data, req);
#ifndef NDEBUG
		// debug
		if (res->error.code) {
			printf("aria2.remove() response error code = %d" "\n"
			       "               message = \"%s\"." "\n",
			       res->error.code, res->error.message);
		}
#endif
		uget_aria2_recycle(global.data, res);
		uget_aria2_recycle(global.data, req);
	}

exit:
	recycle_status_request(status_req);
	plugin->stopped = TRUE;
	uget_plugin_unref((UgetPlugin*)plugin);
	return UG_THREAD_RESULT;
}

// ----------------------------------------------------------------------------
// plugin_accept/plugin_start

static int  plugin_accept(UgetPluginAria2* plugin, UgInfo* node_info)
{
	UgJsonrpcObject*  request;
	UgValue*  value;
	UgValue*  member;
	char*     uri;
	char*     str      = NULL;
	char*     user     = NULL;
	char*     password = NULL;
	union {
		UgetCommon*  common;
		UgetFiles*   files;
		UgetProxy*   proxy;
		UgetHttp*    http;
		UgetHttp*    ftp;
	} temp;

	temp.common = ug_info_get(node_info, UgetCommonInfo);
	if (temp.common == NULL || temp.common->uri == NULL)
		return FALSE;

	uri = temp.common->uri;
	plugin->uri_type = URI_NET;

	ug_uri_init(&plugin->uri_part, uri);
	if ((plugin->uri_part.scheme_len == 4 && strncmp(uri, "file", 4) == 0) ||
	    (plugin->uri_part.scheme_len == 0 && plugin->uri_part.file   >= 0))
	{
		// file type is torrent or metalink?
		if (decide_file_type(plugin) == URI_UNSUPPORTED) {
			uget_plugin_post((UgetPlugin*) plugin,
					uget_event_new_error(UGET_EVENT_ERROR_UNSUPPORTED_FILE,
					                      NULL));
			return FALSE;
		}
		// load file and convert it's binary to base64
		if (plugin->uri_part.path > 0)
			str = ug_file_to_base64(uri + plugin->uri_part.path + 1, NULL);
		else
			str = ug_file_to_base64(uri, NULL);
		if (str == NULL) {
			uget_plugin_post((UgetPlugin*) plugin,
					uget_event_new_error(UGET_EVENT_ERROR_FILE_OPEN_FAILED,
					                      NULL));
			return FALSE;
		}
	}
	else if (plugin->uri_part.scheme_len == 6 && strncmp(uri, "magnet", 6) == 0)
		plugin->uri_type = URI_MAGNET;

	request = uget_aria2_alloc(global.data, TRUE, TRUE);
	if (request->params.type == UG_VALUE_NONE)
		ug_value_init_array(&request->params, 3);

	switch (plugin->uri_type) {
	case URI_NET:
	case URI_MAGNET:
		request->method_static = "aria2.addUri";
		// parameter1 : URIs
		value = ug_value_alloc(&request->params, 1);
		ug_value_init_array(value, 8);
		member = ug_value_alloc(value, 1);
		member->type = UG_VALUE_STRING;
		member->c.string = ug_strdup(uri);
		// mirrors
		add_uri_mirrors(value, temp.common->mirrors);
		// parameter2 : options
		break;

	case URI_TORRENT:
		request->method_static = "aria2.addTorrent";
		// parameter1 : encoded torrent file
		value = ug_value_alloc(&request->params, 1);
		value->type = UG_VALUE_STRING;
		value->c.string = str;
		// parameter2 : URIs
		value = ug_value_alloc(&request->params, 1);
		ug_value_init_array(value, 0);
		// parameter3 : options
		break;

	case URI_METALINK:
		// parameter1 : encoded metalink file
		request->method_static = "aria2.addMetalink";
		value = ug_value_alloc(&request->params, 1);
		value->type = UG_VALUE_STRING;
		value->c.string = str;
		// parameter2 : options
		break;
	}

	// parameterX : options
	value = ug_value_alloc(&request->params, 1);
	ug_value_init_object(value, 16);
	member = ug_value_alloc(value, 1);
	member->name = "continue";
	member->type = UG_VALUE_BOOL;
	member->c.boolean = TRUE;

	if ((temp.common->user     && temp.common->user[0]) ||
	    (temp.common->password && temp.common->password[0]))
	{
		user     = (temp.common->user)     ? temp.common->user : "";
		password = (temp.common->password) ? temp.common->password : "";
	}

	if (temp.common->folder) {
		member = ug_value_alloc(value, 1);
		member->name = "dir";
		member->type = UG_VALUE_STRING;
		member->c.string = ug_strdup(temp.common->folder);
	}

	if (temp.common->file) {
		member = ug_value_alloc(value, 1);
		member->name = "out";
		member->type = UG_VALUE_STRING;
		member->c.string = ug_strdup(temp.common->file);
	}

	member = ug_value_alloc(value, 1);
	member->name = "remote-time";
	member->type = UG_VALUE_BOOL;
	member->c.boolean = temp.common->timestamp;

	member = ug_value_alloc(value, 1);
	member->name = "retry-wait";
	member->type = UG_VALUE_STRING;
	member->c.string = ug_strdup_printf("%u", temp.common->retry_delay);

	member = ug_value_alloc(value, 1);
	member->name = "max-tries";
	member->type = UG_VALUE_STRING;
	member->c.string = ug_strdup_printf("%u", temp.common->retry_limit);

	// speed control: decide speed limit before starting plug-in
	plugin->limit_upper[1] = temp.common->max_upload_speed;
	plugin->limit_upper[0] = temp.common->max_download_speed;
	plugin_ctrl_speed(plugin, plugin->limit);

	member = ug_value_alloc(value, 1);
	member->name = "max-download-limit";
	member->type = UG_VALUE_STRING;
//	member->c.string = ug_strdup_printf("%d", temp.common->max_download_speed);
	member->c.string = ug_strdup_printf("%d", plugin->limit[0]);

	member = ug_value_alloc(value, 1);
	member->name = "max-upload-limit";
	member->type = UG_VALUE_STRING;
//	member->c.string = ug_strdup_printf("%d", temp.common->max_upload_speed);
	member->c.string = ug_strdup_printf("%d", plugin->limit[1]);

	member = ug_value_alloc(value, 1);
	member->name = "lowest-speed-limit";
	member->type = UG_VALUE_STRING;
	member->c.string = ug_strdup_printf("%u", 128);

	// Don't set connection limit if max_connections is 0.
	if (temp.common->max_connections != 0) {
		// aria2 doesn't accept "max-connection-per-server" large than 16.
		member = ug_value_alloc(value, 1);
		member->name = "max-connection-per-server";
		member->type = UG_VALUE_STRING;
		member->c.string = ug_strdup_printf("%u",
				(temp.common->max_connections <= 16) ?
				 temp.common->max_connections : 16);
		// split
		member = ug_value_alloc(value, 1);
		member->name = "split";
		member->type = UG_VALUE_STRING;
		member->c.string = ug_strdup_printf("%u",
				temp.common->max_connections);
	}

	temp.files = ug_info_get(node_info, UgetFilesInfo);
	if (temp.files)
		plugin->files = ug_data_copy(temp.files);
	else
		plugin->files = ug_data_new(UgetFilesInfo);

	temp.proxy = ug_info_get(node_info, UgetProxyInfo);
#ifdef HAVE_LIBPWMD
	if (temp.proxy && temp.proxy->type == UGET_PROXY_PWMD) {
		if (uget_plugin_aria2_set_proxy_pwmd(plugin, node_info, member) == FALSE)
			return FALSE;
	}
	else
#endif
	if (temp.proxy && temp.proxy->host) {
		member = ug_value_alloc(value, 1);
		member->name = "all-proxy";
		member->type = UG_VALUE_STRING;
		if (temp.proxy->port == 0)
			member->c.string = ug_strdup(temp.proxy->host);
		else {
			member->c.string = ug_strdup_printf("%s:%d",
					temp.proxy->host, temp.proxy->port);
		}
		if ((temp.proxy->user     && temp.proxy->user[0]) ||
		    (temp.proxy->password && temp.proxy->password[0]))
		{
			member = ug_value_alloc(value, 1);
			member->name = "all-proxy-user";
			member->type = UG_VALUE_STRING;
			member->c.string = ug_strdup(
					(temp.proxy->user) ? temp.proxy->user : "");

			member = ug_value_alloc(value, 1);
			member->name = "all-proxy-password";
			member->type = UG_VALUE_STRING;
			member->c.string = ug_strdup(
					(temp.proxy->password) ? temp.proxy->password : "");
		}
	}

	temp.http = ug_info_get(node_info, UgetHttpInfo);
	if (temp.http) {
		if (plugin->uri_part.scheme_len >= 4 &&
		    strncmp(uri, "http", 4) == 0)
		{
			if ((temp.http->user     && temp.http->user[0]) ||
			    (temp.http->password && temp.http->password[0]))
			{
				user     = (temp.http->user)     ? temp.http->user : "";
				password = (temp.http->password) ? temp.http->password : "";
			}
		}
		if (temp.http->referrer) {
			member = ug_value_alloc(value, 1);
			member->name = "referer";
			member->type = UG_VALUE_STRING;
			member->c.string = ug_strdup(temp.http->referrer);
		}
//		if (temp.http->cookie_file) {
//			member = ug_value_alloc(value, 1);
//			member->name = "load-cookies";
//			member->type = UG_VALUE_STRING;
//			member->c.string = ug_strdup(temp.http->cookie_file);
//		}
		if (temp.http->user_agent) {
			member = ug_value_alloc(value, 1);
			member->name = "user-agent";
			member->type = UG_VALUE_STRING;
			member->c.string = ug_strdup(temp.http->user_agent);
		}
	}

	temp.ftp = ug_info_get(node_info, UgetFtpInfo);
	if (temp.ftp) {
		if (plugin->uri_part.scheme_len >= 3 && strncmp(uri, "ftp", 3) == 0) {
			if ((temp.ftp->user     && temp.ftp->user[0]) ||
			    (temp.ftp->password && temp.ftp->password[0]))
			{
				user     = (temp.ftp->user)     ? temp.ftp->user : "";
				password = (temp.ftp->password) ? temp.ftp->password : "";
			}
		}
	}

	if (plugin->uri_type == URI_NET && plugin->uri_part.host != -1) {
		if (user || password) {
			str = ug_malloc(strlen(user) + strlen(password) +
					strlen(uri) + 2 + 1);  // + ':' + '@' + '\0'
			str[plugin->uri_part.host] = 0;
			strncpy(str, uri, plugin->uri_part.host);
			strcat(str, user);
			strcat(str, ":");
			strcat(str, password);
			strcat(str, "@");
			strcat(str, uri + plugin->uri_part.host);
			// reset uri for aria2.addUri, request->params[0][0]
			value = request->params.c.array->at;
			value = value->c.array->at;
			ug_free(value->c.string);
			value->c.string = ug_strdup(str);
		}
	}

	plugin->files_per_gid = 0;
	plugin->synced = TRUE;
	plugin->start_time = time(NULL);
	plugin->start_request = request;

	return TRUE;
}

static int  plugin_start(UgetPluginAria2* plugin)
{
	UgThread  thread;
	int       ok;

	// try to start thread
	plugin->paused = FALSE;
	plugin->stopped = FALSE;
	uget_plugin_ref((UgetPlugin*) plugin);
	ok = ug_thread_create(&thread, (UgThreadFunc) plugin_thread, plugin);
	if (ok == UG_THREAD_OK)
		ug_thread_unjoin(&thread);
	else {
		// failed to start thread -----------------
		plugin->paused = TRUE;
		plugin->stopped = TRUE;
		// post error message and decreases the reference count
		uget_plugin_post((UgetPlugin*) plugin,
				uget_event_new_error(UGET_EVENT_ERROR_THREAD_CREATE_FAILED,
				                     NULL));
		uget_plugin_unref((UgetPlugin*) plugin);
		return FALSE;
	}
	return TRUE;
}

// ----------------------------------------------------------------------------
// JSON-RPC request

// speed control
static UgJsonrpcObject*  alloc_speed_request(UgetPluginAria2* plugin)
{
	UgJsonrpcObject*  object;
	UgValue*          options;
	UgValue*          value;

	object = uget_aria2_alloc(global.data, TRUE, TRUE);
	object->method_static = "aria2.changeOption";
	if (object->params.type == UG_VALUE_NONE)
		ug_value_init_array(&object->params, 2);
	// gid
	value = ug_value_alloc(&object->params, 1);
	value->type = UG_VALUE_STRING;
	value->c.string = plugin->gids.at[0];
	// object
	options = ug_value_alloc(&object->params, 1);
	ug_value_init_object(options, 2);
	// max-download-limit
	value = ug_value_alloc(options, 1);
	value->name = ug_strdup("max-download-limit");
	value->type = UG_VALUE_STRING;
	value->c.string = ug_strdup_printf("%d", plugin->limit[0]);
	// max-upload-limit
	value = ug_value_alloc(options, 1);
	value->name = ug_strdup("max-upload-limit");
	value->type = UG_VALUE_STRING;
	value->c.string = ug_strdup_printf("%d", plugin->limit[1]);

	return object;
}

static void  recycle_speed_request(UgJsonrpcObject* object)
{
	UgValue*  value;

	value = uget_aria2_clear_token(object);
	// params[0] is gid
//	value = object->params.c.array->at;
	value->type = UG_VALUE_NONE;
	value->c.array = NULL;
	// ready to recycle it
	uget_aria2_recycle(global.data, object);
}

static UgJsonrpcObject*  alloc_status_request(UgValue** gid)
{
	UgJsonrpcObject*  object;
	UgValue*  params;
	UgValue*  keys;

	// prepare JSON-RPC object for "aria2.tellStatus"
	object = uget_aria2_alloc(global.data, TRUE, TRUE);
	object->method_static = "aria2.tellStatus";
	params = &object->params;
	if (params->type == UG_VALUE_NONE)
		ug_value_init_array(params, 2);
	// gid
	gid[0] = ug_value_alloc(params, 1);
	gid[0]->type = UG_VALUE_STRING;
	gid[0]->c.string = NULL;
	// keys array from UgetAria2.status_keys
	keys = ug_value_alloc(params, 1);
	keys->type = UG_VALUE_ARRAY;
	keys->c.array = global.data->status_keys.c.array;

	return  object;
}

static void  recycle_status_request(UgJsonrpcObject* object)
{
	UgValue*  value;

	value = uget_aria2_clear_token(object);
	// params[0] is gid
//	value = object->params.c.array->at;
	value->type = UG_VALUE_NONE;
	value->c.array = NULL;
	// params[1] is keys of status
//	value = object->params.c.array->at + 1;
	value++;
	value->type = UG_VALUE_NONE;
	value->c.array = NULL;
	// ready to recycle it
	uget_aria2_recycle(global.data, object);
}

// ----------------------------------------------------------------------------
// static utility functions

static void*  ug_file_to_base64(const char* file, int* length)
{
	int     fd;
	int     fsize, fpos = 0;
	int     result_len;
	void*   buffer;
	void*   result;

//	fd = open(file, O_RDONLY | O_BINARY, S_IREAD);
	fd = ug_open(file, UG_O_READONLY | UG_O_BINARY, UG_S_IREAD);
	if (fd == -1)
		return NULL;
//	lseek(fd, 0, SEEK_END);
	ug_seek(fd, 0, SEEK_END);
	fsize = (int) ug_tell(fd);
	buffer = ug_malloc(fsize);
//	lseek(fd, 0, SEEK_SET);
	ug_seek(fd, 0, SEEK_SET);

	do {
		result_len = ug_read(fd, buffer, fsize - fpos);
//		result_len = read(fd, buffer, fsize - fpos);
		fpos += result_len;
	} while (result_len > 0);
//	close(fd);
	ug_close(fd);

	if (fsize != fpos) {
		ug_free(buffer);
		return NULL;
	}

	result = ug_base64_encode(buffer, fsize, &result_len);
	ug_free(buffer);
	if (length)
		*length = result_len;
	return result;
}

static int  decide_file_type(UgetPluginAria2* plugin)
{
	char  buf[11];
	union {
		const char* ext;
		int   fd;
		int   path;
	} temp;

	plugin->uri_type = URI_UNSUPPORTED;
	// handle URI file:///
	temp.path = plugin->uri_part.path;
	if (temp.path > 0 && plugin->uri_part.uri[temp.path] != 0)
		temp.path++;
	//
	temp.fd = ug_open(plugin->uri_part.uri + temp.path,
			UG_O_READONLY | UG_O_BINARY, UG_S_IREAD);
	if (temp.fd != -1 && ug_read(temp.fd, buf, 11) == 11) {
		if (strncmp(buf, "d8:announce", 11) == 0)
			plugin->uri_type = URI_TORRENT;
		else {
			buf[10] = 0;
			if (strchr(buf, '<'))
				plugin->uri_type = URI_METALINK;
		}
	}
	ug_close(temp.fd);

	if (plugin->uri_type == URI_UNSUPPORTED &&
	    ug_uri_part_file_ext(&plugin->uri_part, &temp.ext))
	{
		if (temp.ext[0] == 'm' || temp.ext[0] == 'M')
			plugin->uri_type = URI_METALINK;
		else if (temp.ext[0] == 't' || temp.ext[0] == 'T')
			plugin->uri_type = URI_TORRENT;
		else
			plugin->uri_type = URI_UNSUPPORTED;
	}

	return plugin->uri_type;
}

static void  add_uri_mirrors(UgValue* varray, const char* mirrors)
{
	UgValue*    value;
	const char* curr;
	const char* prev;

	for (curr = mirrors;  curr && curr[0];) {
		// skip space ' '
		while (curr[0] == ' ')
			curr++;
		prev = curr;
		curr = curr + strcspn(curr, " ");

		value = ug_value_alloc(varray, 1);
		value->type = UG_VALUE_STRING;
		value->c.string = ug_malloc(curr - prev + 1);
		value->c.string[curr - prev] = 0;    // NULL terminated
		strncpy(value->c.string, prev, curr - prev);
	}
}

// ----------------------------------------------------------------------------
// PWMD
//
#ifdef HAVE_LIBPWMD
static gboolean	uget_plugin_aria2_set_proxy_pwmd(UgetPluginAria2 *plugin, UgInfo* info, UgValue* options)
{
       struct pwmd_proxy_s pwmd;
       gpg_error_t rc;
       UgetEvent *message;
       UgetProxy *proxy;

       memset(&pwmd, 0, sizeof(pwmd));
       proxy = ug_info_get(info, UgetProxyInfo);
       rc = ug_set_pwmd_proxy_options(&pwmd, proxy);

       if (rc)
               goto fail;

       // proxy host and port
	// host
	UgValue *value = ug_value_alloc(options, 1);
	value->name = ug_strdup("all-proxy");
	value->type = UG_VALUE_STRING;
       if (pwmd.port == 0)
               value->c.string = ug_strdup(pwmd.hostname);
	else {
		value->c.string = ug_strdup_printf("%s:%u", pwmd.hostname, pwmd.port);
	}

	// proxy user and password
       if (pwmd.username || pwmd.password) {
		// user
		value = ug_value_alloc(options, 1);
		value->name = ug_strdup("all-proxy-user");
		value->type = UG_VALUE_STRING;
               value->c.string = ug_strdup(pwmd.username ? pwmd.username : "");
		// password
		value = ug_value_alloc(options, 1);
		value->name = ug_strdup("all-proxy-password");
		value->type = UG_VALUE_STRING;
               value->c.string = ug_strdup(pwmd.password ? pwmd.password : "");
	}

       ug_close_pwmd(&pwmd);
       return TRUE;

fail:
       ug_close_pwmd(&pwmd);
       gchar *e = g_strdup_printf("Pwmd ERR %i: %s", rc, gpg_strerror(rc));
       message = uget_event_new_error(UGET_EVENT_ERROR_CUSTOM, e);
       uget_plugin_post((UgetPlugin*) plugin, message);
       g_free(e);
       return FALSE;
}

#endif	// HAVE_LIBPWMD

