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

#include <stdlib.h>
#include <UgString.h>
#include <UgJson.h>
#include <UgUtil.h>
#include <UgetData.h>
#include <UgJson-custom.h>

// ----------------------------------------------------------------------------
// UgetCommon
static void uget_common_init(UgetCommon* common);
static void uget_common_final(UgetCommon* common);
static int  uget_common_assign(UgetCommon* common, UgetCommon* src);

static const UgEntry  UgetCommonEntry[] =
{
	{"name",     offsetof(UgetCommon, name),     UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"uri",      offsetof(UgetCommon, uri),      UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"mirrors",  offsetof(UgetCommon, mirrors),  UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"file",     offsetof(UgetCommon, file),     UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"folder",   offsetof(UgetCommon, folder),   UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"user",     offsetof(UgetCommon, user),     UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"password", offsetof(UgetCommon, password), UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"format-id", offsetof(UgetCommon, format_id), UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"connect-timeout",    offsetof(UgetCommon, connect_timeout),
			UG_ENTRY_UINT,  NULL, NULL},
	{"transmit-timeout",   offsetof(UgetCommon, transmit_timeout),
			UG_ENTRY_UINT,  NULL, NULL},
	{"retry-delay",        offsetof(UgetCommon, retry_delay),
			UG_ENTRY_INT,   NULL, NULL},
	{"retry-limit",        offsetof(UgetCommon, retry_limit),
			UG_ENTRY_INT,   NULL, NULL},
	{"retry-count",        offsetof(UgetCommon, retry_count),
			UG_ENTRY_INT,   NULL, NULL},
	{"max-connections",    offsetof(UgetCommon, max_connections),
			UG_ENTRY_UINT,  NULL, NULL},
	{"max-upload-speed",   offsetof(UgetCommon, max_upload_speed),
			UG_ENTRY_INT,   NULL, NULL},
	{"max-download-speed", offsetof(UgetCommon, max_download_speed),
			UG_ENTRY_INT,   NULL, NULL},
	{"timestamp",          offsetof(UgetCommon, timestamp),
			UG_ENTRY_INT,   NULL, NULL},
	{NULL}    // null-terminated
};

static const UgDataInfo  UgetCommonInfoStatic =
{
	.name   = "common",
	.size   = sizeof(UgetCommon),
	.init   = (UgInitFunc)   uget_common_init,
	.final  = (UgFinalFunc)  uget_common_final,
	.assign = (UgAssignFunc) uget_common_assign,
	.entry  = UgetCommonEntry,
};
// extern
const UgDataInfo*  UgetCommonInfo = &UgetCommonInfoStatic;

static void uget_common_init(UgetCommon* common)
{
	common->connect_timeout  = 15;
	common->transmit_timeout = 30;
	common->retry_delay = 6;
	common->retry_limit = 99;
	common->max_connections = 1;
	common->timestamp = TRUE;
#ifndef NDEBUG
	common->debug_level = 1;
#endif
}

static void uget_common_final(UgetCommon* common)
{
	ug_free(common->name);
	ug_free(common->uri);
	ug_free(common->mirrors);
	ug_free(common->file);
	ug_free(common->folder);
	ug_free(common->user);
	ug_free(common->password);
	ug_free(common->format_id);
}

static int  uget_common_assign(UgetCommon* common, UgetCommon* src)
{
#if 0
	// Program can NOT copy UgetCommon::name to other one.
	if (common->keeping.enable == FALSE || common->keeping.name == FALSE) {
		ug_free(common->name);
		common->name = (src->name) ? ug_strdup(src->name) : NULL;
	}
#endif
	if (common->keeping.enable == FALSE || common->keeping.uri == FALSE) {
		ug_free(common->uri);
		common->uri = (src->uri) ? ug_strdup(src->uri) : NULL;
		common->keeping.uri = src->keeping.uri;
	}
	if (common->keeping.enable == FALSE || common->keeping.mirrors == FALSE) {
		ug_free(common->mirrors);
		common->mirrors = (src->mirrors) ? ug_strdup(src->mirrors) : NULL;
		common->keeping.mirrors = src->keeping.mirrors;
	}
	if (common->keeping.enable == FALSE || common->keeping.file == FALSE) {
		ug_free(common->file);
		common->file = (src->file) ? ug_strdup(src->file) : NULL;
		common->keeping.file = src->keeping.file;
	}
	if (common->keeping.enable == FALSE || common->keeping.folder == FALSE) {
		ug_free(common->folder);
		common->folder = (src->folder) ? ug_strdup(src->folder) : NULL;
		common->keeping.folder = src->keeping.folder;
	}
	if (common->keeping.enable == FALSE || common->keeping.user == FALSE) {
		ug_free(common->user);
		common->user = (src->user) ? ug_strdup(src->user) : NULL;
		common->keeping.user = src->keeping.user;
	}
	if (common->keeping.enable == FALSE || common->keeping.password == FALSE) {
		ug_free(common->password);
		common->password = (src->password) ? ug_strdup(src->password) : NULL;
		common->keeping.password = src->keeping.password;
	}
	if (common->keeping.enable == FALSE || common->keeping.format_id == FALSE) {
		ug_free(common->format_id);
		common->format_id = (src->format_id) ? ug_strdup(src->format_id) : NULL;
		common->keeping.format_id = src->keeping.format_id;
	}
	// timeout
	if (common->keeping.enable == FALSE || common->keeping.connect_timeout == FALSE) {
		common->connect_timeout = src->connect_timeout;
		common->keeping.connect_timeout = src->keeping.connect_timeout;
	}
	if (common->keeping.enable == FALSE || common->keeping.transmit_timeout == FALSE) {
		common->transmit_timeout = src->transmit_timeout;
		common->keeping.transmit_timeout = src->keeping.transmit_timeout;
	}
	// retry
	if (common->keeping.enable == FALSE || common->keeping.retry_delay == FALSE) {
		common->retry_delay = src->retry_delay;
		common->keeping.retry_delay = src->keeping.retry_delay;
	}
	if (common->keeping.enable == FALSE || common->keeping.retry_limit == FALSE) {
		common->retry_limit = src->retry_limit;
		common->keeping.retry_limit = src->keeping.retry_limit;
	}
	// max connections
	if (common->keeping.enable == FALSE || common->keeping.max_connections == FALSE) {
		common->max_connections = src->max_connections;
		common->keeping.max_connections = src->keeping.max_connections;
	}
	// speed
	if (common->keeping.enable == FALSE || common->keeping.max_upload_speed == FALSE) {
		common->max_upload_speed = src->max_upload_speed;
		common->keeping.max_upload_speed = src->keeping.max_upload_speed;
	}
	if (common->keeping.enable == FALSE || common->keeping.max_download_speed == FALSE) {
		common->max_download_speed = src->max_download_speed;
		common->keeping.max_download_speed = src->keeping.max_download_speed;
	}
	// timestamp
	if (common->keeping.enable == FALSE || common->keeping.timestamp == FALSE) {
		common->timestamp = src->timestamp;
		common->keeping.timestamp = src->keeping.timestamp;
	}

	if (common->keeping.enable == FALSE || common->keeping.debug_level == FALSE) {
		common->debug_level = src->debug_level;
		common->keeping.debug_level = src->keeping.debug_level;
	}

	if (common->keeping.enable == FALSE)
		common->keeping = src->keeping;

	return TRUE;
}

// helper functions
char* uget_name_from_uri_str(const char* uri)
{
	UgUri  uuri;

	ug_uri_init(&uuri, uri);
	return uget_name_from_uri(&uuri);
}

char*  uget_name_from_uri(UgUri* uuri)
{
	const char* filename;
	char*       name = NULL;
	int         length;

	if (uuri->scheme_len == 6 && strncmp(uuri->uri, "magnet", 6) == 0) {
		length = 0;
		filename = strstr(uuri->uri + uuri->file, "dn=");
		if (filename) {
			filename = filename + 3;
			length = strcspn(filename, "&");
			name = ug_malloc(length + 1);
			ug_decode_uri(filename, length, name);
			if (ug_utf8_get_invalid(name, NULL) != -1)
				name = ug_strndup(filename, length);
		}
	}
	if (name == NULL) {
		length = ug_uri_file(uuri, &filename);
		if (length == 0)
			name = ug_strdup(uuri->uri);
		else
			name = ug_uri_get_file(uuri);
	}
	return name;
}

// ----------------------------------------------------------------------------
// UgetProgress

static const UgEntry  UgetProgressEntry[] =
{
	{"complete", offsetof(UgetProgress, complete), UG_ENTRY_INT64,  NULL, NULL},
	{"total",    offsetof(UgetProgress, total),    UG_ENTRY_INT64,  NULL, NULL},
	{"elapsed",  offsetof(UgetProgress, elapsed),  UG_ENTRY_INT64,  NULL, NULL},
	{"uploaded", offsetof(UgetProgress, uploaded), UG_ENTRY_INT64,  NULL, NULL},
	{"percent",  offsetof(UgetProgress, percent),  UG_ENTRY_INT,    NULL, NULL},
//	{"ratio",    offsetof(UgetProgress, ratio),    UG_ENTRY_DOUBLE, NULL, NULL},
	{NULL}		// null-terminated
};

static const UgDataInfo  UgetProgressInfoStatic =
{
	.name   = "progress",
	.size   = sizeof(UgetProgress),
	.init   = (UgInitFunc)   NULL,
	.final  = (UgFinalFunc)  NULL,
	.assign = (UgAssignFunc) NULL,
	.entry  = UgetProgressEntry,
};
// extern
const UgDataInfo*  UgetProgressInfo = &UgetProgressInfoStatic;

// ----------------------------------------------------------------------------
// UgetProxy

static void  uget_proxy_final  (UgetProxy* proxy);
static int   uget_proxy_assign (UgetProxy* proxy, UgetProxy* src);

#ifdef HAVE_LIBPWMD
static const UgEntry  UgetProxyPwmdEntry[] =
{
	{"socket",      offsetof(struct UgetProxyPwmd, socket),
			UG_ENTRY_STRING, NULL, NULL},
	{"socket-args", offsetof(struct UgetProxyPwmd, socket_args),
			UG_ENTRY_STRING, NULL, NULL},
	{"file",        offsetof(struct UgetProxyPwmd, file),
			UG_ENTRY_STRING, NULL, NULL},
	{"element",     offsetof(struct UgetProxyPwmd, element),
			UG_ENTRY_STRING, NULL, NULL},
};
#endif  // End of HAVE_LIBPWMD

static const UgEntry  UgetProxyEntry[] =
{
	{"host",     offsetof(UgetProxy, host),     UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"port",     offsetof(UgetProxy, port),     UG_ENTRY_UINT,
			NULL, NULL},
	{"type",     offsetof(UgetProxy, type),     UG_ENTRY_UINT,
			NULL, NULL},
	{"user",     offsetof(UgetProxy, user),     UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"password", offsetof(UgetProxy, password), UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
#ifdef HAVE_LIBPWMD
	{"pwmd",     offsetof(UgetProxy, pwmd),
			UG_ENTRY_OBJECT, (void*) UgetProxyPwmdEntry, NULL},
#endif
	{NULL},	    // null-terminated
};

static const UgDataInfo  UgetProxyInfoStatic =
{
	.name   = "proxy",
	.size   = sizeof(UgetProxy),
	.init   = (UgInitFunc)   NULL,
	.final  = (UgFinalFunc)  uget_proxy_final,
	.assign = (UgAssignFunc) uget_proxy_assign,
	.entry  = UgetProxyEntry,
};
// extern
const UgDataInfo*  UgetProxyInfo = &UgetProxyInfoStatic;

static void  uget_proxy_final(UgetProxy* proxy)
{
	ug_free(proxy->host);
	ug_free(proxy->user);
	ug_free(proxy->password);

#ifdef HAVE_LIBPWMD
	ug_free(proxy->pwmd.socket);
	ug_free(proxy->pwmd.socket_args);
	ug_free(proxy->pwmd.file);
	ug_free(proxy->pwmd.element);
#endif	// HAVE_LIBPWMD
}

static int   uget_proxy_assign(UgetProxy* proxy, UgetProxy* src)
{
	if (proxy->keeping.enable == FALSE || proxy->keeping.host == FALSE) {
		ug_free(proxy->host);
		proxy->host = (src->host) ? ug_strdup(src->host) : NULL;
		proxy->keeping.host = src->keeping.host;
	}
	if (proxy->keeping.enable == FALSE || proxy->keeping.port == FALSE) {
		proxy->port = src->port;
		proxy->keeping.port = src->keeping.port;
	}
	if (proxy->keeping.enable == FALSE || proxy->keeping.type == FALSE) {
		proxy->type = src->type;
		proxy->keeping.type = src->keeping.type;
	}

	if (proxy->keeping.enable == FALSE || proxy->keeping.user == FALSE) {
		ug_free(proxy->user);
		proxy->user = (src->user) ? ug_strdup(src->user) : NULL;
		proxy->keeping.user = src->keeping.user;
	}
	if (proxy->keeping.enable == FALSE || proxy->keeping.password == FALSE) {
		ug_free(proxy->password);
		proxy->password = (src->password) ? ug_strdup(src->password) : NULL;
		proxy->keeping.password = src->keeping.password;
	}

#ifdef HAVE_LIBPWMD
	if (proxy->keeping.enable == FALSE || proxy->pwmd.keeping.socket == FALSE) {
		ug_free(proxy->pwmd.socket);
		proxy->pwmd.socket = (src->pwmd.socket) ? ug_strdup(src->pwmd.socket) : NULL;
		proxy->pwmd.keeping.socket = src->pwmd.keeping.socket;
	}
	if (proxy->keeping.enable == FALSE || proxy->pwmd.keeping.socket_args == FALSE) {
		ug_free(proxy->pwmd.socket_args);
		proxy->pwmd.socket_args = (src->pwmd.socket_args) ? ug_strdup(src->pwmd.socket_args) : NULL;
		proxy->pwmd.keeping.socket_args = src->pwmd.keeping.socket_args;
	}
	if (proxy->keeping.enable == FALSE || proxy->pwmd.keeping.file == FALSE) {
		ug_free(proxy->pwmd.file);
		proxy->pwmd.file = (src->pwmd.file) ? ug_strdup(src->pwmd.file) : NULL;
		proxy->pwmd.keeping.file = src->pwmd.keeping.file;
	}
	if (proxy->keeping.enable == FALSE || proxy->pwmd.keeping.element == FALSE) {
		ug_free(proxy->pwmd.element);
		proxy->pwmd.element = (src->pwmd.element) ? ug_strdup(src->pwmd.element) : NULL;
		proxy->pwmd.keeping.element = src->pwmd.keeping.element;
	}
#endif	// HAVE_LIBPWMD

	if (proxy->keeping.enable == FALSE)
		proxy->keeping = src->keeping;

	return TRUE;
}

// ---------------------------------------------------------------------------
// UgetHttp

static void  uget_http_init   (UgetHttp* http);
static void  uget_http_final  (UgetHttp* http);
static int   uget_http_assign (UgetHttp* http, UgetHttp* src);

static const UgEntry  UgetHttpEntry[] =
{
	{"user",              offsetof(UgetHttp, user),         UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"password",          offsetof(UgetHttp, password),     UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"referrer",          offsetof(UgetHttp, referrer),     UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"user-agent",        offsetof(UgetHttp, user_agent),   UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"post-data",         offsetof(UgetHttp, post_data),    UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"post-file",         offsetof(UgetHttp, post_file),    UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"cookie-data",       offsetof(UgetHttp, cookie_data),  UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"cookie-file",       offsetof(UgetHttp, cookie_file),  UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"redirection-limit", offsetof(UgetHttp, redirection_limit),UG_ENTRY_UINT,
			NULL, NULL},
	{NULL},    // null-terminated
};

static const UgDataInfo  UgetHttpInfoStatic =
{
	.name   = "http",
	.size   = sizeof(UgetHttp),
	.init   = (UgInitFunc)   uget_http_init,
	.final  = (UgFinalFunc)  uget_http_final,
	.assign = (UgAssignFunc) uget_http_assign,
	.entry  = UgetHttpEntry,
};
// extern
const UgDataInfo*  UgetHttpInfo = &UgetHttpInfoStatic;

static void  uget_http_init(UgetHttp* http)
{
	http->redirection_limit = 30;
}

static void  uget_http_final(UgetHttp* http)
{
	ug_free(http->user);
	ug_free(http->password);
	ug_free(http->referrer);
	ug_free(http->user_agent);
	ug_free(http->post_data);
	ug_free(http->post_file);
	ug_free(http->cookie_data);
	ug_free(http->cookie_file);
}

static int   uget_http_assign(UgetHttp* http, UgetHttp* src)
{
	if (http->keeping.enable == FALSE || http->keeping.user == FALSE) {
		ug_free(http->user);
		http->user = (src->user) ? ug_strdup(src->user) : NULL;
		http->keeping.user = src->keeping.user;
	}
	if (http->keeping.enable == FALSE || http->keeping.password == FALSE) {
		ug_free(http->password);
		http->password = (src->password) ? ug_strdup(src->password) : NULL;
		http->keeping.password = src->keeping.password;
	}
	if (http->keeping.enable == FALSE || http->keeping.referrer == FALSE) {
		ug_free(http->referrer);
		http->referrer = (src->referrer) ? ug_strdup(src->referrer) : NULL;
		http->keeping.referrer = src->keeping.referrer;
	}
	if (http->keeping.enable == FALSE || http->keeping.user_agent == FALSE) {
		ug_free(http->user_agent);
		http->user_agent = (src->user_agent) ? ug_strdup(src->user_agent) : NULL;
		http->keeping.user_agent = src->keeping.user_agent;
	}
	if (http->keeping.enable == FALSE || http->keeping.post_data == FALSE) {
		ug_free(http->post_data);
		http->post_data = (src->post_data) ? ug_strdup(src->post_data) : NULL;
		http->keeping.post_data = src->keeping.post_data;
	}
	if (http->keeping.enable == FALSE || http->keeping.post_file == FALSE) {
		ug_free(http->post_file);
		http->post_file = (src->post_file) ? ug_strdup(src->post_file) : NULL;
		http->keeping.post_file = src->keeping.post_file;
	}
	if (http->keeping.enable == FALSE || http->keeping.cookie_data == FALSE) {
		ug_free (http->cookie_data);
		http->cookie_data = (src->cookie_data) ? ug_strdup(src->cookie_data) : NULL;
		http->keeping.cookie_data = src->keeping.cookie_data;
	}
	if (http->keeping.enable == FALSE || http->keeping.cookie_file == FALSE) {
		ug_free(http->cookie_file);
		http->cookie_file = (src->cookie_file) ? ug_strdup(src->cookie_file) : NULL;
		http->keeping.cookie_file = src->keeping.cookie_file;
	}
	if (http->keeping.enable == FALSE || http->keeping.redirection_limit == FALSE) {
		http->redirection_limit = src->redirection_limit;
		http->keeping.redirection_limit = src->keeping.redirection_limit;
	}

	if (http->keeping.enable == FALSE)
		http->keeping = src->keeping;

	return TRUE;
}

// ---------------------------------------------------------------------------
// UgetFtp

static void  uget_ftp_final  (UgetFtp* ftp);
static int   uget_ftp_assign (UgetFtp* ftp, UgetFtp* src);

static const UgEntry  UgetFtpEntry[] =
{
	{"user",        offsetof(UgetFtp, user),        UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"password",    offsetof(UgetFtp, password),    UG_ENTRY_STRING,
			NULL, UG_ENTRY_NO_NULL},
	{"active-mode", offsetof(UgetFtp, active_mode), UG_ENTRY_INT,
			NULL, NULL},
	{NULL},	    // null-terminated
};

static const UgDataInfo  UgetFtpInfoStatic =
{
	.name   = "ftp",
	.size   = sizeof(UgetFtp),
	.init   = (UgInitFunc)   NULL,
	.final  = (UgFinalFunc)  uget_ftp_final,
	.assign = (UgAssignFunc) uget_ftp_assign,
	.entry  = UgetFtpEntry,
};
// extern
const UgDataInfo*  UgetFtpInfo = &UgetFtpInfoStatic;

static void  uget_ftp_final(UgetFtp* ftp)
{
	ug_free(ftp->user);
	ug_free(ftp->password);
}

static int   uget_ftp_assign(UgetFtp* ftp, UgetFtp* src)
{
	if (ftp->keeping.enable == FALSE || ftp->keeping.user == FALSE) {
		ug_free(ftp->user);
		ftp->user = (src->user) ? ug_strdup(src->user) : NULL;
		ftp->keeping.user = src->keeping.user;
	}
	if (ftp->keeping.enable == FALSE || ftp->keeping.password == FALSE) {
		ug_free(ftp->password);
		ftp->password = (src->password) ? ug_strdup(src->password) : NULL;
		ftp->keeping.password = src->keeping.password;
	}

	if (ftp->keeping.enable == FALSE || ftp->keeping.active_mode == FALSE) {
		ftp->active_mode = src->active_mode;
		ftp->keeping.active_mode = src->keeping.active_mode;
	}

	if (ftp->keeping.enable == FALSE)
		ftp->keeping = src->keeping;

	return TRUE;
}

// ---------------------------------------------------------------------------
// UgetLog

static void  uget_log_final(UgetLog* log);
static void  ug_json_write_list_message(UgJson* json, UgList* list);
static UgJsonError ug_json_parse_list_message(UgJson* json, const char* name,
                                              const char* value,
                                              void* list, void* none);

static const UgEntry  UgetLogEntry[] =
{
	{"added-time",     offsetof(UgetLog, added_time),     UG_ENTRY_CUSTOM,
			ug_json_parse_time_t, ug_json_write_time_t},
	{"completed-time", offsetof(UgetLog, completed_time), UG_ENTRY_CUSTOM,
			ug_json_parse_time_t, ug_json_write_time_t},
	{"messages",       offsetof(UgetLog, messages),       UG_ENTRY_ARRAY,
			ug_json_parse_list_message, ug_json_write_list_message},
	{NULL},    // null-terminated
};

static const UgDataInfo  UgetLogInfoStatic =
{
	.name   = "log",
	.size   = sizeof(UgetLog),
	.init   = (UgInitFunc)   NULL,
	.final  = (UgFinalFunc)  uget_log_final,
	.assign = (UgAssignFunc) NULL,
	.entry  = UgetLogEntry,
};
// extern
const UgDataInfo*  UgetLogInfo = &UgetLogInfoStatic;

static void  uget_log_final(UgetLog* log)
{
	ug_list_foreach(&log->messages, (UgForeachFunc) uget_event_free, NULL);
}

static UgJsonError ug_json_parse_list_message(UgJson* json, const char* name,
                                              const char* value,
                                              void* list, void* none)
{
	UgetEvent* event;

	if (json->type != UG_JSON_OBJECT)
		return UG_JSON_ERROR_TYPE_NOT_MATCH;

	event = uget_event_new(UGET_EVENT_EMPTY);
	ug_list_append(list, (UgLink*) event);
	ug_json_push(json, ug_json_parse_entry, event, (void*) UgetEventEntry);
	return UG_JSON_ERROR_NONE;
}

void  ug_json_write_list_message(UgJson* json, UgList* list)
{
	UgetEvent*  link;

	for (link = (void*)list->head;  link;  link = link->next) {
		ug_json_write_object_head(json);
		ug_json_write_entry(json, link, UgetEventEntry);
		ug_json_write_object_tail(json);
	}
}

// ----------------------------------------------------------------------------
// UgetRelation

static void uget_relation_init(UgetRelation* relation);
static void uget_relation_final(UgetRelation* relation);

// deprecated
static UgJsonError  ug_json_parse_task_priority (UgJson* json,
                                const char* name, const char* value,
                                void* prelation, void* none);

static const UgEntry  UgetRelationEntry[] =
{
	{"group",    offsetof(UgetRelation, group),    UG_ENTRY_INT,
			NULL, NULL},
	{"priority", offsetof(UgetRelation, priority), UG_ENTRY_INT,
			NULL, NULL},

	// deprecated
	{"task",     0, UG_ENTRY_CUSTOM, ug_json_parse_task_priority, NULL},
	{NULL}		// null-terminated
};

static const UgDataInfo  UgetRelationInfoStatic =
{
	.name   = "relation",
	.size   = sizeof(UgetRelation),
	.init   = (UgInitFunc)   uget_relation_init,
	.final  = (UgFinalFunc)  uget_relation_final,
	.assign = (UgAssignFunc) NULL,
	.entry  = UgetRelationEntry,
};
// extern
const UgDataInfo*  UgetRelationInfo = &UgetRelationInfoStatic;

static void uget_relation_init(UgetRelation* relation)
{
	relation->priority = UGET_PRIORITY_NORMAL;
}

static void uget_relation_final(UgetRelation* relation)
{
	ug_free(relation->task);
}

// deprecated - convert old format to new
static UgJsonError  ug_json_parse_task_priority (UgJson* json,
                                const char* name, const char* value,
                                void* prelation, void* none)
{
	UgetRelation*     relation = prelation;

	if (strcmp(name, "task") == 0 && json->type == UG_JSON_OBJECT)
		ug_json_push(json, ug_json_parse_task_priority, relation, none);
	else if (strcmp(name, "priority") == 0 && json->type == UG_JSON_NUMBER)
		relation->priority = strtol(value, NULL, 10);

	return UG_JSON_ERROR_NONE;
}

// ----------------------------------------------------------------------------
// UgetCategory

static void  uget_category_init(UgetCategory* category);
static void  uget_category_final(UgetCategory* category);
static int   uget_category_assign(UgetCategory* category, UgetCategory* src);
static void  ug_array_str_copy(UgArrayStr* dest, UgArrayStr* src);

static const UgEntry  UgetCategoryEntry[] =
{
	{"hosts",          offsetof(UgetCategory, hosts),          UG_ENTRY_ARRAY,
			ug_json_parse_array_string, ug_json_write_array_string},
	{"schemes",        offsetof(UgetCategory, schemes),        UG_ENTRY_ARRAY,
			ug_json_parse_array_string, ug_json_write_array_string},
	{"file-exts",      offsetof(UgetCategory, file_exts),      UG_ENTRY_ARRAY,
			ug_json_parse_array_string, ug_json_write_array_string},
	{"active-limit",   offsetof(UgetCategory, active_limit),   UG_ENTRY_INT,
			NULL, NULL},
	{"finished-limit", offsetof(UgetCategory, finished_limit), UG_ENTRY_INT,
			NULL, NULL},
	{"recycled-limit", offsetof(UgetCategory, recycled_limit), UG_ENTRY_INT,
			NULL, NULL},
	{NULL}		// null-terminated
};

static const UgDataInfo  UgetCategoryInfoStatic =
{
	.name   = "category",
	.size   = sizeof(UgetCategory),
	.init   = (UgInitFunc)   uget_category_init,
	.final  = (UgFinalFunc)  uget_category_final,
	.assign = (UgAssignFunc) uget_category_assign,
	.entry  = UgetCategoryEntry,
};
// extern
const UgDataInfo*  UgetCategoryInfo = &UgetCategoryInfoStatic;

static void  uget_category_init(UgetCategory* category)
{
	ug_array_init(&category->hosts, sizeof(char*), 8);
	ug_array_init(&category->schemes, sizeof(char*), 8);
	ug_array_init(&category->file_exts, sizeof(char*), 8);
	category->active_limit = 3;
	category->finished_limit = 300;
	category->recycled_limit = 300;
}

static void  uget_category_final(UgetCategory* category)
{
	ug_array_foreach_str(&category->hosts, (UgForeachFunc) ug_free, NULL);
	ug_array_foreach_str(&category->schemes, (UgForeachFunc) ug_free, NULL);
	ug_array_foreach_str(&category->file_exts, (UgForeachFunc) ug_free, NULL);
	ug_array_clear(&category->hosts);
	ug_array_clear(&category->schemes);
	ug_array_clear(&category->file_exts);
}

static int   uget_category_assign(UgetCategory* category, UgetCategory* src)
{
	category->active_limit = src->active_limit;
	category->finished_limit = src->finished_limit;
	category->recycled_limit = src->recycled_limit;

	ug_array_str_copy(&category->schemes, &src->schemes);
	ug_array_str_copy(&category->hosts, &src->hosts);
	ug_array_str_copy(&category->file_exts, &src->file_exts);

	return TRUE;
}

static void ug_array_str_copy(UgArrayStr* dest, UgArrayStr* src)
{
	int  index;

	ug_array_foreach_str(dest, (UgForeachFunc) ug_free, NULL);
	dest->length = 0;
	for (index = 0;  index < src->length;  index++)
		*(char**) ug_array_alloc(dest, 1) = ug_strdup(src->at[index]);
}

