/*
 *
 *   Copyright (C) 2026 by Ozgur As
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <stdlib.h>

#include <gio/gio.h>
#include <glib.h>

#include <UgString.h>
#include <UgUri.h>
#include <UgJson.h>
#include <UgEntry.h>
#include <UgetResolver.h>

#define DEFAULT_RESOLVER  "yt-dlp"

// ----------------------------------------------------------------------------
// Path probing

static char* resolver_path = NULL;       // user-set override (NULL = auto)
static char* resolver_resolved = NULL;   // cached absolute path or NULL
static int   resolver_probed = 0;

static void probe_resolver (void)
{
	const char* exe;

	g_free (resolver_resolved);
	resolver_resolved = NULL;
	resolver_probed = 1;

	exe = (resolver_path && resolver_path[0]) ? resolver_path : DEFAULT_RESOLVER;

	// If user gave an absolute path, accept it as-is if executable.
	if (g_path_is_absolute (exe)) {
		if (g_file_test (exe, G_FILE_TEST_IS_EXECUTABLE))
			resolver_resolved = g_strdup (exe);
		return;
	}

	// Otherwise look it up in PATH.
	resolver_resolved = g_find_program_in_path (exe);
}

int  uget_resolver_available (void)
{
	if (!resolver_probed)
		probe_resolver ();
	return resolver_resolved != NULL;
}

void uget_resolver_invalidate (void)
{
	resolver_probed = 0;
	g_free (resolver_resolved);
	resolver_resolved = NULL;
}

void uget_resolver_set_path (const char* path)
{
	g_free (resolver_path);
	resolver_path = (path && path[0]) ? g_strdup (path) : NULL;
	uget_resolver_invalidate ();
}

const char* uget_resolver_get_path (void)
{
	return resolver_path;
}

// ----------------------------------------------------------------------------
// URL classification

// Common direct-file extensions: if the URL ends in one of these, the
// resolver should NOT be invoked — it's already a direct download.
static const char* direct_file_extensions[] = {
	"zip", "rar", "7z", "tar", "gz", "bz2", "xz", "tgz", "tbz2",
	"iso", "img", "dmg", "msi", "exe", "deb", "rpm", "pkg", "apk", "appimage",
	"pdf", "epub", "mobi", "azw3",
	"jpg", "jpeg", "png", "gif", "webp", "svg", "ico", "bmp",
	"mp3", "flac", "wav", "ogg", "aac", "m4a", "opus",
	"mp4", "mkv", "webm", "avi", "mov", "wmv", "m4v", "flv", "ts", "m3u8", "mpd",
	"txt", "log", "csv", "json", "xml", "html", "htm",
	"doc", "docx", "xls", "xlsx", "ppt", "pptx", "odt", "ods",
	"torrent", "magnet",
	NULL
};

int  uget_resolver_should_handle (const char* url)
{
	UgUri        uuri;
	const char*  ext;
	int          ext_len;
	int          i;

	if (url == NULL || url[0] == 0)
		return 0;

	// Only HTTP(S) URLs are candidates.
	if (g_ascii_strncasecmp (url, "http://", 7) != 0 &&
	    g_ascii_strncasecmp (url, "https://", 8) != 0)
		return 0;

	ug_uri_init (&uuri, url);
	ext_len = ug_uri_part_file_ext (&uuri, &ext);
	if (ext_len <= 0) {
		// No extension — let the resolver have a look.
		return 1;
	}

	// Compare against the direct-file allowlist (case insensitive).
	for (i = 0; direct_file_extensions[i] != NULL; i++) {
		int len = (int) strlen (direct_file_extensions[i]);
		if (len == ext_len &&
		    g_ascii_strncasecmp (direct_file_extensions[i], ext, ext_len) == 0)
			return 0;
	}

	// Unknown extension — try the resolver.
	return 1;
}

// ----------------------------------------------------------------------------
// Format / result lifetime

static UgetFormat* format_new (void)
{
	UgetFormat* f = g_new0 (UgetFormat, 1);
	return f;
}

void uget_format_free (UgetFormat* f)
{
	if (f == NULL)
		return;
	g_free (f->format_id);
	g_free (f->url);
	g_free (f->ext);
	g_free (f->vcodec);
	g_free (f->acodec);
	g_free (f);
}

void uget_resolver_result_free (UgetResolverResult* result)
{
	UgetFormat* f;
	UgetFormat* next;

	if (result == NULL)
		return;

	g_free (result->title);
	g_free (result->extractor);
	g_free (result->error);

	for (f = (UgetFormat*) result->formats.head; f; f = next) {
		next = f->next;
		uget_format_free (f);
	}

	g_free (result);
}

// ----------------------------------------------------------------------------
// JSON parsing
//
// Schema (per yt-dlp -j output, simplified):
// {
//   "title":      "...",
//   "extractor":  "...",
//   "url":        "..."        // top-level direct URL (single-stream extractors)
//   "ext":        "...",
//   "formats": [
//     {
//       "format_id": "137",
//       "url":       "https://...",
//       "ext":       "mp4",
//       "vcodec":    "avc1.640028",
//       "acodec":    "none",
//       "width":     1920,
//       "height":    1080,
//       "fps":       30,
//       "tbr":       4500,
//       "filesize":  12345678
//     },
//     ...
//   ]
// }

// Fallback for the approximate size. The extractor reports an exact "filesize"
// only for some streams; progressive/combined streams often carry the size in
// "filesize_approx" instead, with "filesize" null. Write it only when the exact
// size was absent so the picker shows a size for those rows too. Order-safe:
// the guard means an exact "filesize" (which precedes "filesize_approx") wins
// regardless of which key is seen first.
static UgJsonError parse_filesize_approx (UgJson* json,
                                          const char* name, const char* value,
                                          void* dest, void* data)
{
	(void) name;
	(void) data;

	if (json->type == UG_JSON_NUMBER && *(int64_t*) dest == 0)
		*(int64_t*) dest = strtoll (value, NULL, 10);
	return UG_JSON_ERROR_NONE;
}

static const UgEntry  format_entry[] =
{
	{"format_id",       offsetof (UgetFormat, format_id), UG_ENTRY_STRING, NULL, NULL},
	{"url",             offsetof (UgetFormat, url),       UG_ENTRY_STRING, NULL, NULL},
	{"ext",             offsetof (UgetFormat, ext),       UG_ENTRY_STRING, NULL, NULL},
	{"vcodec",          offsetof (UgetFormat, vcodec),    UG_ENTRY_STRING, NULL, NULL},
	{"acodec",          offsetof (UgetFormat, acodec),    UG_ENTRY_STRING, NULL, NULL},
	{"width",           offsetof (UgetFormat, width),     UG_ENTRY_INT,    NULL, NULL},
	{"height",          offsetof (UgetFormat, height),    UG_ENTRY_INT,    NULL, NULL},
	{"fps",             offsetof (UgetFormat, fps),       UG_ENTRY_INT,    NULL, NULL},
	{"tbr",             offsetof (UgetFormat, tbr),       UG_ENTRY_INT,    NULL, NULL},
	{"filesize",        offsetof (UgetFormat, filesize),  UG_ENTRY_INT64,  NULL, NULL},
	{"filesize_approx", offsetof (UgetFormat, filesize),  UG_ENTRY_CUSTOM, (void*) parse_filesize_approx, NULL},
	{NULL}
};

static UgJsonError parse_format_object (UgJson* json,
                                        const char* name, const char* value,
                                        void* result_ptr, void* data)
{
	UgetResolverResult* result = result_ptr;
	UgetFormat* f;

	(void) name;
	(void) value;
	(void) data;

	if (json->type != UG_JSON_OBJECT)
		return UG_JSON_ERROR_TYPE_NOT_MATCH;

	f = format_new ();
	ug_list_append ((UgList*) &result->formats, (UgLink*) f);
	ug_json_push (json, ug_json_parse_entry, f, (void*) format_entry);
	return UG_JSON_ERROR_NONE;
}

static UgJsonError parse_formats_array (UgJson* json,
                                        const char* name, const char* value,
                                        void* result_ptr, void* data)
{
	(void) name;
	(void) value;
	(void) data;

	if (json->type != UG_JSON_ARRAY)
		return UG_JSON_ERROR_TYPE_NOT_MATCH;

	ug_json_push (json, parse_format_object, result_ptr, NULL);
	return UG_JSON_ERROR_NONE;
}

static const UgEntry  toplevel_entry[] =
{
	{"title",       offsetof (UgetResolverResult, title),       UG_ENTRY_STRING, NULL, NULL},
	{"extractor",   offsetof (UgetResolverResult, extractor),   UG_ENTRY_STRING, NULL, NULL},
	{"formats",     0, UG_ENTRY_CUSTOM, (void*) parse_formats_array, NULL},
	{NULL}
};

static UgetResolverResult* parse_json (const char* text, size_t len)
{
	UgetResolverResult* result;
	UgJson  json;
	UgJsonError err;

	result = g_new0 (UgetResolverResult, 1);
	ug_list_init ((UgList*) &result->formats);

	ug_json_init (&json);
	ug_json_begin_parse (&json);
	ug_json_push (&json, ug_json_parse_entry, result, (void*) toplevel_entry);
	ug_json_push (&json, ug_json_parse_object, NULL, NULL);
	ug_json_parse (&json, text, (int) len);
	err = ug_json_end_parse (&json);
	ug_json_final (&json);

	if (err < 0) {
		result->error = g_strdup_printf ("Failed to parse extractor output (code %d)", err);
	}
	else if (result->formats.size == 0) {
		// No formats[] array — extractor returned nothing useful.
		result->error = g_strdup ("Extractor did not return any formats");
	}

	return result;
}

// ----------------------------------------------------------------------------
// Async subprocess

typedef struct {
	UgetResolverCallback  callback;
	void*                 user_data;
	char*                 url;       // copy
	GMainContext*         context;   // for callback dispatch
} ResolveTask;

static void task_free (ResolveTask* task)
{
	if (task == NULL)
		return;
	g_free (task->url);
	if (task->context)
		g_main_context_unref (task->context);
	g_free (task);
}

static void on_communicate_finished (GObject* source, GAsyncResult* res, gpointer user_data)
{
	ResolveTask* task = user_data;
	GSubprocess* proc = G_SUBPROCESS (source);
	GBytes*      stdout_bytes = NULL;
	GError*      error = NULL;
	UgetResolverResult* result = NULL;
	const char*  data;
	gsize        len;

	g_subprocess_communicate_finish (proc, res, &stdout_bytes, NULL, &error);

	if (error) {
		result = g_new0 (UgetResolverResult, 1);
		ug_list_init ((UgList*) &result->formats);
		result->error = g_strdup (error->message);
		g_error_free (error);
	}
	else if (!g_subprocess_get_successful (proc)) {
		result = g_new0 (UgetResolverResult, 1);
		ug_list_init ((UgList*) &result->formats);
		result->error = g_strdup_printf (
				"Extractor exited with status %d",
				g_subprocess_get_exit_status (proc));
	}
	else if (stdout_bytes) {
		data = g_bytes_get_data (stdout_bytes, &len);
		result = parse_json (data, len);
	}
	else {
		result = g_new0 (UgetResolverResult, 1);
		ug_list_init ((UgList*) &result->formats);
		result->error = g_strdup ("Extractor returned no output");
	}

	if (stdout_bytes)
		g_bytes_unref (stdout_bytes);
	g_object_unref (proc);

	if (task->callback)
		task->callback (result, task->user_data);
	else
		uget_resolver_result_free (result);

	task_free (task);
}

void uget_resolver_resolve_async (const char* url,
                                  UgetResolverCallback callback,
                                  void* user_data)
{
	ResolveTask*  task;
	GSubprocess*  proc;
	GError*       error = NULL;
	const char*   exe;

	if (url == NULL || url[0] == 0)
		return;

	if (!uget_resolver_available ()) {
		if (callback) {
			UgetResolverResult* result = g_new0 (UgetResolverResult, 1);
			ug_list_init ((UgList*) &result->formats);
			result->error = g_strdup ("External URL extractor is not installed");
			callback (result, user_data);
		}
		return;
	}

	exe = resolver_resolved;

	proc = g_subprocess_new (
			G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_SILENCE,
			&error,
			exe,
			"-j",            // dump single JSON line per video
			"--no-warnings",
			"--no-playlist", // for now, only resolve one item
			"--",
			url,
			NULL);

	if (proc == NULL) {
		if (callback) {
			UgetResolverResult* result = g_new0 (UgetResolverResult, 1);
			ug_list_init ((UgList*) &result->formats);
			result->error = g_strdup (error ? error->message : "Failed to spawn extractor");
			callback (result, user_data);
		}
		if (error)
			g_error_free (error);
		return;
	}

	task = g_new0 (ResolveTask, 1);
	task->callback = callback;
	task->user_data = user_data;
	task->url = g_strdup (url);
	task->context = g_main_context_ref_thread_default ();

	g_subprocess_communicate_async (proc, NULL, NULL,
			on_communicate_finished, task);
}
