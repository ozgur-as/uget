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

#include <strings.h>
#include <UgString.h>
#include <UgUtil.h>
#include <UgStdio.h>
#include <UgetCurl.h>

#ifdef HAVE_LIBPWMD
#include "pwmd.h"
#endif  // HAVE_LIBPWMD

#define PROGRESS_COUNT_LIMIT    2
#define LOW_SPEED_LIMIT         128
#define LOW_SPEED_TIME          60

enum SchemeType
{
	SCHEME_UNKNOWN,
	SCHEME_HTTP,
	SCHEME_FTP,
};

// curl_easy_perform () -> progress callback -> connect
//                      -> progress callback -> header callback
//                      -> write callback -> progress callback
//
static size_t uget_curl_header_http0 (char *buffer, size_t size,
                                      size_t nmemb, UgetCurl* ugcurl);
static size_t uget_curl_header_http (char *buffer, size_t size,
                                     size_t nmemb, UgetCurl* ugcurl);
static size_t uget_curl_header_ftp0 (char *buffer, size_t size,
                                     size_t nmemb, UgetCurl* ugcurl);
static size_t uget_curl_output_none (char *buffer, size_t size,
                                     size_t nmemb, void* data);
static size_t uget_curl_output_default (char *buffer, size_t size,
                                        size_t nmemb, void* data);
static int    uget_curl_progress (UgetCurl* ugcurl,
                                  curl_off_t dltotal, curl_off_t dlnow,
                                  curl_off_t ultotal, curl_off_t ulnow);
#ifdef HAVE_LIBPWMD
static int  uget_curl_set_proxy_pwmd (UgetCurl* ugcurl, UgetProxy *proxy);
#endif

UgetCurl*  uget_curl_new (void)
{
	UgetCurl*  ugcurl;

	ugcurl = ug_malloc0 (sizeof (UgetCurl));
	ugcurl->self = ugcurl;
	ugcurl->curl = curl_easy_init ();
	curl_easy_setopt (ugcurl->curl, CURLOPT_ERRORBUFFER, ugcurl->error_string);
//	ugcurl->ftp_command = NULL;
//	ugcurl->ftp_command = curl_slist_append (ugcurl->ftp_command, "REST 10");

	return ugcurl;
}

void  uget_curl_free (UgetCurl* ugcurl)
{
	if (ugcurl->curl)
		curl_easy_cleanup (ugcurl->curl);
	if (ugcurl->file.output)
		ug_fclose (ugcurl->file.output);
	if (ugcurl->file.post)
		ug_fclose (ugcurl->file.post);
	if (ugcurl->event)
		uget_event_free (ugcurl->event);
	ug_free (ugcurl->header.uri);
	ug_free (ugcurl->header.filename);
	ug_free (ugcurl);
}

static UgThreadResult  uget_curl_thread (UgetCurl* ugcurl)
{
	char*     tempstr;
	CURLcode  code;

	// perform
	do {
		ugcurl->restart = FALSE;
		code = curl_easy_perform (ugcurl->curl);
		curl_easy_getinfo (ugcurl->curl, CURLINFO_RESPONSE_CODE,
				&ugcurl->response);
		ugcurl->tested = TRUE;
	} while (ugcurl->restart);

	// free event
	if (ugcurl->event) {
		uget_event_free (ugcurl->event);
		ugcurl->event = NULL;
	}

	// HTTP response error code: 4xx Client Error, 5xx Server Error
	if (ugcurl->response >= 400 && ugcurl->scheme_type == SCHEME_HTTP) {
		ugcurl->state = UGET_CURL_ERROR;
		tempstr = ug_strdup_printf ("Server response code : %ld",
				ugcurl->response);
		ugcurl->event = uget_event_new_error (
				UGET_EVENT_ERROR_CUSTOM, tempstr);
		ug_free (tempstr);
		// discard data if remote site response error.
		ugcurl->pos = ugcurl->beg;
		ugcurl->size[0] = 0;
		goto exit;
	}

	switch (code) {
	case CURLE_OK:
		ugcurl->state = UGET_CURL_OK;
		break;

	// write error (out of disk space?) (exit)
	case CURLE_WRITE_ERROR:
		if (ugcurl->event_code > 0) {
			ugcurl->state = UGET_CURL_ERROR;
			ugcurl->event = uget_event_new_error (ugcurl->event_code, NULL);
			ugcurl->test_ok = FALSE;
			break;
		}
		// Don't break here
	// out of memory (exit)
	case CURLE_OUT_OF_MEMORY:
		ugcurl->state = UGET_CURL_ERROR;
		ugcurl->event = uget_event_new_error (
				UGET_EVENT_ERROR_OUT_OF_RESOURCE, NULL);
		break;

	// abort download in progress callback
	case CURLE_ABORTED_BY_CALLBACK:
		// segment is completed or paused by user
		if (ugcurl->paused == FALSE)
			ugcurl->state = UGET_CURL_OK;
		else
			ugcurl->state = UGET_CURL_ABORT;
		break;

	// can resume (retry)
	case CURLE_RECV_ERROR:
	case CURLE_PARTIAL_FILE:
	case CURLE_OPERATION_TIMEDOUT:
		ugcurl->state = UGET_CURL_RETRY;
		break;

	// can't resume (retry)
	case CURLE_RANGE_ERROR:
	case CURLE_BAD_DOWNLOAD_RESUME:
	case CURLE_FTP_COULDNT_USE_REST:
		ugcurl->state = UGET_CURL_NOT_RESUMABLE;
		ugcurl->resumable = FALSE;
		break;

#ifdef NO_RETRY_IF_CONNECT_FAILED
	// can't connect (error)
	case CURLE_COULDNT_CONNECT:
		ugcurl->state = UGET_CURL_ERROR;
		ugcurl->event = uget_event_new_error (
				UGET_EVENT_ERROR_CONNECT_FAILED, ugcurl->error_string);
		goto exit;
#else
	case CURLE_COULDNT_CONNECT:
	case CURLE_COULDNT_RESOLVE_HOST:
#endif
	// retry
	case CURLE_SEND_ERROR:
	case CURLE_GOT_NOTHING:
	case CURLE_BAD_CONTENT_ENCODING:
		ugcurl->state = UGET_CURL_RETRY;
		ugcurl->event = uget_event_new_error (
				UGET_EVENT_ERROR_CUSTOM, ugcurl->error_string);
		break;

	// too many redirection (exit)
	case CURLE_TOO_MANY_REDIRECTS:
		ugcurl->state = UGET_CURL_ERROR;
		ugcurl->event = uget_event_new_error (
				UGET_EVENT_ERROR_CUSTOM, ugcurl->error_string);
		break;

	// exit
	case CURLE_UNSUPPORTED_PROTOCOL:
		ugcurl->state = UGET_CURL_ERROR;
		ugcurl->event = uget_event_new_error (
				UGET_EVENT_ERROR_UNSUPPORTED_SCHEME, ugcurl->error_string);
		break;

	// other error (exit)
#ifdef NO_RETRY_IF_CONNECT_FAILED
	case CURLE_COULDNT_RESOLVE_HOST:
#endif
	case CURLE_COULDNT_RESOLVE_PROXY:
	case CURLE_FAILED_INIT:
	case CURLE_URL_MALFORMAT:
	case CURLE_FTP_WEIRD_SERVER_REPLY:
	case CURLE_REMOTE_ACCESS_DENIED:
	default:
		ugcurl->state = UGET_CURL_ERROR;
		ugcurl->event = uget_event_new_error (
				UGET_EVENT_ERROR_CUSTOM, ugcurl->error_string);
		break;
	}

exit:
	if (ugcurl->state == UGET_CURL_ERROR)
		ugcurl->test_ok = FALSE;
	ugcurl->stopped = TRUE;
	return UG_THREAD_RESULT;
}

void  uget_curl_run (UgetCurl* ugcurl, int joinable)
{
	CURL*  curl;

	curl = ugcurl->curl;
	uget_curl_decide_login (ugcurl);

	ugcurl->pos = ugcurl->beg;
	ugcurl->state = UGET_CURL_READY;
	ugcurl->paused = FALSE;
	ugcurl->stopped = FALSE;    // if thread stop, this value will be TRUE.
	ugcurl->response = 0;
	ugcurl->event_code = 0;
	ugcurl->progress_count = PROGRESS_COUNT_LIMIT;
	// reset download/upload speed
	ugcurl->speed[0] = 0;
	ugcurl->speed[1] = 0;
	// reset downloaded/uploaded size
	ugcurl->size[0] = 0;
	ugcurl->size[1] = 0;

	ug_free (ugcurl->header.uri);
	ug_free (ugcurl->header.filename);
	ugcurl->header.uri = NULL;
	ugcurl->header.filename = NULL;

	// Others -----------------------------------------------------------------
	curl_easy_setopt (curl, CURLOPT_NOSIGNAL, 1L);
	curl_easy_setopt (curl, CURLOPT_FILETIME, 1L);
	// disable peer SSL certificate verification
	curl_easy_setopt (curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, 0L);
	// SSL version and cipher
//	curl_easy_setopt (curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_DEFAULT);
//	curl_easy_setopt (curl, CURLOPT_SSLVERSION, CURL_SSLVERSION_SSLv3);
//	curl_easy_setopt (curl, CURLOPT_SSL_CIPHER_LIST, "ALL:!aNULL:!LOW:!EXPORT:!SSLv2");
//	curl_easy_setopt (curl, CURLOPT_SSL_CIPHER_LIST, "ALL");
//	curl_easy_setopt (curl, CURLOPT_SSL_CIPHER_LIST, "SSLv3");

	// low speed limit for unstable network
	curl_easy_setopt (curl, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);
	curl_easy_setopt (curl, CURLOPT_LOW_SPEED_TIME,  LOW_SPEED_TIME);

	// resume
	curl_easy_setopt (ugcurl->curl, CURLOPT_RESUME_FROM_LARGE,
			(curl_off_t) ugcurl->beg);
	// Progress  --------------------------------------------------------------
	curl_easy_setopt (curl, CURLOPT_XFERINFOFUNCTION, uget_curl_progress);
	curl_easy_setopt (curl, CURLOPT_XFERINFODATA, ugcurl);
	curl_easy_setopt (curl, CURLOPT_NOPROGRESS, FALSE);

	// Header -----------------------------------------------------------------
	if (ugcurl->tested) {
		switch (ugcurl->scheme_type) {
		case SCHEME_HTTP:
			curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION,
					(curl_write_callback) uget_curl_header_http);
			curl_easy_setopt (curl, CURLOPT_HEADERDATA, ugcurl);
			break;

		default:
			curl_easy_setopt (curl, CURLOPT_POSTQUOTE, NULL);
			curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, NULL);
			curl_easy_setopt (curl, CURLOPT_HEADERDATA, NULL);
			break;
		}
	}
	else {
		// setup by scheme type
		switch (ugcurl->scheme_type) {
		case SCHEME_HTTP:
			curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION,
					(curl_write_callback) uget_curl_header_http0);
			curl_easy_setopt (curl, CURLOPT_HEADERDATA, ugcurl);
			break;

		case SCHEME_FTP:
//			curl_easy_setopt (curl, CURLOPT_POSTQUOTE,
//					ugcurl->ftp_command);
			curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION,
					(curl_write_callback) uget_curl_header_ftp0);
			curl_easy_setopt (curl, CURLOPT_HEADERDATA, ugcurl);
			break;

		default:
			curl_easy_setopt (curl, CURLOPT_HEADERFUNCTION, NULL);
			curl_easy_setopt (curl, CURLOPT_HEADERDATA, NULL);
			break;
		}
	}

	// Speed limit ------------------------------------------------------------
	if (ugcurl->limit_changed) {
		curl_easy_setopt (ugcurl->curl, CURLOPT_MAX_RECV_SPEED_LARGE,
				(curl_off_t) ugcurl->limit[0]);
		curl_easy_setopt (ugcurl->curl, CURLOPT_MAX_SEND_SPEED_LARGE,
				(curl_off_t) ugcurl->limit[1]);
		ugcurl->limit_changed = FALSE;
	}

	// Output -----------------------------------------------------------------
	curl_easy_setopt (curl, CURLOPT_NOBODY, 0L);
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION,
	                        uget_curl_output_default);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, ugcurl);

	ug_thread_create (&ugcurl->thread, (UgThreadFunc)uget_curl_thread, ugcurl);
	if (joinable == FALSE)
		ug_thread_unjoin (&ugcurl->thread);
}

int  uget_curl_open_file (UgetCurl* ugcurl, const char* file_path)
{
	FILE*  output_file;

	if (ugcurl->file.output)
		return TRUE;

	if (file_path) {
		output_file = ug_fopen (file_path, "rb+");
		if (output_file == NULL)
			return FALSE;
		ugcurl->file.output = output_file;
	}
	return TRUE;
}

void  uget_curl_close_file (UgetCurl* ugcurl)
{
	if (ugcurl->file.output) {
		ug_fclose (ugcurl->file.output);
		ugcurl->file.output = NULL;
	}
}

void  uget_curl_set_url (UgetCurl* ugcurl, const char* uri)
{
	curl_easy_setopt (ugcurl->curl, CURLOPT_URL, uri);
	uget_curl_decide_scheme (ugcurl, uri);
	ugcurl->tested = FALSE;
	ugcurl->resumable = FALSE;
}

void  uget_curl_set_speed (UgetCurl* ugcurl, int64_t dlspeed, int64_t ulspeed)
{
	ugcurl->limit[0] = dlspeed;
	ugcurl->limit[1] = ulspeed;
	ugcurl->limit_changed = TRUE;
}

void  uget_curl_set_common (UgetCurl* ugcurl, UgetCommon* common)
{
	CURL*    curl;

	curl = ugcurl->curl;
	ugcurl->common = common;

	if (common == NULL)
		return;
	curl_easy_setopt (curl, CURLOPT_CONNECTTIMEOUT, common->connect_timeout);
	curl_easy_setopt (curl, CURLOPT_URL, common->uri);
	uget_curl_decide_scheme (ugcurl, common->uri);
	// debug
	if (common->debug_level)
		curl_easy_setopt(ugcurl->curl, CURLOPT_VERBOSE, 1L);
	// speed control
	uget_curl_set_speed (ugcurl,
			common->max_download_speed,
			common->max_upload_speed);
//	curl_easy_setopt (curl, CURLOPT_MAX_RECV_SPEED_LARGE,
//			(curl_off_t) common->max_download_speed);
//	curl_easy_setopt (curl, CURLOPT_MAX_SEND_SPEED_LARGE,
//			(curl_off_t) common->max_upload_speed);

	// I don't set common->user & common->password here because
	// uget_curl_decide_login() will decide to use one of below login data
	// common->user & common->password
	// http->user & http->password
	// ftp->user & ftp->password
}

void  uget_curl_set_proxy (UgetCurl* ugcurl, UgetProxy* proxy)
{
#ifdef HAVE_LIBPWMD
	if (proxy->type == UGET_PROXY_PWMD) {
		uget_curl_set_proxy_pwmd (ugcurl, proxy);
		return;
	}
#endif

	ug_curl_set_proxy (ugcurl->curl, proxy);
}

int   uget_curl_set_http (UgetCurl* ugcurl, UgetHttp* http)
{
	CURL*    curl;

	curl = ugcurl->curl;
	ugcurl->http = http;

  	curl_easy_setopt (curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt (curl, CURLOPT_AUTOREFERER, 1L);
	curl_easy_setopt (curl, CURLOPT_MAXREDIRS, 30L);

	if (http) {
//		if (http->referrer == NULL && ugcurl->uri.part.fragment != -1) {
//			http->referrer = ug_strndup (temp.common->uri,
//					ugcurl->uri.part.fragment);
//		}
		curl_easy_setopt (curl, CURLOPT_REFERER, http->referrer);
//		if (http->redirection_limit)
		curl_easy_setopt (curl, CURLOPT_MAXREDIRS, http->redirection_limit);

		if (http->user_agent)
			curl_easy_setopt (curl, CURLOPT_USERAGENT, http->user_agent);
//		else {
//			curl_easy_setopt (curl, CURLOPT_USERAGENT,
//					"Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1)");
//			curl_easy_setopt (curl, CURLOPT_USERAGENT,
//					"Mozilla/4.0 (compatible; MSIE 7.0; Windows NT 5.1;"
//					" .NET CLR 2.0.50727; .NET CLR 3.0.4506.2152; .NET CLR 3.5.30729)");
//		}

		// cookie
		if (http->cookie_data)
			curl_easy_setopt (curl, CURLOPT_COOKIE, http->cookie_data);
		else if (http->cookie_file)
			curl_easy_setopt (curl, CURLOPT_COOKIEFILE, http->cookie_file);
		else
			curl_easy_setopt (curl, CURLOPT_COOKIEFILE, "");

		if (http->post_data) {
			curl_easy_setopt (curl, CURLOPT_POST, 1L);
			curl_easy_setopt (curl, CURLOPT_POSTFIELDS, http->post_data);
			curl_easy_setopt (curl, CURLOPT_POSTFIELDSIZE, strlen (http->post_data));
		}
		else if (http->post_file) {
			ugcurl->file.post = ug_fopen (http->post_file, "r");
			if (ugcurl->file.post == NULL)
				return FALSE;
			curl_easy_setopt (curl, CURLOPT_POST, 1L);
			curl_easy_setopt (curl, CURLOPT_READDATA, ugcurl->file.post);
#if defined(_MSC_VER) && !defined(__MINGW32__)	// for MS VC only
			curl_easy_setopt (curl, CURLOPT_READFUNCTION , fread);
#endif
		}
	}

	// I don't set http->user & http->password here because
	// uget_curl_decide_login() will decide to use one of below login data
	// common->user & common->password
	// http->user & http->password
	// ftp->user & ftp->password

	return TRUE;
}

void  uget_curl_set_ftp (UgetCurl* ugcurl, UgetFtp* ftp)
{
	CURL*    curl;

	curl = ugcurl->curl;
	ugcurl->ftp = ftp;

	if (ftp && ftp->active_mode) {
		// FTP Active Mode
		// use EPRT and then LPRT before using PORT
		curl_easy_setopt (curl, CURLOPT_FTP_USE_EPRT, TRUE);
		// '-' symbol to let the library use your system's default IP address.
		curl_easy_setopt (curl, CURLOPT_FTPPORT, "-");
	}
	else {
		// FTP Passive Mode
		// use PASV command in IPv4 server. (Passive Mode)
		// don't use EPSV command. (Extended Passive Mode)
		curl_easy_setopt (curl, CURLOPT_FTP_USE_EPSV, FALSE);
		// don't use EPRT and LPRT command.
		curl_easy_setopt (curl, CURLOPT_FTP_USE_EPRT, FALSE);
		// don't use PORT command.
		curl_easy_setopt (curl, CURLOPT_FTPPORT, NULL);
	}

	// I don't set ftp->user & ftp->password here because
	// uget_curl_decide_login() will decide to use one of below login data
	// common->user & common->password
	// http->user & http->password
	// ftp->user & ftp->password
}

void uget_curl_decide_scheme (UgetCurl* ugcurl, const char* uri)
{
	int  length;

	ugcurl->scheme_type = SCHEME_UNKNOWN;
	if (uri == NULL)
		return;

	length = ug_uri_init (&ugcurl->uri.part, uri);
	if (length >= 3 && strncasecmp (uri, "ftp", 3) == 0)
		ugcurl->scheme_type = SCHEME_FTP;
	else if (length >= 4 && strncasecmp (uri, "http", 4) == 0)
		ugcurl->scheme_type = SCHEME_HTTP;
}

void uget_curl_decide_login (UgetCurl* ugcurl)
{
	CURL*    curl;
	union {
		UgetCommon*  common;
		UgetHttp*    http;
		UgetFtp*     ftp;
	} temp;

	curl = ugcurl->curl;

//	curl_easy_setopt (curl, CURLOPT_LOGIN_OPTIONS, "AUTH=NTLM");
	// ------------------------------------------------------------------------
	// common
	temp.common = ugcurl->common;
	if (temp.common) {
		if ((temp.common->user     && temp.common->user[0]) ||
		    (temp.common->password && temp.common->password[0]))
		{
			// set user & password by common data
			curl_easy_setopt (curl, CURLOPT_USERNAME,
					(temp.common->user)     ? temp.common->user     : "");
			curl_easy_setopt (curl, CURLOPT_PASSWORD,
					(temp.common->password) ? temp.common->password : "");
			// Don't set CURLOPT_HTTPAUTH to a bitmask
			// Don't use CURLAUTH_ANY, it causes authentication failed.
//			curl_easy_setopt (curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
//			curl_easy_setopt (curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
//			curl_easy_setopt (curl, CURLOPT_HTTPAUTH, CURLAUTH_NTLM);
		}
		else {
			// clear user & password (for HTTP redirection)
			curl_easy_setopt (curl, CURLOPT_USERNAME, NULL);
			curl_easy_setopt (curl, CURLOPT_PASSWORD, NULL);
		}
	}

	// ------------------------------------------------------------------------
	// http
	temp.http = ugcurl->http;
	if (temp.http && ugcurl->scheme_type == SCHEME_HTTP)
	{
		if ((temp.http->user     && temp.http->user[0]) ||
		    (temp.http->password && temp.http->password[0]))
		{
			curl_easy_setopt (curl, CURLOPT_USERNAME,
					(temp.http->user)     ? temp.http->user     : "");
			curl_easy_setopt (curl, CURLOPT_PASSWORD,
					(temp.http->password) ? temp.http->password : "");
			// Don't set CURLOPT_HTTPAUTH to a bitmask
			// Don't use CURLAUTH_ANY, it causes authentication failed.
//			curl_easy_setopt (curl, CURLOPT_HTTPAUTH, CURLAUTH_DIGEST);
//			curl_easy_setopt (curl, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
//			curl_easy_setopt (curl, CURLOPT_HTTPAUTH, CURLAUTH_NTLM);
			return;
		}
	}

	// ------------------------------------------------------------------------
	// ftp
	temp.ftp = ugcurl->ftp;
	if (temp.ftp && ugcurl->scheme_type == SCHEME_FTP) {
		// set FTP user & password
		if ((temp.ftp->user     && temp.ftp->user[0]) ||
		    (temp.ftp->password && temp.ftp->password[0]))
		{
			curl_easy_setopt (curl, CURLOPT_USERNAME,
					(temp.ftp->user)     ? temp.ftp->user     : "");
			curl_easy_setopt (curl, CURLOPT_PASSWORD,
					(temp.ftp->password) ? temp.ftp->password : "");
			return;
		}
	}
}

void  ug_curl_set_proxy (CURL* curl, UgetProxy* proxy)
{
	if (proxy == NULL)
		return;

	// proxy type
	switch (proxy->type) {
	case UGET_PROXY_NONE:
		curl_easy_setopt (curl, CURLOPT_PROXYTYPE, 0);
		break;

	default:
	case UGET_PROXY_DEFAULT:
	case UGET_PROXY_HTTP:
		curl_easy_setopt (curl, CURLOPT_PROXYTYPE, CURLPROXY_HTTP);
		break;

	case UGET_PROXY_SOCKS4:
		curl_easy_setopt (curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
		break;

	case UGET_PROXY_SOCKS5:
		curl_easy_setopt (curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);
		break;
	}

	curl_easy_setopt (curl, CURLOPT_PROXY, proxy->host);
	if (proxy->port)
		curl_easy_setopt (curl, CURLOPT_PROXYPORT, proxy->port);
	else
		curl_easy_setopt (curl, CURLOPT_PROXYPORT, 80);
	// proxy user and password
	if ((proxy->user     && proxy->user[0]) ||
		(proxy->password && proxy->password[0]) ||
		proxy->type == UGET_PROXY_SOCKS4 ||
		proxy->type == UGET_PROXY_SOCKS5)
	{
		curl_easy_setopt (curl, CURLOPT_PROXYUSERNAME,
				(proxy->user)     ? proxy->user     : "");
		curl_easy_setopt (curl, CURLOPT_PROXYPASSWORD,
				(proxy->password) ? proxy->password : "");
	}
	else {
		curl_easy_setopt (curl, CURLOPT_PROXYUSERNAME, NULL);
		curl_easy_setopt (curl, CURLOPT_PROXYPASSWORD, NULL);
	}
}

// ----------------------------------------------------------------------------
// static functions

static size_t uget_curl_header_http (char *buffer, size_t size,
                                     size_t nmemb, UgetCurl* ugcurl)
{
	long  response;

	curl_easy_getinfo (ugcurl->curl, CURLINFO_RESPONSE_CODE, &response);

	// This will abort the transfer and return CURL_WRITE_ERROR.
	if (response >= 400) {
		ugcurl->response = response;
		return 0;
	}
	else {
		// It doesn't need to parse HTTP header now.
		curl_easy_setopt (ugcurl->curl, CURLOPT_HEADERFUNCTION,
				(curl_write_callback) NULL);
		curl_easy_setopt (ugcurl->curl, CURLOPT_HEADERDATA, NULL);
	}
	return (size_t)(size * nmemb);
}

static size_t uget_curl_header_http0 (char *buffer, size_t size,
                                      size_t nmemb, UgetCurl* ugcurl)
{
	char*    file;
	char*    temp;
	int      length;

	file   = NULL;
	length = strlen (buffer);

	if (ugcurl->response == 0) {
		curl_easy_getinfo (ugcurl->curl, CURLINFO_RESPONSE_CODE,
				&ugcurl->response);
		// This will abort the transfer and return CURL_WRITE_ERROR.
		if (ugcurl->response >= 400)
			return 0;
	}

	if (length > 15 && strncasecmp (buffer, "Accept-Ranges: ", 15) == 0) {
		buffer += 15;
		if (strncasecmp (buffer, "none", 4) == 0)
			ugcurl->resumable = FALSE;
		else
			ugcurl->resumable = TRUE;
	}
	else if (length > 14 && strncasecmp (buffer, "Content-Type: ", 14) == 0) {
		buffer += 14;
		length = strcspn (buffer, "\r\n");
		if (length >= 9 && strncasecmp (buffer, "text/html", 9) == 0)
			ugcurl->html = TRUE;
		else
			ugcurl->html = FALSE;
	}
	// handle HTTP header "Location:"
	else if (length > 10 && strncasecmp (buffer, "Location: ", 10) == 0) {
		// exclude header and character '\r', '\n'
		buffer += 10;
		length = strcspn (buffer, "\r\n");
		if (ugcurl->header_store) {
			ug_free (ugcurl->header.uri);
			ugcurl->header.uri = ug_strndup (buffer, length);
			uget_curl_decide_scheme (ugcurl, ugcurl->header.uri);
		}
		else {
			temp = ug_strndup (buffer, length);
			uget_curl_decide_scheme (ugcurl, temp);
			ug_free (temp);
		}
		// decide login data (user & password) by scheme
		uget_curl_decide_login (ugcurl);
		// uget_curl_decide_scheme() has called ug_uri_init()
		// to initialize ugcurl->uri.part
//		ug_uri_init (&ugcurl->uri.part, ugcurl->header.uri);
		if (ugcurl->uri.part.file != -1 && ugcurl->header_store) {
			ug_free (ugcurl->header.filename);
			ugcurl->header.filename = ug_uri_get_file (&ugcurl->uri.part);
		}
		if (ugcurl->scheme_type == SCHEME_FTP) {
			curl_easy_setopt (ugcurl->curl, CURLOPT_HEADERFUNCTION,
					(curl_write_callback) uget_curl_header_ftp0);
		}
	}
	// handle HTTP header "Content-Location:"
	else if (length > 18 && strncasecmp (buffer, "Content-Location: ", 18) == 0) {
		// exclude header and character '\r', '\n'
		buffer += 18;
		temp = ug_strndup (buffer, strcspn (buffer, "\r\n"));
		ug_uri_init (&ugcurl->uri.part, temp);
		if (ugcurl->uri.part.file != -1 && ugcurl->header_store) {
			ug_free (ugcurl->header.filename);
			ugcurl->header.filename = ug_uri_get_file (&ugcurl->uri.part);
		}
		ug_free (temp);
	}
	// handle HTTP header "Content-Disposition:"
	else if (length > 21 && strncasecmp (buffer, "Content-Disposition: ", 21) == 0) {
		// exclude header and character '\r', '\n'
		buffer += 21;
		buffer = ug_strndup (buffer, strcspn (buffer, "\r\n"));
		// grab filename
		file = strstr (buffer, "filename=");
		if (file) {
			file += 9;	// value of "filename="
			if (file[0] != '\"')
				temp = ug_strndup (file, strcspn (file, ";"));
			else {
				file += 1;
				temp = ug_strndup (file, strcspn (file, "\""));
			}
			// grab filename
			ug_uri_init (&ugcurl->uri.part, temp);
			if (ugcurl->uri.part.file != -1 && ugcurl->header_store) {
				ug_free (ugcurl->header.filename);
				ugcurl->header.filename = ug_uri_get_file (&ugcurl->uri.part);
			}
			ug_free (temp);
		}
		ug_free (buffer);
	}

	return nmemb * size;
}

static size_t uget_curl_header_ftp0 (char *buffer, size_t size,
                                     size_t nmemb, UgetCurl* ugcurl)
{
	// REST command response "350 xxxx" if FTP server support REST
	if (strncmp (buffer, "350", 3) == 0) {
		ugcurl->resumable = TRUE;
		curl_easy_setopt (ugcurl->curl, CURLOPT_HEADERFUNCTION,
				(curl_write_callback) uget_curl_output_none);
		curl_easy_setopt (ugcurl->curl, CURLOPT_HEADERDATA, NULL);
	}

	return nmemb * size;
}

static size_t uget_curl_output_none (char *buffer, size_t size,
                                     size_t nmemb, void* data)
{
	return nmemb * size;
}

static size_t uget_curl_output_default (char *buffer, size_t size,
										size_t nmemb, void* data)
{
	UgetCurl*  ugcurl = data;
	FILE*      file;

	ugcurl->tested = TRUE;    // This URL was tested.
	// prepare
	if (ugcurl->prepare.func &&
	    ugcurl->prepare.func (ugcurl, ugcurl->prepare.data) == FALSE)
	{
		return 0;
	}
	ugcurl->test_ok = TRUE;   // This URL is OK.

	file = ugcurl->file.output;
	if (file == NULL) {
		ugcurl->event_code = UGET_EVENT_ERROR_NO_OUTPUT_FILE;
		// This will abort the transfer and return CURL_WRITE_ERROR.
		return 0;
	}
	// file offset
	ug_fseek (file, ugcurl->pos, SEEK_SET);

#if defined(_MSC_VER) && !defined(__MINGW32__)	// for MS VC only
	curl_easy_setopt (ugcurl->curl, CURLOPT_WRITEFUNCTION, fwrite);
#else
	curl_easy_setopt (ugcurl->curl, CURLOPT_WRITEFUNCTION, NULL);
#endif
	curl_easy_setopt (ugcurl->curl, CURLOPT_WRITEDATA, file);

	return fwrite (buffer, size, nmemb, file);
}

static int    uget_curl_progress (UgetCurl* ugcurl,
                                  curl_off_t dltotal, curl_off_t dlnow,
                                  curl_off_t ultotal, curl_off_t ulnow)
{
	ugcurl->pos = ugcurl->beg + (int64_t) dlnow;
	ugcurl->size[1] = (int64_t) ulnow;
	if (dlnow > 0 || ulnow > 0)
		ugcurl->state = UGET_CURL_RUN;

	ugcurl->progress_count++;
	if (ugcurl->progress_count > PROGRESS_COUNT_LIMIT) {
		curl_off_t speed_ul, speed_dl;
		ugcurl->progress_count = 0;
		curl_easy_getinfo (ugcurl->curl, CURLINFO_SPEED_UPLOAD_T, &speed_ul);
		curl_easy_getinfo (ugcurl->curl, CURLINFO_SPEED_DOWNLOAD_T, &speed_dl);
		ugcurl->speed[0] = (int64_t) speed_dl;
		ugcurl->speed[1] = (int64_t) speed_ul;
	}

	// speed limit changed
	if (ugcurl->limit_changed) {
		curl_easy_setopt (ugcurl->curl, CURLOPT_MAX_RECV_SPEED_LARGE,
				(curl_off_t) ugcurl->limit[0]);
		curl_easy_setopt (ugcurl->curl, CURLOPT_MAX_SEND_SPEED_LARGE,
				(curl_off_t) ugcurl->limit[1]);
		ugcurl->limit_changed = FALSE;
	}

	// Returning a non-zero value from this callback will cause libcurl
	// to abort the transfer and return CURLE_ABORTED_BY_CALLBACK.
	if (ugcurl->end > 0 && ugcurl->pos >= ugcurl->end) {
		ugcurl->pos = ugcurl->end;
		ugcurl->size[0] = ugcurl->pos - ugcurl->beg;
		return 1;
	}

	ugcurl->size[0] = ugcurl->pos - ugcurl->beg;
	if (ugcurl->paused)
		return 1;
	return 0;
}

// ----------------------------------------------------------------------------
// PWMD
//
#ifdef HAVE_LIBPWMD
static int  uget_curl_set_proxy_pwmd (UgetCurl* ugcurl, UgetProxy* proxy)
{
       CURL* curl;
       struct pwmd_proxy_s pwmd;
       gpg_error_t rc;

       curl = ugcurl->curl;
       memset(&pwmd, 0, sizeof(pwmd));
       rc = ug_set_pwmd_proxy_options(&pwmd, proxy);

       if (rc)
               goto fail;

       // proxy host and port
       curl_easy_setopt (curl, CURLOPT_PROXY, pwmd.hostname);
       curl_easy_setopt (curl, CURLOPT_PROXYPORT, pwmd.port);

       // proxy user and password
       if (pwmd.username || pwmd.password || !strcasecmp(pwmd.type, "socks4") || !strcasecmp(pwmd.type, "socks5")) {
               curl_easy_setopt (curl, CURLOPT_PROXYUSERNAME,
                               (pwmd.username) ? pwmd.username : "");
               curl_easy_setopt (curl, CURLOPT_PROXYPASSWORD,
                               (pwmd.password) ? pwmd.password : "");
       }
       else {
               curl_easy_setopt (curl, CURLOPT_PROXYUSERNAME, NULL);
               curl_easy_setopt (curl, CURLOPT_PROXYPASSWORD, NULL);
       }

       if (!strcasecmp(pwmd.type, "socks4"))
               curl_easy_setopt (curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
       else if (!strcasecmp(pwmd.type, "socks5"))
               curl_easy_setopt (curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS5);

       ug_close_pwmd(&pwmd);
       return TRUE;

fail:
       ug_close_pwmd(&pwmd);
       ugcurl->state = UGET_CURL_ERROR;
       gchar *e = ug_strdup_printf("Pwmd ERR %u: %s", rc, gpg_strerror(rc));
       ugcurl->event = uget_event_new_error (UGET_EVENT_ERROR_CUSTOM, e);
       g_free(e);
       return FALSE;
}

#endif	// HAVE_LIBPWMD

