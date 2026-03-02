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

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef __ANDROID__
#include <android/api-level.h>
#endif

#include <UgUtil.h>
#include <UgStdio.h>
#include <UgString.h>
#include <UgFileUtil.h>
#include <UgetCurl.h>
#include <UgetPluginCurl.h>

#include <errno.h>

#if defined _WIN32 || defined _WIN64
#include <windows.h>    // Sleep()
#include <winsock2.h>
#define  ug_sleep       Sleep
#else
#include <fcntl.h>   // posix_fallocate()
#include <unistd.h>  // sleep(), usleep()
#define  ug_sleep(millisecond)    usleep(millisecond * 1000)
#endif // _WIN32 || _WIN64

#if defined(_MSC_VER)
#define strtoll		_strtoi64    // stdlib.h
#endif

#define MIN_SPLIT_SIZE       (10 * 1024 * 1024)  // can't less than 16384 x 2
#define MIN_SPEED_LIMIT      256     // speed control
#define MAX_REPEAT_DIGITS    5       //  + '.' + digits
#define MAX_REPEAT_COUNTS    10000   // <= 9999

typedef struct UriLink      UriLink;

struct UriLink {
	UG_LINK_MEMBERS(UriLink, UriLink, self);
//	UriLink* self;
//	UriLink* next;
//	UriLink* prev;

	uint8_t  scheme_type;
	uint8_t  resumable:1;
	uint8_t  tested:1;
	uint8_t  ok:1;
	char     uri[1];
};

// ----------------------------------------------------------------------------
// UgetPluginInfo (derived from UgTypeInfo)

static void plugin_init (UgetPluginCurl* plugin);
static void plugin_final(UgetPluginCurl* plugin);
static int  plugin_ctrl (UgetPluginCurl* plugin, int code, void* data);
static int  plugin_accept(UgetPluginCurl* plugin, UgInfo* node_info);
static int  plugin_sync  (UgetPluginCurl* plugin, UgInfo* node_info);
static UgetResult  global_set(int code, void* parameter);
static UgetResult  global_get(int code, void* parameter);

static const char* schemes[] = {"http", "https", "ftp", "ftps", NULL};

static const UgetPluginInfo UgetPluginCurlInfoStatic =
{
	"curl",
	sizeof(UgetPluginCurl),
	(UgInitFunc)   plugin_init,
	(UgFinalFunc)  plugin_final,
	(UgetPluginSyncFunc) plugin_accept,
	(UgetPluginSyncFunc) plugin_sync,
	(UgetPluginCtrlFunc) plugin_ctrl,
	NULL,
	schemes,
	NULL,
	(UgetPluginGlobalFunc) global_set,
	(UgetPluginGlobalFunc) global_get
};
// extern
const UgetPluginInfo* UgetPluginCurlInfo = &UgetPluginCurlInfoStatic;

// ----------------------------------------------------------------------------
// global data and it's functions.

static struct
{
	int  initialized;
	int  ref_count;
} global = {0, 0};

static UgetResult  global_init(void)
{
	if (global.initialized == FALSE) {
#if defined _WIN32 || defined _WIN64
		WSADATA  WSAData;
		WSAStartup(MAKEWORD(2, 2), &WSAData);
#endif // _WIN32 || _WIN64

		if (curl_global_init(CURL_GLOBAL_ALL) != CURLE_OK) {
#if defined _WIN32 || defined _WIN64
			WSACleanup();
#endif
			return UGET_RESULT_ERROR;
		}
		global.initialized = TRUE;
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
	if (global.initialized == FALSE)
		return;

	global.ref_count--;
	if (global.ref_count == 0) {
		global.initialized  = FALSE;
		curl_global_cleanup();
#if defined _WIN32 || defined _WIN64
		WSACleanup();
#endif
	}
}

static UgetResult  global_set(int option, void* parameter)
{
	switch (option) {
	case UGET_PLUGIN_GLOBAL_INIT:
		// do global initialize/uninitialize here
		if (parameter)
			return global_init();
		else
			global_unref();
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
			*(int*)parameter = global.initialized;
		break;

	default:
		return UGET_RESULT_UNSUPPORT;
	}

	return UGET_RESULT_OK;
}

// ----------------------------------------------------------------------------
// plug-in functions

static void plugin_init(UgetPluginCurl* plugin)
{
	if (global.initialized == FALSE)
		global_init();
	else
		global_ref();

	ug_list_init(&plugin->segment.list);
	plugin->file.time = -1;
	plugin->synced = TRUE;
	plugin->paused = TRUE;
	plugin->stopped = TRUE;
}

static void plugin_final(UgetPluginCurl* plugin)
{
	if (plugin->common)
		ug_data_free(plugin->common);
	if (plugin->files)
		ug_data_free(plugin->files);
	if (plugin->proxy)
		ug_data_free(plugin->proxy);
	if (plugin->http)
		ug_data_free(plugin->http);
	if (plugin->ftp)
		ug_data_free(plugin->ftp);
	// free uri.list (UriLink), all link will be freed.
	ug_list_foreach(&plugin->uri.list, (UgForeachFunc) ug_free, NULL);

//	curl_slist_free_all(plugin->ftp_command);
	ug_free(plugin->folder.path);
	ug_free(plugin->file.name_fmt);
	ug_free(plugin->file.path);
	ug_free(plugin->aria2.path);

	global_unref();
}

// ----------------------------------------------------------------------------
// plugin_ctrl

static int  plugin_ctrl_speed(UgetPluginCurl* plugin, int* speed);
static int  plugin_start(UgetPluginCurl* plugin);

static int  plugin_ctrl(UgetPluginCurl* plugin, int code, void* data)
{
	switch (code) {
	case UGET_PLUGIN_CTRL_START:
		if (plugin->common)
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

static int  plugin_ctrl_speed(UgetPluginCurl* plugin, int* speed)
{
	UgetCommon*  common;
	int          value;

	// notify plug-in that speed limit has been changed
	if (plugin->limit.download != speed[0] || plugin->limit.upload != speed[1])
		plugin->limit_changed = TRUE;
	// decide speed limit by user specified data.
	common = plugin->common;
	if (common == NULL) {
		plugin->limit.download = speed[0];
		plugin->limit.upload   = speed[1];
	}
	else {
		// download
		value = speed[0];
		if (common->max_download_speed) {
			if (value > common->max_download_speed || value == 0) {
				value = common->max_download_speed;
				plugin->limit_changed = TRUE;
			}
		}
		plugin->limit.download = value;
		// upload
		value = speed[1];
		if (common->max_upload_speed) {
			if (value > common->max_upload_speed || value == 0) {
				value = common->max_upload_speed;
				plugin->limit_changed = TRUE;
			}
		}
		plugin->limit.upload = value;
	}
	return plugin->limit_changed;
}

// ----------------------------------------------------------------------------
// plugin_sync

static int  plugin_sync(UgetPluginCurl* plugin, UgInfo* node_info)
{
	UgetCommon*    common;
	UgetFiles*     files;
	UgetProgress*  progress;
	char*          name;
	int            speed[2];

	if (plugin->stopped) {
		if (plugin->synced)
			return FALSE;
		plugin->synced = TRUE;
	}
	// avoid crash if plug-in failed to start.
	if (plugin->common == NULL)
		return FALSE;

	// sync data between plug-in and foreign UgData
	common = ug_info_realloc(node_info, UgetCommonInfo);
	common->retry_count = plugin->common->retry_count;
	// sync changed limit from node_info
	if (plugin->common->max_upload_speed   != common->max_upload_speed ||
		plugin->common->max_download_speed != common->max_download_speed)
	{
		// speed control
		plugin->common->max_upload_speed   = common->max_upload_speed;
		plugin->common->max_download_speed = common->max_download_speed;
		speed[1] = common->max_upload_speed;
		speed[0] = common->max_download_speed;
		plugin_ctrl_speed(plugin, speed);
	}
	plugin->common->max_connections = common->max_connections;
	plugin->common->retry_limit = common->retry_limit;
	if (common->max_connections > 0)
		plugin->segment.n_max = common->max_connections;

	progress = ug_info_realloc(node_info, UgetProgressInfo);
	progress->upload_speed   = plugin->speed.upload;
	progress->download_speed = plugin->speed.download;

	progress->uploaded   = plugin->size.upload;
	progress->complete   = plugin->size.download;

	if (plugin->file.size > 0)
		progress->total = plugin->file.size;
	else
		progress->total = progress->complete;

	if (progress->total > 0)
		progress->percent = (int) (progress->complete * 100 / progress->total);
	else
		progress->percent = 0;

	// If total size and average speed is unknown, don't calculate remain time.
	if (progress->download_speed > 0 && progress->total > 0) {
		progress->left = (progress->total - progress->complete) /
				progress->download_speed;
	}
	// consume time
	progress->elapsed = time(NULL) - plugin->start_time;

	// update UgetFiles
	files = ug_info_realloc(node_info, UgetFilesInfo);
	uget_plugin_lock(plugin);
	uget_files_sync(files, plugin->files);
	uget_plugin_unlock(plugin);
	// set name
	if (plugin->file_renamed && plugin->file.path) {
		plugin->file_renamed = FALSE;
		// change name
#if defined _WIN32 || defined _WIN64
		name = strrchr(plugin->file.path, '\\');
#else
		name = strrchr(plugin->file.path, '/');
#endif
		if (name && name[1]) {
			if (common->name == NULL || strcmp(name, common->name)) {
				ug_free(common->name);
				common->name = ug_strdup(name + 1);
				uget_plugin_post((UgetPlugin*) plugin,
						uget_event_new(UGET_EVENT_NAME));
			}
			ug_free(common->file);
			common->file = ug_strdup(name + 1);
		}
	}

	return TRUE;
}

// ----------------------------------------------------------------------------
// plugin_accept/plugin_start

static void  plugin_decide_uris(UgetPluginCurl* plugin);
static void  plugin_decide_folder(UgetPluginCurl* plugin);
static void  plugin_decide_files(UgetPluginCurl* plugin);
static UgThreadResult  plugin_thread(UgetPluginCurl* plugin);

static int  plugin_accept(UgetPluginCurl* plugin, UgInfo* node_info)
{
	union {
		UgetCommon*  common;
		UgetFiles*   files;
		UgetProxy*   proxy;
		UgetHttp*    http;
		UgetFtp*     ftp;
	} temp;
	int  speed[2];

	temp.common = ug_info_get(node_info, UgetCommonInfo);
	if (temp.common == NULL || temp.common->uri == NULL)
		return FALSE;
	plugin->common = ug_data_copy(temp.common);
	plugin_decide_uris(plugin);
	plugin_decide_folder(plugin);
	// speed control: decide speed limit before starting plug-in
	speed[0] = plugin->limit.download;
	speed[1] = plugin->limit.upload;
	plugin_ctrl_speed(plugin, speed);

	temp.files = ug_info_get(node_info, UgetFilesInfo);
	if (temp.files)
		plugin->files = ug_data_copy(temp.files);
	else
		plugin->files = ug_data_new(UgetFilesInfo);

	temp.proxy = ug_info_get(node_info, UgetProxyInfo);
	if (temp.proxy)
		plugin->proxy  = ug_data_copy(temp.proxy);

	temp.http = ug_info_get(node_info, UgetHttpInfo);
	if (temp.http) {
		plugin->http = ug_data_copy(temp.http);
		// check http->post_file
		if (temp.http->post_file) {
			if (ug_file_is_exist(temp.http->post_file) == FALSE) {
				uget_plugin_post((UgetPlugin*) plugin,
						uget_event_new_error(UGET_EVENT_ERROR_POST_FILE_NOT_FOUND,
						                     NULL));
				return FALSE;
			}
		}
		// check http->cookie_file
		if (temp.http->cookie_file) {
			if (ug_file_is_exist(temp.http->cookie_file) == FALSE) {
				uget_plugin_post((UgetPlugin*) plugin,
						uget_event_new_error(UGET_EVENT_ERROR_COOKIE_FILE_NOT_FOUND,
						                     NULL));
				return FALSE;
			}
		}
	}

	temp.ftp = ug_info_get(node_info, UgetFtpInfo);
	if (temp.ftp)
		plugin->ftp = ug_data_copy(temp.ftp);

	return TRUE;
}

static int  plugin_start(UgetPluginCurl* plugin)
{
	UgThread    thread;
	int         ok;

	plugin->start_time = time(NULL);
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

static UriLink* plugin_replace_uri(UgetPluginCurl* plugin, UriLink* old_link,
                                   const char* uri, int uri_len)
{
	UriLink*  uri_link;

	if (uri_len == -1)
		uri_len = strlen(uri);
	uri_link = ug_malloc(sizeof(UriLink) + uri_len);
	strncpy(uri_link->uri, uri, uri_len);
	uri_link->uri[uri_len] = 0;   // null terminated
	uri_link->self = uri_link;
	uri_link->next = NULL;
	uri_link->prev = NULL;
	uri_link->scheme_type = 0;
	uri_link->resumable = FALSE;
	uri_link->tested = FALSE;
	uri_link->ok = FALSE;

	// add to list
	if (old_link == NULL)
		ug_list_append(&plugin->uri.list, (void*) uri_link);
	else {
		ug_list_insert(&plugin->uri.list, (void*) old_link,
				(void*) uri_link);
		ug_list_remove(&plugin->uri.list, (void*) old_link);
		if (plugin->uri.link == (void*) old_link)
			plugin->uri.link  = (void*) uri_link;
		ug_free(old_link);
	}

	return uri_link;
}

static void plugin_decide_uris(UgetPluginCurl* plugin)
{
	UgetCommon*  common;
	const char*  curr;
	const char*  prev;

	common = plugin->common;
	// uri
	plugin->uri.link = (void*) plugin_replace_uri(plugin, NULL,
	                                              common->uri, -1);
	ug_free(common->uri);
	common->uri = NULL;
	// mirrors
	for (curr = common->mirrors;  curr && curr[0];) {
		// skip space ' '
		while (curr[0] == ' ')
			curr++;
		prev = curr;
		curr = curr + strcspn(curr, " ");
		// add to uri.list
		plugin_replace_uri(plugin, NULL, prev, curr - prev);
	}
	ug_free(common->mirrors);
	common->mirrors = NULL;
}

static void  plugin_decide_folder(UgetPluginCurl* plugin)
{
	UgetCommon* common;
	int         length;
	int         value;

	// folder
	common = plugin->common;
	if (common->folder == NULL || common->folder[0] == 0)
		length = 0;
	else {
		length = strlen(common->folder);
		value  = common->folder[length - 1];
		plugin->folder.path = ug_malloc(length + 2); // + '/' + '\x0'
		plugin->folder.path[0] = 0;
		strcpy(plugin->folder.path, common->folder);
		if (value != '\\' || value != '/') {
#if defined _WIN32 || defined _WIN64
			strcat(plugin->folder.path, "\\");
#else
			strcat(plugin->folder.path, "/");
#endif
			length++;
		}
	}
	plugin->folder.length = length;
}

static void  plugin_decide_files(UgetPluginCurl* plugin)
{
	// update UgetFiles
	uget_plugin_lock(plugin);
	// insert/replace file into files
	if (plugin->aria2.path) {
		uget_files_replace(plugin->files,
		                   plugin->aria2.path,
		                   UGET_FILE_TEMPORARY, 0);
	}
	uget_files_replace(plugin->files,
	                   plugin->file.path,
	                   UGET_FILE_REGULAR, 0);
	uget_plugin_unlock(plugin);
}

// ----------------------------------------------------------------------------
// plugin_thread

#define N_THREAD(plugin)   ((plugin)->segment.list.size)

static void delay_ms(UgetPluginCurl* plugin, int  milliseconds);
static int  switch_uri(UgetPluginCurl* plugin, UgetCurl* ugcurl, int is_resumable);
static int  prepare_file(UgetCurl* ugcurl, UgetPluginCurl* plugin);
static char* get_repeating_fmt_string(char* filename);
static void complete_file(UgetPluginCurl* plugin);
static int  load_file_info(UgetPluginCurl* plugin);
static void clear_file_info(UgetPluginCurl* plugin);
static int  reuse_download(UgetPluginCurl* plugin, UgetCurl* ugcurl, int next_uri);
static int  split_download(UgetPluginCurl* plugin, UgetCurl* ugcurl);
static void adjust_speed_limit(UgetPluginCurl* plugin);
static UgetCurl* create_segment(UgetPluginCurl* plugin);

static UgThreadResult  plugin_thread(UgetPluginCurl* plugin)
{
	UgetCommon* common;
	UgetCurl*   ugcurl;
	UgetCurl*   ugnext;
	int         counter;
	int         n_active_last = 0;
	struct {
		int64_t upload;
		int64_t download;
	} size, speed;

	common = plugin->common;
	common->retry_count = 0;
	plugin->segment.n_max = common->max_connections;
	if (plugin->segment.n_max == 0)
		plugin->segment.n_max = 1;

	// create new segment and add it to segment.list
	ugcurl = create_segment(plugin);
	if (load_file_info(plugin)) {
		uget_curl_open_file(ugcurl, plugin->file.path);
		ugcurl->beg = plugin->segment.beg;
		uget_a2cf_lack(&plugin->aria2.ctrl,
		               (uint64_t*) &ugcurl->beg,
		               (uint64_t*) &ugcurl->end);
		plugin->segment.beg = ugcurl->end;
		// plugin_sync() will set foreign UgetCommon::name
		plugin->file_renamed = TRUE;
		plugin->synced = FALSE;
	}
	else {
		clear_file_info(plugin);
		ugcurl->prepare.func = (UgetCurlFunc) prepare_file;
		ugcurl->prepare.data = plugin;
		ugcurl->header_store = TRUE;
	}
	ug_list_append(&plugin->segment.list, (void*) ugcurl);

	// start curl
	uget_curl_run(ugcurl, FALSE);

	// main loop
	for (counter = 0;  N_THREAD(plugin) > 0;  counter++) {
		// sleep 0.5 second
		ug_sleep(500);
		// reset data, plug-in will count them (in segment loop) later
		plugin->segment.n_active = 0;
		size.upload = 0;
		size.download = 0;
		speed.upload = 0;
		speed.download = 0;

		// segment loop
		ugcurl = (UgetCurl*) plugin->segment.list.head;
		for (;  ugcurl;  ugcurl = ugnext) {
			ugnext = ugcurl->next;

			// split download, use these code with split_download()
			if (ugcurl->split) {
				if (ugcurl->prev == NULL || ugcurl->prev->end < ugcurl->beg)
					ugcurl->split = FALSE;
				else if (ugcurl->beg < ugcurl->prev->pos) {
					// if previous segment overwrite current one.
					ugcurl->split = FALSE;
					ugcurl->paused = TRUE;
					ugcurl->end = ugcurl->beg;
					ugcurl->pos = ugcurl->beg;
					ugcurl->size[0] = 0;
#ifndef NDEBUG
					if (common->debug_level) {
						printf("\n"
						       "previous segment overwrite at %u KiB\n",
						       (unsigned) (ugcurl->beg / 1024));
					}
#endif
				}
				else if (ugcurl->beg < ugcurl->pos) {
					// If this segment has downloaded data,
					// plug-in split new segment from previous one.
					ugcurl->split = FALSE;
					ugcurl->prev->end = ugcurl->beg;
					if (ugcurl->prev->pos > ugcurl->beg) {
						ugcurl->prev->pos = ugcurl->beg;
						ugcurl->prev->size[0] = ugcurl->prev->pos -
						                        ugcurl->prev->beg;
					}
#ifndef NDEBUG
					if (common->debug_level) {
						printf("\n"
						       "split new segment at %u KiB\n",
						       (unsigned) (ugcurl->beg / 1024));
					}
#endif
				}
			}

			// if user want to stop plug-in, it must stop all UgetCurl in list.
			if (plugin->paused) {
				ugcurl->paused = TRUE;
				plugin->segment.n_max = 0;
			}
			// update aria2 control file progress
			if (plugin->aria2.path)
				uget_a2cf_fill(&plugin->aria2.ctrl, ugcurl->beg, ugcurl->pos);
			// progress
			if (ugcurl->state >= UGET_CURL_OK) {
				// ugcurl has stopped
				plugin->base.upload += ugcurl->size[1];
				plugin->base.download += ugcurl->size[0];
			}
			else if (ugcurl->state == UGET_CURL_RUN) {
				size.upload += ugcurl->size[1];
				size.download += ugcurl->size[0];
				speed.upload += ugcurl->speed[1];
				speed.download += ugcurl->speed[0];
			}

			// handle UgetCurl by state
			switch (ugcurl->state) {
			default:
				break;

			case UGET_CURL_RUN:
				plugin->segment.n_active++;
				break;

			case UGET_CURL_OK:
				ugcurl->state = UGET_CURL_RESPLIT;
				// special case for unknown file size
				if (plugin->file.size == 0 && N_THREAD (plugin) == 1) {
					complete_file(plugin);
					// delete download
					ug_list_remove(&plugin->segment.list, (void*)ugcurl);
					uget_curl_free(ugcurl);
				}
				break;

			case UGET_CURL_ABORT:
				// delete download
				ug_list_remove(&plugin->segment.list, (void*)ugcurl);
				uget_curl_free(ugcurl);
				break;

			case UGET_CURL_ERROR:
				// if no other downloading segment, plug-in response error
				if (N_THREAD(plugin) == 1) {
					// post error message
					if (ugcurl->event) {
						uget_plugin_post((UgetPlugin*) plugin, ugcurl->event);
						ugcurl->event = NULL;
					}
					// delete download
					ug_list_remove(&plugin->segment.list, (void*)ugcurl);
					uget_curl_free(ugcurl);
				}
				else {
					// try to reuse download
					reuse_download(plugin, ugcurl, TRUE);
				}
				break;

			case UGET_CURL_RETRY:
				// if no other downloading segment
				if (N_THREAD(plugin) == 1) {
					common->retry_count++;
					if (common->retry_count < common->retry_limit ||
					    common->retry_limit == 0)
					{
						ugcurl->beg = ugcurl->pos;
						delay_ms(plugin, common->retry_delay * 1000);
						uget_curl_run(ugcurl, FALSE);
					}
					else {
						// delete segment
						ug_list_remove(&plugin->segment.list, (void*)ugcurl);
						uget_curl_free(ugcurl);
					}
				}
				else {
					// try to reuse download
					reuse_download(plugin, ugcurl, FALSE);
				}
				break;

			case UGET_CURL_NOT_RESUMABLE:
				// if no other downloading segment
				if (N_THREAD(plugin) == 1) {
					uget_plugin_post((UgetPlugin*) plugin,
							uget_event_new_normal(
									UGET_EVENT_NORMAL_NOT_RESUMABLE, NULL));
					common->retry_count++;
					if (common->retry_count < common->retry_limit ||
					    common->retry_limit == 0)
					{
						plugin->base.download = 0;
						plugin->size.download = 0;
						ugcurl->beg = 0;
						ugcurl->end = plugin->file.size;
						delay_ms(plugin, common->retry_delay * 1000);
						switch_uri(plugin, ugcurl, TRUE);
						uget_curl_run(ugcurl, FALSE);
					}
					else {
						// delete download
						ug_list_remove(&plugin->segment.list, (void*)ugcurl);
						uget_curl_free(ugcurl);
					}
				}
				else {
					// try to reuse download
					reuse_download(plugin, ugcurl, TRUE);
				}
				break;
			}
		}

		// use completed UgetCurl to split new segment after segment loop
		ugcurl = (UgetCurl*) plugin->segment.list.head;
		for (;  ugcurl;  ugcurl = ugnext) {
			ugnext = ugcurl->next;
			if (ugcurl->state == UGET_CURL_RESPLIT) {
				if (split_download(plugin, ugcurl) == FALSE) {
					// delete download
					ug_list_remove(&plugin->segment.list, (void*)ugcurl);
					uget_curl_free(ugcurl);
				}
			}
		}
		// progress ---------------------
		plugin->size.upload = plugin->base.upload + size.upload;
		plugin->size.download = plugin->base.download + size.download;
		// Don't update speed when stopping
		if (plugin->segment.list.size) {
			plugin->speed.upload = speed.upload;
			plugin->speed.download = speed.download;
		}
		plugin->synced = FALSE;
		// check file size --------------
		if (plugin->file.size) {
			// response error if file size is different
			if (plugin->file.size < plugin->size.download) {
#if 0

#ifndef NDEBUG
				if (common->debug_level) {
					printf("file size is different\n");
					printf("plugin->file.size = %d\n",     (int)plugin->file.size);
					printf("plugin->size.download = %d\n", (int)plugin->size.download);
				}
#endif  // NDEBUG
				plugin->size.download = plugin->file.size;
#else
				plugin->paused = TRUE;
				if (N_THREAD(plugin) > 0)
					continue;    // wait other thread
				else {
					if (plugin->aria2.path)
						ug_unlink(plugin->aria2.path);
					uget_plugin_post((UgetPlugin*) plugin,
							uget_event_new_error(
									UGET_EVENT_ERROR_INCORRECT_SOURCE,
									NULL));
					plugin->synced = FALSE;
					break;
				}
#endif
			}
			// download completed
			if (plugin->file.size == plugin->size.download) {
				if (N_THREAD(plugin) > 0)
					continue;    // wait other thread
				else {
					complete_file(plugin);
					plugin->synced = FALSE;
					break;
				}
			}
		}
		// timer ------------------------
		// adjust speed every 0.5 x 2 = 1 second.
		if ((counter & 1) == 1 || n_active_last != plugin->segment.n_active) {
			n_active_last = plugin->segment.n_active;
			adjust_speed_limit(plugin);
		}
		// save aria2 control file every 0.5 x 4 = 2 seconds.
		if ((counter & 3) == 3 || N_THREAD(plugin) == 0) {
			if (plugin->aria2.path)
				uget_a2cf_save(&plugin->aria2.ctrl, plugin->aria2.path);
		}
		// split download every 0.5 x 8 = 4 seconds.
		if ((counter & 7) == 7 && plugin->file.size) {
			// If some threads are connecting, It doesn't split new segment.
			if (N_THREAD(plugin) <  plugin->segment.n_max &&
			    N_THREAD(plugin) == plugin->segment.n_active)
			{
				split_download(plugin, NULL);
			}
		}
		// retry ------------------------
		if (common->retry_count >= common->retry_limit && common->retry_limit != 0) {
			uget_plugin_post((UgetPlugin*) plugin,
					uget_event_new_error(
							UGET_EVENT_ERROR_TOO_MANY_RETRIES, NULL));
			plugin->synced = FALSE;
			plugin->paused = TRUE;
		}
	}

	// count the latest downloaded size if download doesn't complete
	if ((plugin->file.size != plugin->size.download) && plugin->aria2.path) {
		plugin->size.download = uget_a2cf_completed(&plugin->aria2.ctrl);
		plugin->synced = FALSE;
	}

	// free segment list
	ug_list_foreach(&plugin->segment.list, (UgForeachFunc) uget_curl_free, NULL);
	ug_list_clear(&plugin->segment.list, FALSE);
	//
	uget_a2cf_clear(&plugin->aria2.ctrl);
	plugin->stopped = TRUE;
	uget_plugin_unref((UgetPlugin*) plugin);
	return UG_THREAD_RESULT;
}

static int prepare_existed(UgetCurl* ugcurl, UgetPluginCurl* plugin)
{
	long    ftime;

	// file.size
	if (plugin->file.size) {
		curl_off_t content_length;
		curl_easy_getinfo(ugcurl->curl,
				CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &content_length);
		if (plugin->file.size != ugcurl->beg + (int64_t) content_length) {
			// if remote file size and local file size are not the same,
			// plug-in will create new download file.
			if (plugin->prepared == FALSE) {
				// plugin_thread() has initialized/created some data for this function.
				// program must clear these data before calling prepare_file()
				clear_file_info(plugin);
				uget_curl_close_file(ugcurl);
				return prepare_file(ugcurl, plugin);
			}
			// don't write INCORRECT data to existed file.
			ugcurl->event_code = UGET_EVENT_ERROR_INCORRECT_SOURCE;
			ugcurl->size[0] = 0;
			return FALSE;
		}
	}
	// file.time
	if (plugin->file.time == -1) {
		curl_easy_getinfo(ugcurl->curl, CURLINFO_FILETIME, &ftime);
		plugin->file.time = (time_t) ftime;
	}

	if (uget_curl_open_file(ugcurl, plugin->file.path)) {
		plugin->prepared = TRUE;
		return TRUE;
	}
	else {
		ugcurl->event_code = UGET_EVENT_ERROR_FILE_OPEN_FAILED;
		return FALSE;
	}
}

static int prepare_file(UgetCurl* ugcurl, UgetPluginCurl* plugin)
{
	UgetCommon*  common;
	int    length;
	int    counts;
	int    value;
	union {
		long     ftime;
		double   fsize;
		int64_t  val64;
		UriLink* ulink;
	} temp;

	// file.time
	curl_easy_getinfo(ugcurl->curl, CURLINFO_FILETIME, &temp.ftime);
	plugin->file.time = (time_t) temp.ftime;
	// file.size
	{
		curl_off_t content_length;
		curl_easy_getinfo(ugcurl->curl,
				CURLINFO_CONTENT_LENGTH_DOWNLOAD_T, &content_length);
		plugin->file.size = (int64_t) content_length + ugcurl->beg;
	}
	if (plugin->file.size == -1)
		plugin->file.size = 0;

	common = plugin->common;
	length = plugin->folder.length;
	// decide filename
	if (common->file == NULL) {
		if (ugcurl->header.filename) {
			common->file = ugcurl->header.filename;
			ugcurl->header.filename = NULL;
		}
		else if (ugcurl->uri.part.file != -1)
			common->file = ug_uri_get_file(&ugcurl->uri.part);
		// if it is still no filename, set default one
		if (common->file == NULL)
			common->file = ug_strdup("index");
		// replace invalid characters \/:*?"<>| by _ in filename.
		ug_str_replace_chars(common->file, "\\/:*?\"<>|", '_');
	}
	length += strlen(common->file);

	// path = folder + filename
	//                             length + digits + ".aria2" + '\0'
	plugin->file.path = ug_malloc(length + MAX_REPEAT_DIGITS + 6 + 1);
	plugin->file.path[0] = 0;  // you need this line if common->folder is NULL.
	if (plugin->folder.path)
		strcpy(plugin->file.path, plugin->folder.path);
	strcat(plugin->file.path, common->file);
	plugin->file.name_fmt = get_repeating_fmt_string(common->file);

	// create folder
	if (ug_create_dir_all(plugin->file.path, plugin->folder.length) == -1) {
		ugcurl->event_code = UGET_EVENT_ERROR_FOLDER_CREATE_FAILED;
		return FALSE;
	}

	// create file
	for (counts = 0;  counts < MAX_REPEAT_COUNTS;  counts++) {
		value = ug_open(plugin->file.path, UG_O_CREATE | UG_O_EXCL | UG_O_WRONLY,
				UG_S_IREAD | UG_S_IWRITE | UG_S_IRGRP | UG_S_IROTH);
//		value = ug_open(plugin->file.path, UG_O_CREATE | UG_O_EXCL | UG_O_RDWR,
//				UG_S_IREAD | UG_S_IWRITE | UG_S_IRGRP | UG_S_IROTH);
		// check if this path can't access
//		if (value == -1 && ug_file_is_exist(plugin->file.path) == FALSE) {
//			ugcurl->event_code = UGET_EVENT_ERROR_FILE_CREATE_FAILED;
//			return FALSE;    // error
//		}

		// check exist downloaded file & it's control file
		strcat(plugin->file.path + length, ".aria2");
		if (value == -1) {
			if (uget_a2cf_load(&plugin->aria2.ctrl, plugin->file.path)) {
				if (plugin->aria2.ctrl.total_len == plugin->file.size) {
					plugin->aria2.path = ug_strdup(plugin->file.path);
					plugin->base.download = uget_a2cf_completed(&plugin->aria2.ctrl);
					plugin->size.download = plugin->base.download;
					*(char*) strstr(plugin->file.path + length, ".aria2") = 0;
					break;
				}
				uget_a2cf_clear(&plugin->aria2.ctrl);
			}
		}
		else {
			// reset downloaded size if plug-in decide to create new file.
			plugin->base.download = 0;
			plugin->size.download = 0;
			// allocate disk space if plug-in known file size
			if (plugin->file.size) {
				// preallocate space for a file.
#if defined _WIN32 || defined _WIN64
				LARGE_INTEGER size;
				HANDLE        handle;
				handle = (HANDLE) _get_osfhandle(value);
				size.QuadPart = plugin->file.size;
				if(SetFilePointerEx(handle ,size, 0, FILE_BEGIN) == FALSE)
					ugcurl->event_code = UGET_EVENT_ERROR_OUT_OF_RESOURCE;
				if(SetEndOfFile(handle) == FALSE)
					ugcurl->event_code = UGET_EVENT_ERROR_OUT_OF_RESOURCE;
				SetFilePointer(handle, 0, 0, FILE_BEGIN);
#elif defined HAVE_FTRUNCATE
				if (ftruncate(value, plugin->file.size) == -1)
					ugcurl->event_code = UGET_EVENT_ERROR_OUT_OF_RESOURCE;
#elif defined __ANDROID__ && __ANDROID_API__ >= 12
				if (ftruncate64(value, plugin->file.size) == -1)
					ugcurl->event_code = UGET_EVENT_ERROR_OUT_OF_RESOURCE;
#elif defined HAVE_POSIX_FALLOCATE
				if (posix_fallocate(value, 0, plugin->file.size) != 0)
					ugcurl->event_code = UGET_EVENT_ERROR_OUT_OF_RESOURCE;
#elif defined __ANDROID__ && __ANDROID_API__ >= 20
				if (posix_fallocate64(value, 0, plugin->file.size) != 0)
					ugcurl->event_code = UGET_EVENT_ERROR_OUT_OF_RESOURCE;
#else
				if (ug_write(value, "O", 1) == -1)  // begin of file
					ugcurl->event_code = UGET_EVENT_ERROR_OUT_OF_RESOURCE;
				if (ug_seek(value, plugin->file.size - 1, SEEK_SET) == -1)
					ugcurl->event_code = UGET_EVENT_ERROR_OUT_OF_RESOURCE;
				if (ug_write(value, "X", 1) == -1)  // end of file
					ugcurl->event_code = UGET_EVENT_ERROR_OUT_OF_RESOURCE;
#endif // _WIN32 || _WIN64
				// create aria2 control file if no error
				if (ugcurl->event_code == 0) {
					plugin->aria2.path = ug_strdup(plugin->file.path);
					uget_a2cf_init(&plugin->aria2.ctrl, plugin->file.size);
					uget_a2cf_save(&plugin->aria2.ctrl, plugin->aria2.path);
				}
			}
			ug_close(value);
			// remove tail ".aria2" string in file path
			*(char*) strstr(plugin->file.path + length, ".aria2") = 0;
			// if error occurred while allocating disk space, delete created download file.
			if (ugcurl->event_code > 0) {
				ug_unlink(plugin->file.path);
				return FALSE;
			}
			break;
		}

		sprintf(plugin->file.path + plugin->folder.length,
				plugin->file.name_fmt, counts);
	}

	if (counts == MAX_REPEAT_COUNTS) {
		ugcurl->event_code = UGET_EVENT_ERROR_FILE_CREATE_FAILED;
		return FALSE;
	}

	// set filename if counts > 0
	if (counts) {
		ug_free(common->file);
		common->file = ug_strdup(plugin->file.path + plugin->folder.length);
	}
	plugin->file_renamed = TRUE;
	// update UgetFiles
	plugin_decide_files(plugin);

	// event
	if (ugcurl->resumable) {
		uget_plugin_post((UgetPlugin*) plugin,
				uget_event_new_normal(UGET_EVENT_NORMAL_RESUMABLE, NULL));
	}
	else {
		uget_plugin_post((UgetPlugin*) plugin,
				uget_event_new_normal(UGET_EVENT_NORMAL_NOT_RESUMABLE, NULL));
	}
#ifndef NDEBUG
	if (common->debug_level) {
		printf("CURL message = %s\n", ugcurl->error_string);
		printf("plugin->file.path = %s\n", plugin->file.path);
		printf("plugin->file.size = %d\n", (int)plugin->file.size);
		printf("plugin->file.time = %d\n", (int)plugin->file.time);
		printf("resumable = %d\n", ugcurl->resumable);
	}
#endif

	// set flags to UriLink
	if (ugcurl->header.uri) {
		// HTTP redirection
		temp.ulink = plugin_replace_uri(plugin, ugcurl->uri.link,
		                                ugcurl->header.uri, -1);
		ugcurl->uri.link = temp.ulink;
		ug_free(ugcurl->header.uri);
		ugcurl->header.uri = NULL;
	}
	else
		temp.ulink = ugcurl->uri.link;
	temp.ulink->scheme_type = ugcurl->scheme_type;
	temp.ulink->resumable = ugcurl->resumable;
	temp.ulink->tested = TRUE;
	temp.ulink->ok = TRUE;
	// change callback
	ugcurl->prepare.func = (UgetCurlFunc) prepare_existed;
	ugcurl->prepare.data = plugin;
	// prepare to download
	plugin->prepared = TRUE;
	// file and it's offset
	temp.val64 = 0;
	uget_a2cf_lack(&plugin->aria2.ctrl,
	               (uint64_t*) &temp.val64,
	               (uint64_t*) &ugcurl->end);
	plugin->segment.beg = ugcurl->end;
	if (ugcurl->beg == temp.val64) {
		if (uget_curl_open_file(ugcurl, plugin->file.path) == FALSE) {
			ugcurl->event_code = UGET_EVENT_ERROR_FILE_OPEN_FAILED;
			return FALSE;
		}
		return TRUE;
	}
	else {
		ugcurl->beg = temp.val64;
		ugcurl->pos = temp.val64;
		curl_easy_setopt(ugcurl->curl, CURLOPT_RESUME_FROM_LARGE,
				(curl_off_t) temp.val64);
		if (uget_curl_open_file(ugcurl, plugin->file.path))
			ugcurl->restart = TRUE;
		return FALSE;
	}
}

// used by get_repeating_fmt_string()
enum {
	EXT_NUMBER = 0x01,
	EXT_UPPER  = 0x02,
	EXT_LOWER  = 0x04,

	EXT_CAMEL  = EXT_UPPER | EXT_LOWER,
	EXT_ALL    = EXT_UPPER | EXT_LOWER | EXT_NUMBER,
};

// used by get_repeating_fmt_string()
static char* find_dot_ext(const char* filename, const char* end, uint8_t* status)
{
	uint8_t     stat = 0;
    const char* cur = NULL;

	for (cur = end-1;  cur >= filename;  cur--) {
		if (cur[0] == '.') {
			if (cur == end-1 || cur == filename)
				break;
			if (status)
				status[0] = stat;
			return (char*)cur;
		}
		else if (cur[0] >= '0' && cur[0] <= '9')
			stat |= EXT_NUMBER;
		else if (cur[0] >= 'A' && cur[0] <= 'Z')
			stat |= EXT_UPPER;
		else if (cur[0] >= 'a' && cur[0] <= 'z')
			stat |= EXT_LOWER;
		else
			break;
	}
	return NULL;
}

static char* get_repeating_fmt_string(char* filename)
{
	char*   result;
	char*   dot[3] = {NULL, NULL, NULL};
	uint8_t stat[3] = {0, 0, 0};
	int     index;
	int     length;

	length = strlen(filename);
	result = ug_malloc(length + 3 + 1);  // + ".%d" + "\0"

	// dot[0]
	dot[0] = find_dot_ext(filename, filename + length, &stat[0]);
	if (dot[0])
		dot[1] = find_dot_ext(filename, dot[0], &stat[1]);
	if (dot[1])
		dot[2] = find_dot_ext(filename, dot[1], &stat[2]);

	for (index = 2;  index >= 0;  index--) {
		if (dot[index] == NULL)
			continue;

		// replace exist number
		if (stat[index] == EXT_NUMBER) {
			// strncpy() doesn't always null-terminated
			strncpy(result, filename, dot[index] - filename);
			result[dot[index] - filename] = 0;
			strcat(result, ".%d");
			if (index > 0)
				strcat(result, dot[index-1]);
			return result;
		}

		// insert number
		if (index == 2)
			continue;
		// strncpy() doesn't always null-terminated
		strncpy(result, filename, dot[index] - filename);
		result[dot[index] - filename] = 0;
		strcat(result, ".%d");
		strcat(result, dot[index]);
		return result;
	}

	// append number
	strcpy(result, filename);
	strcat(result, ".%d");

	// return printf() format string
	return result;
}

static int  load_file_info(UgetPluginCurl* plugin)
{
	UgetCommon*  common;
	char*        path;
	int          length;

	common = plugin->common;
	if (common == NULL || common->file == NULL)
		return FALSE;
	// folder + filename
	length = plugin->folder.length;
	length += strlen(common->file);
	// path
	path = ug_malloc(length + 6 + 1);  // length + ".aria2" + '\0'
	path[0] = 0;  // you need this line if common->folder is NULL.
	if (plugin->folder.path)
		strcpy(path, plugin->folder.path);
	strcat(path, common->file);
	if (ug_file_is_exist(path) == FALSE) {
		ug_free(path);
		return FALSE;
	}
	strcat(path, ".aria2");
	// aria2 control file
	if (uget_a2cf_load(&plugin->aria2.ctrl, path)) {
		plugin->file.size = plugin->aria2.ctrl.total_len;
		plugin->file.path = ug_strndup(path, length);
		plugin->aria2.path = path;
		plugin->base.download = uget_a2cf_completed(&plugin->aria2.ctrl);
		plugin->size.download = plugin->base.download;
		// update UgetFiles
		plugin_decide_files(plugin);
		return TRUE;
	}
	else {
		uget_a2cf_clear(&plugin->aria2.ctrl);
		ug_free(path);
		return FALSE;
	}
}

static void  clear_file_info(UgetPluginCurl* plugin)
{
	// update UgetFiles
	uget_plugin_lock(plugin);
	uget_files_apply_deleted(plugin->files);
	uget_plugin_unlock(plugin);

	uget_a2cf_clear(&plugin->aria2.ctrl);
	ug_free(plugin->aria2.path);
	plugin->aria2.path = NULL;

	ug_free(plugin->file.path);
	plugin->file.path = NULL;
	plugin->file.size = 0;
	plugin->base.download = 0;
	plugin->size.download = 0;
}

static int  switch_uri(UgetPluginCurl* plugin, UgetCurl* ugcurl, int is_resumable)
{
	UriLink*  uri_link;

	uri_link = (UriLink*) plugin->uri.link;
	if (uri_link == NULL)
		uri_link = (UriLink*) plugin->uri.list.head;

	// set URI and decide it's scheme
	uget_curl_set_url(ugcurl, uri_link->uri);
	uri_link->scheme_type = ugcurl->scheme_type;
	// sync URI flags to UgetCurl
	ugcurl->uri.link = uri_link;
	ugcurl->resumable = uri_link->resumable;
	ugcurl->tested = uri_link->tested;
	ugcurl->test_ok = uri_link->ok;
	// pointer current URI to next one
	plugin->uri.link = (void*) uri_link->next;

	return TRUE;
}

static void complete_file(UgetPluginCurl* plugin)
{
	if (plugin->aria2.path) {
		// update UgetFiles
		uget_plugin_lock(plugin);
		uget_files_replace(plugin->files,
		                   plugin->file.path,
		                   UGET_FILE_REGULAR, UGET_FILE_STATE_COMPLETED);
		uget_files_replace(plugin->files,
		                   plugin->aria2.path,
		                   UGET_FILE_ATTACHMENT, UGET_FILE_STATE_DELETED);
		uget_plugin_unlock(plugin);
		// delete aria2 control file
		ug_unlink(plugin->aria2.path);
		ug_free(plugin->aria2.path);
		plugin->aria2.path = NULL;
	}
	// modify file time
	if (plugin->common->timestamp == TRUE && plugin->file.time != -1)
		ug_modify_file_time(plugin->file.path, plugin->file.time);
	// completed message
	uget_plugin_post((UgetPlugin*)plugin,
			uget_event_new(UGET_EVENT_COMPLETED));
	uget_plugin_post((UgetPlugin*)plugin,
			uget_event_new(UGET_EVENT_STOP));
}

static int  reuse_download(UgetPluginCurl* plugin, UgetCurl* ugcurl, int next_uri)
{
	if (ugcurl->beg == ugcurl->pos) {
		// delete segment if no downloaded data
		ug_list_remove(&plugin->segment.list, (void*)ugcurl);
		uget_curl_free(ugcurl);
		return FALSE;
	}
	else {
		// reuse this segment
		if (next_uri == TRUE)
			switch_uri(plugin, ugcurl, TRUE);
		ugcurl->beg = ugcurl->pos;
		uget_curl_run(ugcurl, FALSE);
		return TRUE;
	}
}

static int  split_download(UgetPluginCurl* plugin, UgetCurl* ugcurl)
{
	UgetCurl*  temp;
	UgetCurl*  sibling = NULL;
	uint64_t   cur;
	uint64_t   end;

	if (plugin->aria2.path == NULL)
		return FALSE;

	// try to find unused space
	cur = plugin->segment.beg;
	if (uget_a2cf_lack(&plugin->aria2.ctrl, &cur, &end)) {
		plugin->segment.beg = end;
#ifndef NDEBUG
		if (plugin->common->debug_level) {
			printf("\n" "lack %u-%u KiB\n",
			       (unsigned) cur / 1024,
			       (unsigned) end / 1024);
		}
#endif
	}
	// if no unused space, try to split downloading segment.
	else {
		// cur = segment size;  end = the largest segment size;
		end = 0;
		for (temp = (void*)plugin->segment.list.head;  temp;  temp = temp->next) {
			cur = temp->end - temp->pos;
			if (end < cur) {
				end = cur;
				sibling = temp;
			}
		}
		if (sibling == NULL)
			return FALSE;
		cur = (sibling->end - sibling->pos) >> 1;
		// if segment is too small, don't split it.
		if (cur < MIN_SPLIT_SIZE)
			return FALSE;
		// cur = begin of new segment;  end = end of new segment;
		cur = sibling->end - cur;
		end = sibling->end;
		if (cur & 16383)
			cur += 16384 - (cur & 16383);

#ifndef NDEBUG
		if (plugin->common->debug_level) {
			printf("\n" "split %u-%u KiB\n",
			       (unsigned) cur / 1024,
			       (unsigned) end / 1024);
		}
#endif
	}

	// reuse or create UgetCurl
	// if this UgetCurl has been inserted in segment.list, remove it.
	if (ugcurl)
		ug_list_remove(&plugin->segment.list, (UgLink*) ugcurl);
	else
		ugcurl = create_segment(plugin);

	// add to segment.list
	if (sibling == NULL)
		ug_list_append(&plugin->segment.list, (void*) ugcurl);
	else {
		ugcurl->split = TRUE;
		ug_list_insert(&plugin->segment.list,
				(void*) sibling->next, (void*) ugcurl);
	}

	ugcurl->beg = cur;
	ugcurl->end = end;
	uget_curl_run(ugcurl, FALSE);
	return TRUE;
}

static void delay_ms(UgetPluginCurl* plugin, int  milliseconds)
{
	while (plugin->paused == FALSE) {
		if (milliseconds >  500) {
			milliseconds -= 500;
			ug_sleep(500);
			continue;
		}
		ug_sleep(milliseconds);
		return;
	}
}

static UgetCurl* create_segment(UgetPluginCurl* plugin)
{
	UgetCurl*  ugcurl;

	ugcurl = uget_curl_new();
	uget_curl_set_common(ugcurl, plugin->common);
	uget_curl_set_proxy(ugcurl, plugin->proxy);
	uget_curl_set_http(ugcurl, plugin->http);
	uget_curl_set_ftp(ugcurl, plugin->ftp);
	// set speed limit
	if (plugin->limit.download)
		ugcurl->limit[0] = plugin->limit.download / (plugin->segment.list.size + 1);
	if (plugin->limit.upload)
		ugcurl->limit[1] = plugin->limit.upload / (plugin->segment.list.size + 1);
	// select URL
	switch_uri(plugin, ugcurl, FALSE);
	// set output function
	ugcurl->prepare.func = (UgetCurlFunc) prepare_existed;
	ugcurl->prepare.data = plugin;
	return ugcurl;
}

// speed control
static void  adjust_speed_limit_index(UgetPluginCurl* plugin, int idx, int64_t remain)
{
	UgetCurl*  ucurl;

	// balance speed
	remain = remain / plugin->segment.n_active;

	for (ucurl = (UgetCurl*) plugin->segment.list.head; ucurl; ucurl=ucurl->next) {
		if (ucurl->state != UGET_CURL_RUN)
			continue;
		ucurl->limit[idx] = ucurl->speed[idx] + remain;
		if (ucurl->limit[idx] < MIN_SPEED_LIMIT)
			ucurl->limit[idx] = MIN_SPEED_LIMIT;
		ucurl->limit_changed = TRUE;
	}
}

static void  disable_speed_limit(UgetPluginCurl* plugin, int idx)
{
	UgetCurl*  ugcurl;

	ugcurl = (UgetCurl*) plugin->segment.list.head;
	for (;  ugcurl;  ugcurl = ugcurl->next) {
		ugcurl->limit[idx] = 0;
		ugcurl->limit_changed = TRUE;
	}
}

static void  adjust_speed_limit(UgetPluginCurl* plugin)
{
	if (plugin->segment.n_active == 0)
		return;

	// download
	if (plugin->limit.download > 0)
		adjust_speed_limit_index(plugin, 0, plugin->limit.download - plugin->speed.download);
	else if (plugin->limit_changed)
		disable_speed_limit(plugin, 0);
	// upload
	if (plugin->limit.upload > 0)
		adjust_speed_limit_index(plugin, 1, plugin->limit.upload - plugin->speed.upload);
	else if (plugin->limit_changed)
		disable_speed_limit(plugin, 1);

	plugin->limit_changed = FALSE;
}

