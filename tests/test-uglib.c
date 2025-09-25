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
#include <stdint.h>
#include <string.h>
#include <UgUri.h>
#include <UgNode.h>
#include <UgUtil.h>
#include <UgBuffer.h>
#include <UgOption.h>
#include <UgList.h>
#include <UgSLink.h>
#include <UgString.h>
#include <UgHtml.h>

#if defined _WIN32 || defined _WIN64
#include <UgUtil.h>
#include <windows.h>
#include <shellapi.h>  // ShellExecuteW()

void  test_launch (void)
{
	uint16_t*  pathutf16;
	uint16_t*  parmutf16;
	int        result;

	pathutf16 = ug_utf8_to_utf16 ("C:\\Program Files\\uGet\\bin\\aria2c", -1, NULL);
	parmutf16 = ug_utf8_to_utf16 ("--enable-rpc=true", -1, NULL);

	wprintf (L"%s %s", pathutf16, parmutf16);

	result = (int)ShellExecuteW (NULL, L"open", pathutf16, parmutf16,
	                             NULL, SW_HIDE);
	if (result > 32)
		puts ("ShellExecuteW - OK");
	free (parmutf16);
	free (pathutf16);
}
#else
void  test_launch (void)
{
}
#endif

// ----------------------------------------------------------------------------
// test UgUri

void  dump_uri (UgUri* uri)
{
}

void  test_uri (void)
{
	char*  temp;
	char*  hosts[]   = {"ftp.you.com", ".your.org", ".edu", NULL};
	char*  exts[]    = {"png", "bmp", "jpg", NULL};
	char*  schemes[] = {"ftp", "http", "git", NULL};
//	const char*  uri   = "ftp://i.am.ftp.you.com/file.bmp";
	const char*  uri   = "http://my.and.your.org/file%200.png?field1=value1&field2=value2&field3";
//	const char*  uri   = "git://this.edu/file.jpg";
	const char*  query;
	UgUriQuery   uquery;
	UgUri  uuri;
	int    index;

	puts ("\n--- test_uri:");

	ug_uri_init (&uuri, uri);
	index = ug_uri_match_hosts (&uuri, hosts);
	printf ("ug_uri_match_hosts () return %d\n", index);
	index = ug_uri_match_schemes (&uuri, schemes);
	printf ("ug_uri_match_schemes () return %d\n", index);
	index = ug_uri_match_file_exts (&uuri, exts);
	printf ("ug_uri_match_file_exts () return %d\n", index);

	temp = ug_uri_get_file (&uuri);
	puts (temp);
	ug_free (temp);

	query = uri + uuri.query;
	while ((index = ug_uri_query_part (&uquery, query)) > 0) {
        printf ("ug_uri_query_part() return %d, %.*s=%.*s\n",
		        index,
		        uquery.field_len, query,
		        uquery.value_len, uquery.value);
		query = uquery.field_next;
	}

	temp = ug_strdup ("file%2020%%20100%2f%20.jpg");
	index = ug_decode_uri (temp, -1, temp);
	printf ("ug_decode_uri() return %d, output - '%s'\n", index, temp);
	ug_free (temp);
}

// ----------------------------------------------------------------------------
// test UgList

static UgLink  ug_links[4];

void  test_list (void)
{
	UgLink*    temp;
	UgList     list;
	uintptr_t  index;

	ug_list_init (&list);
        for (index = 0;  index < 4;  index++)
                ug_links[index].data = (void*) index;

        ug_list_append (&list, ug_links + 0);
        ug_list_append (&list, ug_links + 1);
        ug_list_append (&list, ug_links + 2);
        ug_list_append (&list, ug_links + 3);

        ug_list_remove (&list, ug_links + 2);
        ug_list_insert (&list, ug_links + 3, ug_links + 2);

	for (temp = list.head;  temp;  temp = temp->next) {
		printf ("%d, %p, %p\n",
				(int)(intptr_t)temp->data,
				temp->prev, temp->next);
	}
}

// ----------------------------------------------------------------------------
// test UgNode

void  dump_node (UgNode* root)
{
	UgNode*	cur;

	printf ("node->next : %p\n", root->next);
	printf ("node->prev : %p\n", root->prev);
	printf ("node->parent : %p\n", root->parent);
	printf ("node->children : %p\n", root->children);
	printf ("node->n_children : %d\n", root->n_children);
	for (cur = root->children;  cur;  cur = cur->next)
		printf ("%u\n", (unsigned)(uintptr_t)cur->data);
}

void  test_node (void)
{
	UgNode*	root;
	UgNode*	node1;
	UgNode*	node2;
	UgNode*	node3;
	UgNode*	node4;

	puts ("\n--- test_node:");
	root = ug_node_new ();
	node1 = ug_node_new ();
	node1->data = (void*)(uintptr_t) 1;
	node2 = ug_node_new ();
	node2->data = (void*)(uintptr_t) 2;
	node3 = ug_node_new ();
	node3->data = (void*)(uintptr_t) 3;
	node4 = ug_node_new ();
	node4->data = (void*)(uintptr_t) 4;

	ug_node_append (root, node3);
	printf ("ug_node_append (3)\n");
	ug_node_prepend (root, node1);
	printf ("ug_node_prepend (1)\n");
	ug_node_insert (root, node3, node2);
	printf ("ug_node_insert (2) before 3\n");
	ug_node_append (root, node4);
	printf ("ug_node_append (4)\n");
	printf ("root.n_children : %d\n", root->n_children);
	dump_node (root);

	ug_node_unlink (node2);
	dump_node (node2);

	ug_node_free (root);
	ug_node_free (node1);
	ug_node_free (node2);
	ug_node_free (node3);
	ug_node_free (node4);
}

// ----------------------------------------------------------------------------
// UgBuffer

void  test_buffer ()
{
	UgBuffer  buffer;

	puts ("\n--- test_buffer:");
	ug_buffer_init (&buffer, 3);
	ug_buffer_write (&buffer, "This is test string.", -1);
	ug_buffer_write_char (&buffer, 'K');

	*ug_buffer_alloc (&buffer, 1) = 'S';
	*ug_buffer_alloc (&buffer, 1) = 'D';
	ug_buffer_write_char (&buffer, 'K');

	ug_buffer_write_char (&buffer, '\0');
	puts ((char*)buffer.beg);
	ug_buffer_clear (&buffer, 1);
}

// ----------------------------------------------------------------------------
// UgSLink

static void slinks_add_range (UgSLinks* slinks, char** strings,
                              int  beg, int  end)
{
	for (;  beg < end;  beg++) {
		strings[beg] = ug_strdup_printf ("%.3d", beg);
		ug_slinks_add (slinks, strings[beg]);
	}
}

static void slinks_remove_range (UgSLinks* slinks, char** strings,
                                 int  beg, int  end)
{
	for (;  beg < end;  beg++) {
		ug_slinks_remove (slinks, strings[beg], NULL);
		ug_free (strings[beg]);
		strings[beg] = NULL;
	}
}

void test_slink ()
{
	char**    strings;
	UgSLinks  slinks;

	puts ("\n--- test_slink:");
	ug_slinks_init (&slinks, 16);
	strings = ug_malloc0 (sizeof (char*) * 100);

	slinks_add_range (&slinks, strings, 40, 60);
	slinks_add_range (&slinks, strings,  0, 20);
	slinks_add_range (&slinks, strings, 80, 100);
	slinks_add_range (&slinks, strings, 20, 40);
	slinks_add_range (&slinks, strings, 60, 80);

	slinks_remove_range (&slinks, strings,  0, 10);
	slinks_remove_range (&slinks, strings, 90, 100);
	slinks_remove_range (&slinks, strings, 70, 90);
	slinks_remove_range (&slinks, strings, 10, 30);
	slinks_remove_range (&slinks, strings, 30, 50);
	slinks_remove_range (&slinks, strings, 50, 70);

	slinks_add_range (&slinks, strings,  0, 20);
	slinks_add_range (&slinks, strings, 80, 100);

	slinks_remove_range (&slinks, strings,  0, 10);
	slinks_remove_range (&slinks, strings, 90, 100);
	ug_slinks_foreach (&slinks, (void*)puts, NULL);
	slinks_remove_range (&slinks, strings, 10, 20);
	slinks_remove_range (&slinks, strings, 80, 90);

	ug_free (strings);
	ug_slinks_final (&slinks);
}

// ----------------------------------------------------------------------------
// UgUtil

void  test_base64 ()
{
	const char* test_string = "This is a test string.";
	char* base64;
	char* orig;
	int   len;

	puts ("\n--- test_base64:");
	base64 = ug_base64_encode ((uint8_t*)test_string, strlen (test_string), &len);
	printf ("%.*s\n", len, base64);
	puts (base64);

	orig = (char*) ug_base64_decode(base64, len, NULL);
	printf ("%.*s\n", len, orig);

	ug_free (base64);
	ug_free (orig);
}

// ----------------------------------------------------------------------------
// Utility

void  test_utility ()
{
	char*  temp;
	time_t res;
	int    n;

	res = ug_str_rfc822_to_time ("Sat, 07 Sep 2002 00:00:01 GMT");
	if (res != -1)
		puts (ctime (&res));

	res = ug_str_rfc3339_to_time ("2013-09-12T22:50:20+08:00");
	if (res != -1)
		puts (ctime (&res));

	temp = ug_build_filename ("basedir", "path", "file", NULL);
	printf ("ug_build_filename() - %s\n", temp);
	ug_free (temp);

	temp = ug_strdup ("\nThis\n one\r");
	puts ("--- ug_str_remove_crlf () --- start ---");
	puts (temp);
	n = ug_str_remove_crlf (temp, temp);
	puts (temp);

	printf ("--- ug_str_remove_crlf () return %d --- end ---", n);
	ug_free (temp);
}

// ----------------------------------------------------------------------------
// Option

struct Opt
{
	int   index;
	char* name;
};

UgOptionEntry opt_entry[] =
{
	{"category-index", "ci", offsetof (struct Opt, index), UG_ENTRY_INT, "", "=N", NULL},
	{"category-name",  "cn", offsetof (struct Opt, name),  UG_ENTRY_STRING, "", "=name", NULL},
	{NULL}
};

int  print_option_callback (UgOption* option,
                            const char* name,
                            const char* value,
                            void* dest, void* data)
{
    printf ("%s = %s\n", name, value);
    return TRUE;
}

void  test_option (void)
{
	UgOption    option;
	struct Opt  dest;

	ug_option_init (&option);
//	ug_option_set_parser (&option, print_option_callback, NULL, NULL);
	ug_option_set_parser (&option, ug_option_parse_entry, &dest, opt_entry);
	ug_option_parse (&option, "--category-index=1", -1);
	ug_option_parse (&option, "-cn=hhk", -1);
	ug_option_final (&option);
}

void test_cmd_arg (void)
{
	const char* cmd = "--enable --file=\"\" --arg  ";
	char** argv;
	int    argc;
	int    count;

	argv = ug_argv_from_cmd (cmd, &argc, 0);
	for (count = 0;  count < argc; count++)
		puts (argv[count]);
	ug_argv_free (argv);
}

// ----------------------------------------------------------------------------
// HTML

void start_element (UgHtml*        uhtml,
                    const char*    element_name,
                    const char**   attribute_names,
                    const char**   attribute_values,
                    void*          dest,
                    void*          data)
{
	int  index;

	printf ("<%s", element_name);
	for (index = 0;  attribute_names[index];  index++)
		printf (" %s=%s", attribute_names[index], attribute_values[index]);
	printf (">");
}

void end_element (UgHtml*        uhtml,
                  const char*    element_name,
                  void*          dest,
                  void*          data)
{
	printf ("</%s>\n", element_name);
}

void text (UgHtml*        uhtml,
           const char*    text,
           int            text_len,
           void*          dest,
           void*          data)
{
	printf ("%s", text);
}

UgHtmlParser testparser =
{
	start_element,
	end_element,
	text
};

void test_html (void)
{
	UgHtml* uhtml;

	uhtml = ug_html_new ();
	ug_html_begin_parse (uhtml);
	ug_html_push (uhtml, &testparser, NULL, NULL);
	ug_html_parse (uhtml, "< p attr1=\"value1\" attr2=val2/><test> &lt; &xxxxx; </test><s></s>", -1);
	ug_html_end_parse (uhtml);
	puts("\n");
}

void test_unicode (void)
{
	const char* utf8_name = "\xE9\xBB\x83\xE6\xAD\xA3\xE9\x9B\x84";
	char*       utf8_str;
	uint32_t*   ucs4_str;
	int   len;
	int   idx;

	ucs4_str = ug_utf8_to_ucs4 (utf8_name, -1, &len);
	for (idx = 0;  idx < len;  idx++)
		printf ("\\x%X", ucs4_str[idx]);
	puts ("");

	utf8_str = ug_ucs4_to_utf8 (ucs4_str,  -1, &len);
	for (idx = 0;  idx < len;  idx++)
		printf ("\\x%X", (unsigned)(uint8_t)utf8_str[idx]);
	puts ("");

	ug_free (ucs4_str);
	ug_free (utf8_str);
}

// ----------------------------------------------------------------------------
// main

int   main (void)
{
	test_unicode ();
	test_html ();
	test_cmd_arg ();
	test_option ();
	test_list ();
	test_node ();
	test_uri ();
	test_buffer ();
	test_slink ();
//	test_launch ();
	test_base64 ();
	test_utility ();

	return 0;
}

