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

#include <stdio.h>
#include <string.h>
#include <UgString.h>
#include <UgJsonrpc.h>
#include <UgJsonrpcCurl.h>
#include <curl/curl.h>

static int  global_ref_count = 0;

void  ug_jsonrpc_curl_init (UgJsonrpcCurl* jrcurl)
{
	if (global_ref_count == 0) {
		curl_global_init (CURL_GLOBAL_ALL);
	}
	global_ref_count++;

	ug_buffer_init (&jrcurl->buffer, 4096);
	ug_json_init (&jrcurl->json);
	ug_jsonrpc_init (&jrcurl->rpc, &jrcurl->json, &jrcurl->buffer);
	jrcurl->url = NULL;

	// libcurl
	jrcurl->curl = curl_easy_init ();
	jrcurl->slist = NULL;
	jrcurl->slist = curl_slist_append (jrcurl->slist,
			"Content-Type: application/json-rpc; charset=utf-8");

	jrcurl->rpc.send.func = (UgJsonrpcFunc) ug_jsonrpc_curl_send;
	jrcurl->rpc.send.data = jrcurl;
	jrcurl->rpc.receive.func = (UgJsonrpcFunc) ug_jsonrpc_curl_receive;
	jrcurl->rpc.receive.data = jrcurl;
}

void  ug_jsonrpc_curl_final (UgJsonrpcCurl* jrcurl)
{
	global_ref_count--;
	if (global_ref_count == 0) {
		curl_global_cleanup ();
	}

	ug_json_final (&jrcurl->json);
	ug_jsonrpc_clear (&jrcurl->rpc);
	ug_buffer_clear (&jrcurl->buffer, TRUE);
	ug_free (jrcurl->url);

	// libcurl
	curl_easy_cleanup (jrcurl->curl);
	curl_slist_free_all (jrcurl->slist);
}

void  ug_jsonrpc_curl_set_url (UgJsonrpcCurl* jrcurl, const char* url)
{
	ug_free (jrcurl->url);
	jrcurl->url = ug_strdup (url);
	curl_easy_setopt (jrcurl->curl, CURLOPT_URL, jrcurl->url);
	curl_easy_setopt (jrcurl->curl, CURLOPT_CONNECTTIMEOUT, 15);
	curl_easy_setopt (jrcurl->curl, CURLOPT_NOSIGNAL, 1L);
	// disable peer SSL certificate verification
	curl_easy_setopt (jrcurl->curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt (jrcurl->curl, CURLOPT_SSL_VERIFYPEER, 0L);
}

static size_t	ug_jsonrpc_curl_write (char* buffer, size_t size, size_t nmemb, UgJsonrpcCurl* jrcurl)
{
	int  error;

	size = size * nmemb;
#ifdef DEBUG
	printf ("\n%.*s", size, buffer);
#endif

	error = ug_json_parse (&jrcurl->json, buffer, size);
	jrcurl->receive_size += size;
	if (error < 0 || jrcurl->rpc.error == 0)
		jrcurl->rpc.error = error;
	return size;
}

int   ug_jsonrpc_curl_send (UgJsonrpcCurl* jrcurl)
{
	int  send_size;

	send_size = jrcurl->buffer.cur - jrcurl->buffer.beg;
	curl_easy_setopt (jrcurl->curl, CURLOPT_POST, TRUE);
	curl_easy_setopt (jrcurl->curl, CURLOPT_POSTFIELDS, jrcurl->buffer.beg);
	curl_easy_setopt (jrcurl->curl, CURLOPT_POSTFIELDSIZE, (long) send_size);
	curl_easy_setopt (jrcurl->curl, CURLOPT_HTTPHEADER, jrcurl->slist);
	curl_easy_setopt (jrcurl->curl, CURLOPT_WRITEFUNCTION,
			(curl_write_callback) ug_jsonrpc_curl_write);
	curl_easy_setopt (jrcurl->curl, CURLOPT_WRITEDATA, jrcurl);

	curl_easy_perform (jrcurl->curl);

	jrcurl->receive_size = 0;
	jrcurl->response = 0;
	curl_easy_getinfo (jrcurl->curl, CURLINFO_RESPONSE_CODE, &jrcurl->response);

	jrcurl->buffer.cur = jrcurl->buffer.beg;
	if (jrcurl->response != 200)
		return -1;
	return send_size;
}

int   ug_jsonrpc_curl_receive (UgJsonrpcCurl* jrcurl)
{
	jrcurl->buffer.cur = jrcurl->buffer.beg;
	if (jrcurl->response != 200)
		return -1;
	return jrcurl->receive_size;
}

