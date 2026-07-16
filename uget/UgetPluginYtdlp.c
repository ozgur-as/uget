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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <gio/gio.h>
#include <glib.h>

#include <UgString.h>
#include <UgetData.h>
#include <UgetEvent.h>
#include <UgetResolver.h>
#include <UgetPluginYtdlp.h>

// ----------------------------------------------------------------------------
// UgetPluginInfo

static void plugin_init  (UgetPluginYtdlp* plugin);
static void plugin_final (UgetPluginYtdlp* plugin);
static int  plugin_ctrl  (UgetPluginYtdlp* plugin, int code, void* data);
static int  plugin_accept(UgetPluginYtdlp* plugin, UgInfo* node_info);
static int  plugin_sync  (UgetPluginYtdlp* plugin, UgInfo* node_info);
static UgetResult  global_set(int code, void* parameter);
static UgetResult  global_get(int code, void* parameter);

static const UgetPluginInfo UgetPluginYtdlpInfoStatic =
{
	.name       = "ytdlp",
	.size       = sizeof(UgetPluginYtdlp),
	.init       = (UgInitFunc)         plugin_init,
	.final      = (UgFinalFunc)        plugin_final,
	.accept     = (UgetPluginSyncFunc) plugin_accept,
	.sync       = (UgetPluginSyncFunc) plugin_sync,
	.ctrl       = (UgetPluginCtrlFunc) plugin_ctrl,
	.hosts      = NULL,
	.schemes    = NULL,
	.file_exts  = NULL,
	.global_set = (UgetPluginGlobalFunc) global_set,
	.global_get = (UgetPluginGlobalFunc) global_get,
};
const UgetPluginInfo* UgetPluginYtdlpInfo = &UgetPluginYtdlpInfoStatic;

// ----------------------------------------------------------------------------
// global

static struct {
	int  initialized;
	int  ref_count;
} global = {0, 0};

static UgetResult  global_init(void)
{
	if (global.initialized == FALSE)
		global.initialized = TRUE;
	global.ref_count++;
	return UGET_RESULT_OK;
}

static void  global_ref(void)   { global.ref_count++; }
static void  global_unref(void)
{
	if (!global.initialized)
		return;
	if (--global.ref_count == 0)
		global.initialized = FALSE;
}

static UgetResult  global_set(int option, void* parameter)
{
	switch (option) {
	case UGET_PLUGIN_GLOBAL_INIT:
		if (parameter)
			return global_init();
		else
			global_unref();
		return UGET_RESULT_OK;
	default:
		return UGET_RESULT_UNSUPPORT;
	}
}

static UgetResult  global_get(int option, void* parameter)
{
	switch (option) {
	case UGET_PLUGIN_GLOBAL_INIT:
		if (parameter)
			*(int*)parameter = global.initialized;
		return UGET_RESULT_OK;
	default:
		return UGET_RESULT_UNSUPPORT;
	}
}

// ----------------------------------------------------------------------------
// plugin lifecycle

static void plugin_init (UgetPluginYtdlp* plugin)
{
	if (global.initialized == FALSE)
		global_init();
	else
		global_ref();
}

static void plugin_final (UgetPluginYtdlp* plugin)
{
	if (plugin->common)
		ug_data_free(plugin->common);
	if (plugin->progress)
		ug_data_free(plugin->progress);
	g_free(plugin->error_message);
	g_free(plugin->final_path);
	if (plugin->proc)
		g_object_unref((GObject*) plugin->proc);
	global_unref();
}

// ----------------------------------------------------------------------------
// plugin_accept

static int  plugin_accept(UgetPluginYtdlp* plugin, UgInfo* node_info)
{
	UgetCommon* common;

	common = ug_info_get(node_info, UgetCommonInfo);
	if (common == NULL || common->uri == NULL || common->uri[0] == 0)
		return FALSE;
	if (common->format_id == NULL || common->format_id[0] == 0)
		return FALSE;

	plugin->common = (UgetCommon*) ug_data_copy((UgData*) common);
	return TRUE;
}

// ----------------------------------------------------------------------------
// subprocess worker

// Parse a '|' separated field from progress line. Advances `*pp` past the
// field and returns TRUE if it read a numeric value (not "NA").
static int parse_field(const char** pp, char* buf, size_t buflen)
{
	const char* start = *pp;
	const char* p = start;
	int len;
	while (*p && *p != '|' && *p != '\n' && *p != '\r')
		p++;
	len = (int)(p - start);
	*pp = (*p == '|') ? p + 1 : p;
	if (len <= 0 || (size_t) len >= buflen)
		return 0;
	memcpy(buf, start, len);
	buf[len] = 0;
	return strcmp(buf, "NA") != 0;
}

// Parse yt-dlp progress lines. We use --newline with a custom template so
// progress arrives on its own line in a known format:
//   [uget-progress]<downloaded>|<total>|<speed>|<percent>|<eta>
// Fields can be "NA" when unknown.
//
// yt-dlp reports progress per-stream. When downloading separate video+audio
// streams for later muxing, the downloaded byte count restarts at 0 for
// each new stream. We detect that boundary and keep a cumulative count so
// the UI sees a monotonically increasing progress bar.
static void parse_progress_line(UgetPluginYtdlp* plugin, const char* line)
{
	const char* p;
	char        buf[64];
	int64_t     stream_downloaded = -1;
	int64_t     stream_total = -1;
	int         speed = -1;
	int         percent = -1;
	int         eta = -1;

	if (strncmp(line, "[uget-progress]", 15) != 0)
		return;
	p = line + 15;

	if (parse_field(&p, buf, sizeof buf))
		stream_downloaded = g_ascii_strtoll(buf, NULL, 10);
	if (parse_field(&p, buf, sizeof buf))
		stream_total = g_ascii_strtoll(buf, NULL, 10);
	if (parse_field(&p, buf, sizeof buf))
		speed = (int) g_ascii_strtoll(buf, NULL, 10);
	if (parse_field(&p, buf, sizeof buf))
		percent = (int) (g_ascii_strtod(buf, NULL) + 0.5);
	if (parse_field(&p, buf, sizeof buf))
		eta = (int) g_ascii_strtoll(buf, NULL, 10);

	uget_plugin_lock(plugin);

	if (stream_downloaded >= 0) {
		// Stream boundary: byte count went backwards (or jumped a lot).
		// Commit the previous stream and start counting the new one.
		if (stream_downloaded < plugin->stream_current) {
			plugin->stream_committed += plugin->stream_current;
		}
		plugin->stream_current = stream_downloaded;
		plugin->downloaded = plugin->stream_committed + stream_downloaded;
	}

	if (stream_total >= 0) {
		// Monotonic total: the cumulative total can only grow. We assume
		// the current stream's total is at least as big as what we've seen,
		// and that past streams contributed their committed bytes.
		int64_t est_total = plugin->stream_committed + stream_total;
		if (est_total > plugin->total)
			plugin->total = est_total;
		// If `downloaded` somehow exceeds `total` (unlikely), bump total.
		if (plugin->downloaded > plugin->total)
			plugin->total = plugin->downloaded;
	}

	if (speed >= 0)
		plugin->speed = speed;

	if (plugin->total > 0)
		plugin->percent = (int) (plugin->downloaded * 100 / plugin->total);
	else if (percent >= 0)
		plugin->percent = percent;

	if (eta >= 0)
		plugin->eta_seconds = eta;

	uget_plugin_unlock(plugin);
}

// After move, yt-dlp prints the final filepath on its own line (via --print).
//   [uget-filepath]<absolute path>
static void parse_filepath_line(UgetPluginYtdlp* plugin, const char* line)
{
	char* path;

	if (strncmp(line, "[uget-filepath]", 15) != 0)
		return;
	path = g_strdup(line + 15);
	// Strip trailing newline
	{
		int n = (int) strlen(path);
		while (n > 0 && (path[n-1] == '\n' || path[n-1] == '\r'))
			path[--n] = 0;
	}
	uget_plugin_lock(plugin);
	g_free(plugin->final_path);
	plugin->final_path = path;
	uget_plugin_unlock(plugin);
}

// yt-dlp emits bracketed stage markers like "[Merger] Merging formats..."
// or "[ExtractAudio] Destination: ..." once downloads complete and it
// begins post-processing (ffmpeg mux, audio extraction, format convert).
// We treat any of these as "postprocessing" so the UI can stop showing
// speed/ETA and freeze percent at 99% until the process actually exits.
static int is_postprocessing_marker(const char* line)
{
	static const char* const tags[] = {
		"[Merger]", "[ExtractAudio]", "[VideoConvertor]", "[VideoRemuxer]",
		"[FixupM4a]", "[FixupM3u8]", "[FixupTimestamp]", "[Fixup",
		"[Metadata]", "[EmbedThumbnail]", "[EmbedSubtitle]",
		"[SplitChapters]", "[SponsorBlock]", "[ModifyChapters]",
		NULL
	};
	int i;
	for (i = 0; tags[i]; i++) {
		size_t len = strlen(tags[i]);
		if (strncmp(line, tags[i], len) == 0)
			return 1;
	}
	return 0;
}

static void plugin_handle_line(UgetPluginYtdlp* plugin, const char* line)
{
	if (line == NULL || line[0] == 0)
		return;
	if (strncmp(line, "[uget-progress]", 15) == 0) {
		parse_progress_line(plugin, line);
	}
	else if (strncmp(line, "[uget-filepath]", 15) == 0) {
		parse_filepath_line(plugin, line);
	}
	else if (is_postprocessing_marker(line)) {
		uget_plugin_lock(plugin);
		plugin->postprocessing = 1;
		plugin->speed = 0;
		plugin->eta_seconds = 0;
		uget_plugin_unlock(plugin);
	}
	// all other output is ignored
}

static UgThreadResult plugin_thread(UgetPluginYtdlp* plugin)
{
	GSubprocess*  proc;
	GError*       error = NULL;
	const char*   exe;
	GPtrArray*    argv;
	GInputStream* stdout_stream;
	GDataInputStream* reader;
	char*         line;
	gsize         line_len;

	exe = uget_resolver_get_path();
	if (exe == NULL || exe[0] == 0)
		exe = "yt-dlp";

	// Build argv.
	argv = g_ptr_array_new();
	g_ptr_array_add(argv, (gpointer) exe);
	g_ptr_array_add(argv, (gpointer) "--newline");
	g_ptr_array_add(argv, (gpointer) "--no-warnings");
	g_ptr_array_add(argv, (gpointer) "--no-playlist");
	g_ptr_array_add(argv, (gpointer) "--progress");
	g_ptr_array_add(argv, (gpointer) "--progress-template");
	g_ptr_array_add(argv, (gpointer)
			"[uget-progress]%(progress.downloaded_bytes)s|%(progress.total_bytes)s|"
			"%(progress.speed)s|%(progress._percent_str)s|%(progress.eta)s");
	g_ptr_array_add(argv, (gpointer) "--print");
	g_ptr_array_add(argv, (gpointer) "after_move:[uget-filepath]%(filepath)s");
	g_ptr_array_add(argv, (gpointer) "-f");
	g_ptr_array_add(argv, (gpointer) plugin->common->format_id);
	// Output template: prefer common->file (seeded from the resolver's
	// title when the format was picked) so the on-disk name matches the
	// UI. If common->file is empty, fall back to yt-dlp's own title
	// template.
	{
		static char out_template[4096];
		const char* folder = (plugin->common->folder && plugin->common->folder[0])
				? plugin->common->folder : ".";
		if (plugin->common->file && plugin->common->file[0]) {
			// Strip an existing extension — yt-dlp will add the real one.
			const char* dot = strrchr(plugin->common->file, '.');
			int stem_len = dot ? (int)(dot - plugin->common->file)
			                   : (int) strlen(plugin->common->file);
			g_snprintf(out_template, sizeof out_template,
					"%s/%.*s.%%(ext)s", folder, stem_len, plugin->common->file);
		}
		else {
			g_snprintf(out_template, sizeof out_template,
					"%s/%%(title).200B [%%(id)s].%%(ext)s", folder);
		}
		g_ptr_array_add(argv, (gpointer) "-o");
		g_ptr_array_add(argv, (gpointer) out_template);
	}
	g_ptr_array_add(argv, (gpointer) "--");
	g_ptr_array_add(argv, (gpointer) plugin->common->uri);
	g_ptr_array_add(argv, NULL);

	proc = g_subprocess_newv((const gchar* const*) argv->pdata,
			G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_MERGE,
			&error);
	g_ptr_array_free(argv, TRUE);

	if (proc == NULL) {
		uget_plugin_lock(plugin);
		plugin->failed = TRUE;
		plugin->error_message = g_strdup(error ? error->message : "spawn failed");
		plugin->thread_running = FALSE;
		uget_plugin_unlock(plugin);
		if (error) g_error_free(error);
		uget_plugin_unref((UgetPlugin*) plugin);
		return (UgThreadResult) 0;
	}

	uget_plugin_lock(plugin);
	plugin->proc = proc;
	uget_plugin_unlock(plugin);

	// Read stdout line by line.
	stdout_stream = g_subprocess_get_stdout_pipe(proc);
	reader = g_data_input_stream_new(stdout_stream);

	while ((line = g_data_input_stream_read_line_utf8(reader, &line_len, NULL, NULL)) != NULL) {
		plugin_handle_line(plugin, line);
		g_free(line);

		uget_plugin_lock(plugin);
		int stop = plugin->stop_requested;
		uget_plugin_unlock(plugin);
		if (stop) {
			g_subprocess_send_signal(proc, 15 /* SIGTERM */);
			break;
		}
	}

	g_object_unref(reader);

	// Wait for completion
	g_subprocess_wait(proc, NULL, NULL);

	uget_plugin_lock(plugin);
	if (g_subprocess_get_if_exited(proc) && g_subprocess_get_exit_status(proc) == 0) {
		plugin->completed = TRUE;
		plugin->percent = 100;
		if (plugin->total > 0)
			plugin->downloaded = plugin->total;
	}
	else {
		plugin->failed = TRUE;
		if (plugin->error_message == NULL)
			plugin->error_message = g_strdup_printf(
					"Extractor exited with status %d",
					g_subprocess_get_exit_status(proc));
	}
	plugin->thread_running = FALSE;
	uget_plugin_unlock(plugin);

	uget_plugin_unref((UgetPlugin*) plugin);
	return (UgThreadResult) 0;
}

// ----------------------------------------------------------------------------
// plugin_ctrl

static int plugin_start(UgetPluginYtdlp* plugin)
{
	UgThread thread;

	if (plugin->common == NULL)
		return FALSE;

	plugin->thread_running = TRUE;
	plugin->stop_requested = FALSE;
	plugin->completed = FALSE;
	plugin->failed = FALSE;

	uget_plugin_ref((UgetPlugin*) plugin);
	if (ug_thread_create(&thread, (UgThreadFunc) plugin_thread, plugin) != 0) {
		plugin->thread_running = FALSE;
		uget_plugin_post((UgetPlugin*) plugin,
				uget_event_new_error(UGET_EVENT_ERROR_THREAD_CREATE_FAILED, NULL));
		uget_plugin_unref((UgetPlugin*) plugin);
		return FALSE;
	}
	ug_thread_unjoin(&thread);
	return TRUE;
}

static int plugin_ctrl(UgetPluginYtdlp* plugin, int code, void* data)
{
	switch (code) {
	case UGET_PLUGIN_CTRL_START:
		return plugin_start(plugin);

	case UGET_PLUGIN_CTRL_STOP:
		uget_plugin_lock(plugin);
		plugin->stop_requested = TRUE;
		if (plugin->proc)
			g_subprocess_send_signal((GSubprocess*) plugin->proc, 15);
		uget_plugin_unlock(plugin);
		return TRUE;

	case UGET_PLUGIN_CTRL_SPEED:
		// Speed limiting is not supported — yt-dlp manages its own rate.
		return FALSE;

	case UGET_PLUGIN_GET_STATE:
		*(int*)data = plugin->thread_running;
		return TRUE;

	default:
		break;
	}
	return FALSE;
}

// ----------------------------------------------------------------------------
// plugin_sync

static int plugin_sync(UgetPluginYtdlp* plugin, UgInfo* node_info)
{
	UgetCommon*    common;
	UgetProgress*  progress;
	// Local snapshot populated under lock; used without lock afterwards.
	int     running, completed, failed;
	int     had_error = FALSE;
	int64_t downloaded, total;
	int     speed, percent, eta;
	char*   final_path = NULL;    // stolen from plugin, freed here
	char*   error_message = NULL; // stolen from plugin, freed here

	if (plugin->common == NULL)
		return FALSE;

	uget_plugin_lock(plugin);
	running     = plugin->thread_running;
	completed   = plugin->completed;
	failed      = plugin->failed;
	downloaded  = plugin->downloaded;
	total       = plugin->total;
	speed       = plugin->speed;
	percent     = plugin->percent;
	eta         = plugin->eta_seconds;
	if (completed) {
		// Steal strings so we can fire one-shot events without holding lock.
		final_path = plugin->final_path;
		plugin->final_path = NULL;
		plugin->completed = FALSE;
	}
	if (failed && plugin->error_message) {
		error_message = plugin->error_message;
		plugin->error_message = NULL;
		plugin->failed = FALSE;
	}
	uget_plugin_unlock(plugin);

	common = ug_info_realloc(node_info, UgetCommonInfo);
	progress = ug_info_realloc(node_info, UgetProgressInfo);

	// Cap percent at 99 until the subprocess actually exits. yt-dlp's
	// download phase finishes at 100% per-stream, but after that it still
	// needs to mux / remux / fixup — which can take several seconds on
	// large files. If we let the bar hit 100% during download the user
	// would see it freeze there until completion; instead we hold at 99.
	if (!completed && percent >= 100)
		percent = 99;

	progress->complete       = downloaded;
	progress->total          = total;
	progress->download_speed = speed;
	progress->upload_speed   = 0;
	progress->percent        = percent;
	progress->left           = eta;

	// Copy resolved filename when download finishes.
	if (final_path) {
		const char* slash = strrchr(final_path, '/');
		if (slash && slash[1]) {
			if (common->name == NULL || strcmp(slash + 1, common->name)) {
				ug_free(common->name);
				common->name = ug_strdup(slash + 1);
				uget_plugin_post((UgetPlugin*) plugin,
						uget_event_new(UGET_EVENT_NAME));
			}
			ug_free(common->file);
			common->file = ug_strdup(slash + 1);
		}
		g_free(final_path);
	}

	// Surface errors.
	if (error_message) {
		uget_plugin_post((UgetPlugin*) plugin,
				uget_event_new_error(UGET_EVENT_ERROR_CUSTOM, error_message));
		g_free(error_message);
		had_error = TRUE;
	}

	// Completion event (after the filename has been applied).
	if (completed) {
		uget_plugin_post((UgetPlugin*) plugin,
				uget_event_new(UGET_EVENT_COMPLETED));
	}

	return running || completed || had_error;
}
