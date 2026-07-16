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

// yt-dlp plug-in: uses an external extractor (yt-dlp) as the actual
// downloader. Used for page URLs where the required quality can only
// be delivered by downloading separate video+audio streams that must
// be muxed together — which yt-dlp handles natively.
//
// The plug-in is selected for a download when UgetCommon::format_id is
// non-NULL. That flag is set by the GUI's format picker when the user
// chooses a non-combined stream (or when auto-pick chooses a format
// that would require muxing).

#ifndef UGET_PLUGIN_YTDLP_H
#define UGET_PLUGIN_YTDLP_H

#include <UgetPlugin.h>
#include <UgetData.h>
#include <UgThread.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgetPluginYtdlp   UgetPluginYtdlp;

extern const UgetPluginInfo*     UgetPluginYtdlpInfo;

struct UgetPluginYtdlp
{
	UGET_PLUGIN_MEMBERS;
/*	// ------ UgType members ------
	const UgetPluginInfo*  info;

	// ------ UgetPlugin members ------
	UgetEvent*    events;
	UgMutex       mutex;
	int           ref_count;
 */

	UgetCommon*    common;
	UgetProgress*  progress;   // shared with UgetApp via sync()

	// subprocess state (written by worker thread, read under mutex)
	void*          proc;       // GSubprocess* (opaque in header)
	UgThread       thread;
	uint8_t        thread_running;
	uint8_t        stop_requested;

	// Progress snapshot (fed by stdout parser, drained by sync()).
	// `downloaded` and `total` are monotonic cumulative counts across all
	// streams yt-dlp downloads (video + audio + ...), not per-stream.
	int64_t        downloaded;
	int64_t        total;
	int            speed;
	int            percent;
	int            eta_seconds;
	uint8_t        postprocessing;     // TRUE while yt-dlp is muxing / remuxing

	// Internal stream-boundary tracking. yt-dlp reports progress for each
	// stream independently (each resets to 0 → stream_total). When we see
	// downloaded bytes go backwards, we know a stream boundary has been
	// crossed and we commit the previous stream's bytes.
	int64_t        stream_current;     // bytes in current stream
	int64_t        stream_committed;   // bytes from already-finished streams

	// final state
	uint8_t        completed;
	uint8_t        failed;
	char*          error_message;
	char*          final_path;  // filepath reported by yt-dlp on completion
};

#ifdef __cplusplus
}
#endif

#endif  // UGET_PLUGIN_YTDLP_H
