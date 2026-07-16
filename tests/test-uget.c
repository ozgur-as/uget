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
#include <UgArray.h>
#include <UgetNode.h>

#include <UgString.h>
#include <UgData.h>
#include <UgetFiles.h>
#include <UgJson.h>
//#include <UgetPlugin.h>

#include <UgetA2cf.h>
#include <UgetCurl.h>
#include <UgetRss.h>
#include <UgetSequence.h>

// ----------------------------------------------------------------------------
// UgetNode

UgEntry	NodeChildrenEntry[] = {
	{NULL, 0, UG_ENTRY_ARRAY,
			(UgJsonParseFunc) ug_json_parse_uget_node_children,
			(UgJsonWriteFunc) ug_json_write_uget_node_children},
	{NULL}
};

void init_node (UgetNode* parent)
{
	UgetNode*  newone;
	UgetNode*  newtwo;

	newone = uget_node_new (NULL);
	uget_node_append (parent, newone);
	newtwo = uget_node_new (NULL);
	uget_node_append (parent, newtwo);

	parent = newtwo;
	newone = uget_node_new (NULL);
	uget_node_append (parent, newone);
	newtwo = uget_node_new (NULL);
	uget_node_append (parent, newtwo);
}

// "01-03-02"
void dump_int_array (UgArrayInt* array)
{
	int  index;

	for (index = 0;  index < array->length;  index++) {
		if (index)
			printf ("-");
		printf ("%.2d", array->at[index]);
	}
	puts ("");
}

void dump_fake_path (UgetNode* node, UgArrayInt* array)
{
	UgetNode*   cur;
	int         index;

	for (index = 0, cur = node->fake;  cur;  cur = cur->peer, index++) {
		*(int*) ug_array_alloc(array, 1) = index;
		dump_int_array (array);
		dump_fake_path (cur, array);
		array->length--;
	}
}

void print_fake_path (UgetNode* node)
{
	UgArrayInt  array;
	ug_array_init(&array, sizeof (int), 20);
	dump_fake_path (node, &array);
	ug_array_clear (&array);
}

void test_fake_path ()
{
	UgetNode*  node;
	UgetNode*  fake0;
	UgetNode*  fake1;
	UgetNode*  fake2;

	node = uget_node_new (NULL);
	fake0 = uget_node_new (node);
	fake1 = uget_node_new (node);
	fake2 = uget_node_new (node);

	uget_node_new (fake0);

	uget_node_new (fake1);
	uget_node_new (fake1);

	uget_node_new (fake2);
	uget_node_new (fake2);
	uget_node_new (fake2);

	print_fake_path (node);

	uget_node_free (node);
}

void parse_node (UgetNode* parent)
{
	int     code;
	UgJson  json;
	const char* json_string = {
		"["
		"  {"
		"    \"name\": null,"
		"    \"type\": 1,"
		"    \"info\": {"
		"    },"
		"    \"children\": ["
		"    ]"
		"  },"
		"  {"
		"    \"name\": null,"
		"    \"type\": 2,"
		"    \"info\": {"
		"    },"
		"    \"children\": ["
		"      {"
		"        \"name\": null,"
		"        \"type\": 3,"
		"        \"info\": {"
		"        },"
		"        \"children\": ["
		"        ]"
		"      },"
		"      {"
		"        \"name\": null,"
		"        \"type\": 4,"
		"        \"info\": {"
		"        },"
		"        \"children\": ["
		"        ]"
		"      }"
		"    ]"
		"  }"
		"]"
	};

	ug_json_init (&json);
	ug_json_begin_parse (&json);
#if 1
	// method 1: use UgEntry to parse start of array
	ug_json_push (&json, ug_json_parse_entry, parent, NodeChildrenEntry);
#else
	// method 2: push ug_json_parse_array() to parse start of array
	ug_json_push (&json, ug_json_parse_uget_node_children, parent, NULL);
	ug_json_push (&json, ug_json_parse_array, NULL, NULL);
#endif
	code = ug_json_parse (&json, json_string, -1);
	printf ("ug_json_parse response %d\n", code);
	code = ug_json_end_parse (&json);
	printf ("ug_json_end_parse response %d\n", code);
	ug_json_final (&json);
}

// UgBufferFunc
static int  buffer_to_file (UgBuffer* buffer)
{
	FILE*   file = buffer->data;

	printf ("write %d bytes to file\n", ug_buffer_length (buffer));
	fwrite (buffer->beg, 1, ug_buffer_length(buffer), file);
	buffer->cur = buffer->beg;
	return 0;
}

void write_node_to_file (UgetNode* node, char* filename)
{
	UgBuffer  buffer;
	UgJson    json;
	FILE*     file;

	file = fopen (filename, "w");
	ug_buffer_init (&buffer, 128);
	buffer.more = buffer_to_file;
	buffer.data = file;

	ug_json_init (&json);
	ug_json_begin_write (&json, UG_JSON_FORMAT_INDENT, &buffer);
#if 1
	// method 1: use UgEntry to write start of object
	ug_json_write_entry (&json, node, NodeChildrenEntry);
#else
	// method 2: call ug_json_write_object_head() to write start of object
	ug_json_write_array_head (&json);
	ug_json_write_uget_node_children (&json, node);
	ug_json_write_array_tail (&json);
#endif
	ug_json_end_write (&json);
	ug_json_final (&json);

	ug_buffer_clear (&buffer, 1);
	fclose (file);
}

void test_uget_node ()
{
	UgetNode* root;

	root = uget_node_new (NULL);
#if 0
	init_node (root);
#else
	parse_node (root);
#endif
	write_node_to_file (root, "test-UgetNode.json");
}

// ----------------------------------------------------------------------------
// UgetA2cf

void print_bitfield (uint8_t* bytes, int length)
{
	int  count;
	int  index;
	int  temp;

	for (index = 0;  index < length;  index++) {
		for (count = 0;  count < 8;  count++) {
			temp = (bytes[0] << count) & 0x80;
			if (temp)
				printf ("1");
			else
				printf ("0");
		}
		printf ("\n");
		bytes++;
	}
}

void print_a2cf (UgetA2cf* a2cf)
{
	UgetA2cfPiece* piece;

	if (a2cf == NULL)
		return;

	printf ("ver : %d\n", (int)a2cf->ver);
	printf ("ext : %d\n", (int)a2cf->ext);
	printf ("info_hash_len : %d\n", (int)a2cf->info_hash_len);

	printf ("piece_len : %d\n", (int)a2cf->piece_len);
	printf ("total_len : %d\n", (int)a2cf->total_len);
	printf ("upload_len : %d\n", (int)a2cf->upload_len);

	printf ("bitfield_len : %d\n", (int)a2cf->bitfield_len);
	if (a2cf->bitfield_len) {
		printf ("bitfield:\n");
		print_bitfield (a2cf->bitfield, a2cf->bitfield_len);
	}

	printf ("n_pieces : %d\n", (int)a2cf->piece.list.size);
	for (piece = (void*)a2cf->piece.list.head;  piece;  piece = piece->next) {
		printf ("index : %d\n", (int)piece->index);
		printf ("length : %d\n", (int)piece->length);
		printf ("bitfield_length : %d\n", (int)piece->bitfield_len);
		printf ("bitfield:\n");
		print_bitfield (piece->bitfield, piece->bitfield_len);
	}
}

void test_uget_a2cf (void)
{
	UgetA2cf     a2cf;
	const char*  fname;
	uint64_t  beg, end;

	memset (&a2cf, 0, sizeof (UgetA2cf));
	fname = "D:\\Downloads\\TestData-Aria2\\npp.6.4.2.Installer.exe.aria2-1";
	printf ("%s\n", fname);
	uget_a2cf_load (&a2cf, fname);
	print_a2cf (&a2cf);
	uget_a2cf_clear (&a2cf);

	fname = "D:\\Downloads\\TestData-Aria2\\npp.6.4.2.Installer.exe.aria2-2";
	printf ("%s\n", fname);
	uget_a2cf_load (&a2cf, fname);
	// fill
	beg = 3145728 + 16384 * 2;
	end = 5242880 + 16384 * 4 + 1;
//	beg = 7340032; // + 16384 * 2;
//	end = 7401344;
	printf ("fill %d - %d\n", (int)beg, (int)end);
	printf ("fill result %d\n", (int)uget_a2cf_fill (&a2cf, beg, end));
	// lack
	beg = 0;
//	beg = 3145728 - 16384 * 2;
//	beg = 5242880;
//	beg = 7340032;
//	beg = 7401344;
	if (uget_a2cf_lack (&a2cf, &beg, &end))
		printf ("lack %d - %d\n", (int)beg, (int)end);
	print_a2cf (&a2cf);
	// completed
	printf ("completed: %u\n", (unsigned)uget_a2cf_completed (&a2cf));
	// clear
	uget_a2cf_clear (&a2cf);

	fname = "D:\\Downloads\\TestData-Aria2\\npp.6.4.3.Installer.exe.aria2";
	printf ("%s\n", fname);
	uget_a2cf_load (&a2cf, fname);
	print_a2cf (&a2cf);
	// lack
	beg = 0;
	uget_a2cf_lack (&a2cf, &beg, &end);
	printf ("lack %d - %d\n", (int)beg, (int)end);
	uget_a2cf_clear (&a2cf);

	fname = "D:\\Downloads\\TestData-Aria2\\ubuntu-13.04-desktop-amd64.iso.aria2";
	printf ("%s\n", fname);
	uget_a2cf_load (&a2cf, fname);
	print_a2cf (&a2cf);
	uget_a2cf_clear (&a2cf);

	fname = "D:\\Downloads\\TestData-Aria2\\eclipse-cpp-kepler-R-win32.zip.aria2";
	printf ("%s\n", fname);
	uget_a2cf_load (&a2cf, fname);
	print_a2cf (&a2cf);
	uget_a2cf_clear (&a2cf);
}

// ----------------------------------------------------------------------------
// UgetCurl

void test_uget_curl (void)
{
	UgetCurl*  ugcurl;

	ugcurl = uget_curl_new ();
	uget_curl_set_url (ugcurl, "http://msysgit.googlecode.com/files/Git-1.8.5.2-preview20131230.exe");
	uget_curl_open_file (ugcurl, NULL);
	ugcurl->header_store = TRUE;
	uget_curl_run (ugcurl, TRUE);
	ug_thread_join (&ugcurl->thread);
	uget_curl_free (ugcurl);
}

// ----------------------------------------------------------------------------
// UgetRss

void test_uget_rss (void)
{
	UgetRss*     urss;
	UgetRssFeed* feed;
	UgetRssItem* item = NULL;

	urss = uget_rss_new ();
	uget_rss_add_builtin (urss, UGET_RSS_STABLE);
	uget_rss_update (urss, TRUE);
	ug_thread_join (&urss->thread);

	feed = uget_rss_find_updated (urss, NULL);
	if (feed)
		item = uget_rss_feed_find (feed, -1);
	if (item)
		printf ("title %s\n", item->title);

	uget_rss_unref (urss);
}

// ----------------------------------------------------------------------------
// UgetSeq

void test_seq (void)
{
	UgetSequence  useq;
	UgList  list;
	UgLink* link;

	ug_list_init (&list);

	uget_sequence_init (&useq);

	// digits
	uget_sequence_add (&useq, 0, 5, 2);
	// ASCII or Unicode
	uget_sequence_add (&useq, 'a', 'b', 0);
	uget_sequence_add (&useq, 0x7532, 0x7535, 0);

	printf (" --- sequence ---  count = %d\n",
			uget_sequence_count (&useq, "http://sample/*-*-*.mp4"));
	uget_sequence_get_list (&useq, "http://sample/*-*-*.mp4", &list);
	for (link = list.head;  link;  link = link->next)
		puts (link->data);
	uget_sequence_clear_result(&list);

	puts (" --- preview ---");
	uget_sequence_get_preview (&useq, "http://sample/*-*.mp4", &list);
	for (link = list.head;  link;  link = link->next)
		puts (link->data);
	uget_sequence_clear_result(&list);

	uget_sequence_final (&useq);
}

// ----------------------------------------------------------------------------
// UgetFiles

void test_files_json(UgetFiles* files)
{
	UgJson       json;
	UgBuffer     buffer;
	UgJsonError  code;
	UgetFiles*   files2;

	ug_json_init(&json);
	ug_buffer_init(&buffer, 4096);

	// write
	ug_json_begin_write(&json, UG_JSON_FORMAT_INDENT, &buffer);
	ug_json_write_object_head(&json);
    ug_json_write_entry(&json, files, files->info->entry);
	ug_json_write_object_tail(&json);
	ug_json_end_write(&json);
	ug_buffer_write_char(&buffer, '\0');
	puts("\n--- print files ---");
	puts(buffer.beg);

	files2 = ug_data_new(UgetFilesInfo);
	// parse
	ug_json_begin_parse(&json);
	ug_json_push(&json, ug_json_parse_entry, files2, (void*)files2->info->entry);
	ug_json_push(&json, ug_json_parse_object, NULL, NULL);
	code = ug_json_parse(&json, buffer.beg, -1);
	ug_json_end_parse(&json);
	printf("\n" "parse return %d\n", code);

	// write
	buffer.cur = buffer.beg;
	ug_json_begin_write(&json, UG_JSON_FORMAT_INDENT, &buffer);
    ug_json_write_entry(&json, files2, files2->info->entry);
	ug_json_end_write(&json);
	ug_buffer_write_char(&buffer, '\0');
	puts("\n--- print files2 ---");
	puts(buffer.beg);

	ug_data_free(files2);

	ug_buffer_clear(&buffer, TRUE);
	ug_json_final(&json);
}

void test_files(void)
{
	UgetFiles* files;
	UgetFiles* src;
	UgetFile*  element;

	files = ug_data_new(UgetFilesInfo);
	src   = ug_data_new(UgetFilesInfo);

	element = uget_files_realloc(files, "0.mp4");
	element = uget_files_realloc(files, "1.mp4");
	element = uget_files_realloc(files, "2.mp4");
	element = uget_files_realloc(files, "xyz.mp4");

	test_files_json(files);

	element = uget_files_realloc(src, "0.mp4");
	element = uget_files_realloc(src, "1.mp4");
	element = uget_files_realloc(src, "2.mp4");
	element = uget_files_realloc(src, "3.mp4");

	element = uget_files_realloc(src, "foo.mp4");
	element = uget_files_realloc(src, "xxx.apk");

	uget_files_sync(files, src);
	test_files_json(files);

	element->state |= UGET_FILE_STATE_DELETED;
	src->sync_count++;

	uget_files_sync(files, src);
	test_files_json(files);

	ug_data_free(files);
	ug_data_free(src);
}

// ----------------------------------------------------------------------------
// main

int main (void)
{
//	test_uget_node ();
//	test_fake_path ();

//	test_uget_a2cf ();
//	test_uget_curl ();
//	test_uget_rss ();
//	test_seq ();
	test_files();

	return 0;
}

