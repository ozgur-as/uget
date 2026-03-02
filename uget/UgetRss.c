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

#include <UgUtil.h>
#include <UgString.h>
#include <UgHtmlEntry.h>
#include <UgJson-custom.h>
#include <UgJsonFile.h>
#include <UgetRss.h>
#include <curl/curl.h>

// Built-in RSS feed URLs removed (old FeedBurner URLs are dead).
// The RSS system is preserved for future use — register feeds via
// uget_rss_add() with custom URLs.
#define UGET_RSS_URL_STABLE     ""
#define UGET_RSS_URL_DEVELMENT  ""
#define UGET_RSS_URL_NEWS       ""
#define UGET_RSS_URL_TUTORIALS  ""
#define UGET_RSS_URL_ALL        ""

// ----------------------------------------------------------------------------
// link

void ug_html_start_element_rss_link (UgHtml*        uhtml,
                                     const char*    element_name,
                                     const char**   attribute_names,
                                     const char**   attribute_values,
                                     void*          dest,
                                     void*          data)
{
	const char* rel  = NULL;
	const char* href = NULL;
	const char* type = NULL;
	int         index;

	// if <link> string already exist
	if (*(void**)dest) {
		ug_html_push (uhtml, &ug_html_parser_unknown, dest, NULL);
		return;
	}
	// RSS 2.0
	if (attribute_names[0] == NULL) {
		ug_html_push (uhtml, &ug_html_parser_string, dest, NULL);
		return;
	}

	// Atom
	for (index = 0;  attribute_names[index];  index++) {
		if (strcmp (attribute_names[index], "rel") == 0)
			rel = attribute_values[index];
		else if (strcmp (attribute_names[index], "href") == 0)
			href = attribute_values[index];
		else if (strcmp (attribute_names[index], "type") == 0)
			type = attribute_values[index];

		if (type == NULL || href == NULL || rel == NULL)
			continue;

		if (strcmp (type, "text/html") != 0)
			continue;
		if (strcmp (rel,  "alternate") != 0)
			continue;
		*(char**) dest = ug_strdup (href);
	}

	ug_html_push (uhtml, &ug_html_parser_unknown, NULL, NULL);
}

// ----------------------------------------------------------------------------
// Rss <item> (ATOM <entry>)

const UgHtmlEntry  UgHtmlEntryRssItem[] =
{
	{"title",    offsetof (UgetRssItem, title),    &ug_html_parser_string, NULL, NULL},
	{"link",     offsetof (UgetRssItem, link),     &ug_html_parser_custom, ug_html_start_element_rss_link, NULL},
	{"updated",  offsetof (UgetRssItem, updated),  &ug_html_parser_time_rfc3339, NULL, NULL},
	{"pubDate",  offsetof (UgetRssItem, updated),  &ug_html_parser_time_rfc822,  NULL, NULL},
	{NULL}
};

void ug_html_start_element_rss_item (UgHtml*        uhtml,
                                     const char*    element_name,
                                     const char**   attribute_names,
                                     const char**   attribute_values,
                                     void*          dest,
                                     void*          data)
{
	UgetRssItem* item;

	item = uget_rss_item_new ();
	ug_list_append ((UgList*) dest, (UgLink*) item);
	ug_html_push (uhtml, &ug_html_parser_entry,
	              item, (void*) &UgHtmlEntryRssItem);
}

// ----------------------------------------------------------------------------
// RSS

const UgHtmlEntry  UgHtmlEntryRss[] =
{
	{"title",    offsetof (UgetRssFeed, title),    &ug_html_parser_string, NULL, NULL},
	{"link",     offsetof (UgetRssFeed, link),     &ug_html_parser_string, NULL, NULL},
	{"updated",  offsetof (UgetRssFeed, updated),  &ug_html_parser_time_rfc3339, NULL, NULL},
	{"pubDate",  offsetof (UgetRssFeed, updated),  &ug_html_parser_time_rfc822,  NULL, NULL},
	{"item",     offsetof (UgetRssFeed, items),    &ug_html_parser_custom,  ug_html_start_element_rss_item, NULL},
	{"entry",    offsetof (UgetRssFeed, items),    &ug_html_parser_custom,  ug_html_start_element_rss_item, NULL},
	{NULL}
};

void ug_html_start_element_rss (UgHtml*        uhtml,
                                const char*    element_name,
                                const char**   attribute_names,
                                const char**   attribute_values,
                                void*          dest,
                                void*          data)
{
	if (strcmp (element_name, "channel") == 0 || strcmp (element_name, "feed") == 0) {
		ug_html_push (uhtml, &ug_html_parser_entry,
		              dest, (void*) &UgHtmlEntryRss);
	}
}

void ug_html_end_element_rss (UgHtml*        uhtml,
                              const char*    element_name,
                              void*          dest,
                              void*          data)
{
	if (strcmp (element_name, "channel") == 0 || strcmp (element_name, "feed") == 0)
		ug_html_pop (uhtml);
}

const UgHtmlParser  ug_html_parser_rss =
{
	(void*) ug_html_start_element_rss,
	(void*) ug_html_end_element_rss,
	(void*) NULL
};

// ----------------------------------------------------------------------------
// UgetRssItem

UgetRssItem* uget_rss_item_new (void)
{
	UgetRssItem* item;

	item = ug_malloc0 (sizeof (UgetRssItem));
	item->self = item;
	return item;
}

void uget_rss_item_free (UgetRssItem* item)
{
	ug_free (item->title);
	ug_free (item->link);
	ug_free (item);
}

// ----------------------------------------------------------------------------
// UgetRssFeed

const UgEntry  UgetRssFeedEntry[] =
{
	{"type",    offsetof (UgetRssFeed, type),    UG_ENTRY_INT,    NULL, NULL},
	{"url",     offsetof (UgetRssFeed, url),     UG_ENTRY_STRING, NULL, NULL},
	{"checked", offsetof (UgetRssFeed, checked), UG_ENTRY_CUSTOM,
			ug_json_parse_time_t, ug_json_write_time_t},
	{NULL}
};

UgetRssFeed*  uget_rss_feed_new (void)
{
	UgetRssFeed*  feed;

	feed = ug_malloc0 (sizeof (UgetRssFeed));
	feed->self = feed;
	feed->checked = -1;
	return feed;
}

void  uget_rss_feed_free (UgetRssFeed* feed)
{
	uget_rss_feed_clear (feed, FALSE);
	ug_free (feed);
}

void  uget_rss_feed_clear (UgetRssFeed* feed, int reserve_user_data)
{
	ug_free (feed->title);
	ug_free (feed->link);
	ug_list_foreach (&feed->items, (UgForeachFunc) uget_rss_item_free, NULL);
	ug_list_clear (&feed->items, FALSE);

	feed->title = NULL;
	feed->link = NULL;
	feed->updated = -1;

	if (reserve_user_data == FALSE) {
		ug_free (feed->url);
		feed->url = NULL;
		feed->type = 0;
		feed->checked = -1;
	}
}

void  uget_rss_feed_move (UgetRssFeed* feed, UgetRssFeed* src)
{
	uget_rss_feed_clear (feed, TRUE);
	feed->title = src->title;
	feed->link  = src->link;
	feed->updated = src->updated;
	feed->items = src->items;

	src->title = NULL;
	src->link = NULL;
	src->updated = -1;
	ug_list_clear (&src->items, FALSE);
}

UgetRssItem*  uget_rss_feed_find (UgetRssFeed* feed, time_t time_after)
{
	UgetRssItem*  item;
	UgetRssItem*  result = NULL;

	if (time_after == -1)
		result = (UgetRssItem*) feed->items.head;
	else {
		for (item = (UgetRssItem*) feed->items.head;  item;  item = item->next) {
			if (item->updated > time_after) {
				if (result == NULL || result->updated > item->updated)
					result = item;
			}
		}
	}

	return result;
}

// ----------------------------------------------------------------------------
// UgetRss

UgetRss*  uget_rss_new (void)
{
	UgetRss*  urss;

	urss = ug_malloc (sizeof (UgetRss));
	ug_html_init (&urss->uhtml);
	ug_list_init (&urss->feeds);
	urss->checked = NULL;
	urss->updating = FALSE;
	urss->ref_count = 1;
	return urss;
}

void  uget_rss_ref (UgetRss* urss)
{
	urss->ref_count++;
}

void  uget_rss_unref (UgetRss* urss)
{
	if (--urss->ref_count == 0) {
		ug_html_final (&urss->uhtml);
		ug_list_foreach (&urss->feeds, (UgForeachFunc) uget_rss_feed_free, NULL);
		ug_free (urss);
	}
}

void  uget_rss_add (UgetRss* urss, UgetRssFeed* feed)
{
	ug_list_append (&urss->feeds, (UgLink*) feed);
}

void  uget_rss_add_builtin (UgetRss* urss, UgetRssType type)
{
	UgetRssFeed*  feed;
	const char*   url;

	switch (type) {
	case UGET_RSS_STABLE:
		url = UGET_RSS_URL_STABLE;
		break;

	case UGET_RSS_DEVELMENT:
		url = UGET_RSS_URL_DEVELMENT;
		break;

	case UGET_RSS_NEWS:
		url = UGET_RSS_URL_NEWS;
		break;

	case UGET_RSS_TUTORIALS:
		url = UGET_RSS_URL_TUTORIALS;
		break;

	case UGET_RSS_ALL:
		url = UGET_RSS_URL_ALL;
		break;

	default:
		return;
	}

	for (feed = (UgetRssFeed*) urss->feeds.head;  feed;  feed = feed->next) {
		if (feed->type == type) {
			if (strcmp (feed->url, url)) {
				ug_free (feed->url);
				feed->url = ug_strdup (url);
			}
			return;
		}
	}

	feed = uget_rss_feed_new ();
	feed->type = type;
	feed->url = ug_strdup (url);
	uget_rss_add (urss, feed);
}

void  uget_rss_add_url (UgetRss* urss, const char* url)
{
	UgetRssFeed*  feed;

	feed = uget_rss_feed_new ();
	feed->url = ug_strdup (url);
	feed->type = UGET_RSS_USER;
	ug_list_append (&urss->feeds, (UgLink*) feed);
}

/*
static size_t  uget_rss_curl_check (void *ptr, size_t size, size_t nmemb, UgHtml* uhtml)
{
	return nmemb;
}
 */

static size_t  uget_rss_curl_write (void *ptr, size_t size, size_t nmemb, UgHtml* uhtml)
{
	ug_html_parse (uhtml, (char*) ptr, size * nmemb);
	return nmemb;
}

static UgThreadResult  uget_rss_thread (UgetRss* urss)
{
	CURL*		  curl;
	CURLcode	  res;
	long		  response_code = 0;
	UgetRssFeed*  feed;
	UgetRssFeed*  temp;
	UgetRssItem*  item;

	curl = curl_easy_init();
	// disable peer SSL certificate verification
	curl_easy_setopt (curl, CURLOPT_SSL_VERIFYPEER, FALSE);
	// others
	curl_easy_setopt (curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION,
			(curl_write_callback) uget_rss_curl_write);
	curl_easy_setopt (curl, CURLOPT_WRITEDATA, &urss->uhtml);
	temp = uget_rss_feed_new ();

	for (feed = (UgetRssFeed*) urss->feeds.head;  feed;  feed = feed->next) {
		if (feed->url == NULL)
			continue;
		uget_rss_feed_clear (temp, FALSE);

		ug_html_push (&urss->uhtml, &ug_html_parser_rss, temp, NULL);
		ug_html_begin_parse (&urss->uhtml);

		curl_easy_setopt (curl, CURLOPT_URL, feed->url);
		res = curl_easy_perform (curl);
		curl_easy_getinfo (curl, CURLINFO_RESPONSE_CODE, &response_code);

		ug_html_end_parse (&urss->uhtml);
		if (res != CURLE_OK || response_code >= 400)
			continue;
		uget_rss_feed_move (feed, temp);
		// check item
		item = (UgetRssItem*) feed->items.head;
		if (item && item->updated > feed->checked)
			urss->n_updated++;
	}

	uget_rss_feed_free (temp);
	curl_easy_cleanup (curl);

	urss->updating = FALSE;
	uget_rss_unref (urss);
	return UG_THREAD_RESULT;
}

void  uget_rss_update (UgetRss* urss, int joinable)
{
	if (urss->updating == TRUE)
		return;

	urss->n_updated = 0;
	urss->updating = TRUE;
	uget_rss_ref (urss);
	ug_thread_create (&urss->thread, (UgThreadFunc) uget_rss_thread, urss);
	if (joinable == FALSE)
		ug_thread_unjoin (&urss->thread);
}

UgetRssFeed*  uget_rss_find_updated (UgetRss* urss, UgetRssFeed* feed_after)
{
	UgetRssItem*  item;

	if (feed_after == NULL) {
		if (urss->checked)
			feed_after = urss->checked->next;
	}
	if (feed_after == NULL)
		feed_after = (UgetRssFeed*) urss->feeds.head;

	for (;  feed_after;  feed_after = feed_after->next) {
		item = (UgetRssItem*) feed_after->items.head;
		if (item && item->updated > feed_after->checked)
			break;
	}

	urss->checked = feed_after;
	return feed_after;
}

int   uget_rss_save_feeds (UgetRss* urss, const char* path)
{
	UgJsonFile*  jfile;

	jfile = ug_json_file_new (4096);
	if (ug_json_file_begin_write (jfile, path, UG_JSON_FORMAT_ALL)) {
//		ug_json_write_array_head (&jfile->json);
		ug_json_write_entry (&jfile->json, urss, UgetRssEntry);
//		ug_json_write_array_tail (&jfile->json);
		ug_json_file_end_write (jfile);
		ug_json_file_free (jfile);
		return TRUE;
	}
	ug_json_file_free (jfile);
	return FALSE;
}

int   uget_rss_load_feeds (UgetRss* urss, const char* path)
{
	UgJsonFile*  jfile;

	jfile = ug_json_file_new (0);
	if (ug_json_file_begin_parse (jfile, path)) {
		ug_json_push (&jfile->json, ug_json_parse_entry,  urss, (void*) UgetRssEntry);
//		ug_json_push (&jfile->json, ug_json_parse_array, urss, NULL);
		ug_json_file_end_parse (jfile);
		ug_json_file_free (jfile);
		return TRUE;
	}
	ug_json_file_free (jfile);
	return FALSE;
}

// ------------------------------------

static UgJsonError ug_json_parse_uget_rss_feeds (UgJson* json,
                                const char* name, const char* value,
                                void* rss, void* none)
{
	UgetRss*      urss = rss;
	UgetRssFeed*  feed;

	if (json->type != UG_JSON_OBJECT) {
//		if (json->type == UG_JSON_ARRAY)
//			ug_json_push (json, ug_json_parse_unknown, NULL, NULL);
		return UG_JSON_ERROR_TYPE_NOT_MATCH;
	}

	feed = uget_rss_feed_new ();
	ug_list_append (&urss->feeds, (UgLink*) feed);
	ug_json_push (json, ug_json_parse_entry, feed, (void*)UgetRssFeedEntry);
	return UG_JSON_ERROR_NONE;
}

static void  ug_json_write_uget_rss_feeds (UgJson* json, const UgetRss* urss)
{
	UgetRssFeed*  feed;

	for (feed = (UgetRssFeed*) urss->feeds.head;  feed;  feed = feed->next) {
		ug_json_write_object_head (json);
		ug_json_write_entry (json, (void*) feed, UgetRssFeedEntry);
		ug_json_write_object_tail (json);
	}
}

const UgEntry  UgetRssEntry[] =
{
	{NULL,  0, UG_ENTRY_ARRAY,
			ug_json_parse_uget_rss_feeds, ug_json_write_uget_rss_feeds},
	{NULL}
};

