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

// External URL resolver: invokes a configurable extractor (default: yt-dlp)
// to convert a page URL into one or more direct download URLs.
//
// The resolver runs the extractor as a subprocess with `-j <url>` and parses
// its JSON output. It is intentionally vendor-neutral — no specific site
// names are referenced anywhere in the source.

#ifndef UGET_RESOLVER_H
#define UGET_RESOLVER_H

#include <stdint.h>
#include <UgList.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct UgetFormat            UgetFormat;
typedef struct UgetResolverResult    UgetResolverResult;

// One downloadable stream / format returned by the extractor.
struct UgetFormat
{
	UG_LINK_MEMBERS (UgetFormat, UgetFormat, self);
//	UgetFormat*  self;
//	UgetFormat*  next;
//	UgetFormat*  prev;

	char*    format_id;     // extractor-specific id
	char*    url;           // direct media URL
	char*    ext;           // container extension ("mp4", "webm", ...)
	char*    vcodec;        // video codec, or NULL/"none" for audio-only
	char*    acodec;        // audio codec, or NULL/"none" for video-only
	int      width;
	int      height;
	int      fps;
	int      tbr;           // total bitrate (kbps), 0 if unknown
	int64_t  filesize;      // bytes, 0 if unknown
};

// Result of a resolution attempt.
struct UgetResolverResult
{
	char*    title;         // page / video title
	char*    extractor;     // generic extractor identifier returned by tool
	UgList   formats;       // list of UgetFormat
	char*    error;         // NULL on success, error message otherwise
};

// Callback signature. The result is owned by the callee — call
// uget_resolver_result_free() when done.
typedef void (*UgetResolverCallback)(UgetResolverResult* result, void* user_data);

// Probe the system for an external resolver. Returns 1 if found, 0 otherwise.
// Caches the result; call uget_resolver_invalidate() to re-probe.
int          uget_resolver_available (void);
void         uget_resolver_invalidate (void);

// Override the resolver executable path (NULL = look up "yt-dlp" in PATH).
// Used by settings.
void         uget_resolver_set_path (const char* path);
const char*  uget_resolver_get_path (void);

// Heuristic: returns 1 if the URL looks like something the resolver should
// handle (page URL, no obvious file extension). Returns 0 for direct file
// URLs that should go straight to the HTTP downloader.
int          uget_resolver_should_handle (const char* url);

// Spawn the extractor on the given URL and call back with the result.
// The callback always runs on the same GLib main context as the caller.
// Caller retains ownership of `url` (it is copied internally).
void         uget_resolver_resolve_async (const char* url,
                                          UgetResolverCallback callback,
                                          void* user_data);

// Free a result returned to a callback.
void         uget_resolver_result_free (UgetResolverResult* result);

// Free a single format (used internally; exposed for convenience).
void         uget_format_free (UgetFormat* format);

#ifdef __cplusplus
}
#endif

#endif  // UGET_RESOLVER_H
