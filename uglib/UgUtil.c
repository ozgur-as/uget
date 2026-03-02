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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdarg.h>
#include <stdlib.h>     // strtoul()
#include <wchar.h>      // wcslen()
#include <UgDefine.h>
#include <UgUtil.h>
#include <UgString.h>   // ug_strdup()

#if defined _WIN32 || defined _WIN64
#include <windows.h>
//#include <PowrProf.h>   // SetSuspendState()
#else
#include <stdio.h>      // popen()
#include <unistd.h>
#include <sys/time.h>
#endif

// ----------------------------------------------------------------------------
// Time

uint64_t ug_get_time_count (void)
{
#if defined _WIN32 || defined _WIN64
	return (uint64_t) GetTickCount ();
#else
    struct timeval tv;

    gettimeofday(&tv, NULL);

    return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
#endif
}

// ----------------------------------------------------------------------------
// Unicode

int  ug_utf8_get_invalid (const char* input, char* ch)
{
	int            nb = 0, na;
	const uint8_t *c;

	for (c = (uint8_t*) input;  *c;  c += (nb + 1)) {
		if (!(*c & 0x80))
			nb = 0;
		else if ((*c & 0xc0) == 0x80) {
			if (ch)
				*ch = *c;
			return (intptr_t)c - (intptr_t)input;
		}
		else if ((*c & 0xe0) == 0xc0)
			nb = 1;
		else if ((*c & 0xf0) == 0xe0)
			nb = 2;
		else if ((*c & 0xf8) == 0xf0)
			nb = 3;
		else if ((*c & 0xfc) == 0xf8)
			nb = 4;
		else if ((*c & 0xfe) == 0xfc)
			nb = 5;

		na = nb;
		while (na-- > 0) {
			if ((*(c + nb) & 0xc0) != 0x80) {
				if (ch)
					*ch = *(c + nb);
				return (intptr_t)(c + nb) - (intptr_t)input;
			}
		}
	}
	return -1;
}

// 0xC0: Start of a 2-byte sequence
static const uint8_t  utf8Limits[] = {0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

uint16_t*  ug_utf8_to_utf16 (const char* string, int count, int* utf16len)
{
	uint8_t     ch;
	uint16_t*   result;
	uint16_t*   dest;
	uint32_t    value;
	const char* end;

	if (count == -1)
		count  = strlen (string);
	end  = string + count;
	result = ug_malloc (sizeof (uint16_t) * (count+1) );
	dest   = result;

	while (string < end) {
		ch = *string++;

		if(ch < 0x80) { // 0-127, US-ASCII (single byte)
			*dest++ = (uint16_t)ch;
			continue;
		}

		if(ch < 0xC0) // The first octet for each code point should within 0-191
			break;

		for(count = 1;  count < 5;  count++)
			if(ch < utf8Limits[count])
				break;
		value = ch - utf8Limits[count - 1];

		do {
			uint8_t  ch2;

			if (string >= end)  //  || string[0] == 0
				break;
			ch2 = *string++;
			if (ch2 == 0)
				break;
			if(ch2 < 0x80 || ch2 >= 0xC0)
				break;
			value <<= 6;
			value |= (ch2 - 0x80);
		} while(--count != 0);

		if(value < 0x10000) {
			*dest++ = (uint16_t) value;
		}
		else {
			value -= 0x10000;
			if(value >= 0x100000)
				break;
			*dest++ = (uint16_t) (0xD800 + (value >> 10));
			*dest++ = (uint16_t) (0xDC00 + (value & 0x3FF));
		}
	}

	if (utf16len)
		*utf16len = dest - result;
	*dest++ = 0;
	return result;
}

char*   ug_utf16_to_utf8 (const uint16_t* string, int count, int* utf8len)
{
	uint16_t    ch;
	uint8_t*    result;
	uint8_t*    dest;
	const uint16_t* end;

	if (count == -1)
		count  = wcslen ((wchar_t*) string);
	end  = string + count;
	result = ug_malloc (sizeof (uint8_t) * (count+1) * 3);
	dest   = result;

	while (string < end) {
		ch = *string++;
		if (ch >= 1 && ch <= 0x7F)
			*dest++ = (uint8_t) ch;
		else if (ch > 0x7FF) {
			*dest++ = (uint8_t) (0xE0 | ((ch >> 12) & 0x0F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  6) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  0) & 0x3F));
		}
		else {
			*dest++ = (uint8_t) (0xC0 | ((ch >>  6) & 0x1F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  0) & 0x3F));
		}
	}

	if (utf8len)
		*utf8len = dest - result;
	*dest++ = 0;
	return (char*)result;
}

uint32_t*  ug_utf8_to_ucs4 (const char* string, int count, int* ucs4len)
{
	uint8_t     ch;
	uint32_t*   result;
	uint32_t*   dest;
	uint32_t    value;
	const char* end;

	if (count == -1)
		count  = strlen (string);
	end  = string + count;
	result = ug_malloc (sizeof (uint32_t) * (count+1) );
	dest   = result;

	while (string < end) {
		ch = *string++;

		if(ch < 0x80) { // 0-127, US-ASCII (single byte)
			*dest++ = (uint32_t)ch;
			continue;
		}

		if(ch < 0xC0) // The first octet for each code point should within 0-191
			break;

		for(count = 1;  count < 5;  count++)
			if(ch < utf8Limits[count])
				break;
		value = ch - utf8Limits[count - 1];

		do {
			uint8_t  ch2;

			if (string >= end)  //  || string[0] == 0
				break;
			ch2 = *string++;
			if (ch2 == 0)
				break;
			if(ch2 < 0x80 || ch2 >= 0xC0)
				break;
			value <<= 6;
			value |= (ch2 - 0x80);
		} while(--count != 0);

		*dest++ = value;
	}

	if (ucs4len)
		*ucs4len = dest - result;
	*dest++ = 0;
	return result;
}

char*   ug_ucs4_to_utf8 (const uint32_t* string, int count, int* utf8len)
{
	uint32_t    ch;
	uint8_t*    result;
	uint8_t*    dest;
	const uint32_t* end;

	if (count == -1) {
		for (count = 0;  string[count] != 0;  count++)
			;
	}
	end  = string + count;
	result = ug_malloc (sizeof (uint8_t) * (count+1) * 6);
	dest   = result;

	while (string < end) {
		ch = *string++;
		if (ch >= 1 && ch <= 0x7F)
			*dest++ = (uint8_t) ch;
		else if (ch > 0x3FFFFFF) {
			*dest++ = (uint8_t) (0xFC | ((ch >> 30) & 0x01));
			*dest++ = (uint8_t) (0x80 | ((ch >> 24) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >> 18) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >> 12) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  6) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  0) & 0x3F));
		}
		else if (ch > 0x10FFFF) {
			*dest++ = (uint8_t) (0xF8 | ((ch >> 24) & 0x03));
			*dest++ = (uint8_t) (0x80 | ((ch >> 18) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >> 12) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  6) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  0) & 0x3F));
		}
		else if (ch > 0xFFFF) {
			*dest++ = (uint8_t) (0xF0 | ((ch >> 18) & 0x07));
			*dest++ = (uint8_t) (0x80 | ((ch >> 12) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  6) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  0) & 0x3F));
		}
		else if (ch > 0x7FF) {
			*dest++ = (uint8_t) (0xE0 | ((ch >> 12) & 0x0F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  6) & 0x3F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  0) & 0x3F));
		}
		else {
			*dest++ = (uint8_t) (0xC0 | ((ch >>  6) & 0x1F));
			*dest++ = (uint8_t) (0x80 | ((ch >>  0) & 0x3F));
		}
	}

	if (utf8len)
		*utf8len = dest - result;
	*dest++ = 0;
	return (char*)result;
}

// ----------------------------------------------------------------------------
// Base64

static int  mod_table[] = {0, 2, 1};
static char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static char *decoding_table = NULL;

static void ug_base64_build_decoding_table()
{
	int i;
	decoding_table = ug_malloc(256);

	for (i = 0; i < 64; i++)
		decoding_table[(unsigned char) encoding_table[i]] = i;
}

void ug_base64_cleanup()
{
	ug_free (decoding_table);
	decoding_table = NULL;
}

char* ug_base64_encode (const unsigned char* data,
                        int  input_length,
                        int* output_length)
{
	int   i, j;
	int   length;
	char* encoded_data;

	length = 4 * ((input_length + 2) / 3);
	encoded_data = (char*) ug_malloc (length + 1);  // + '\0'
	if (encoded_data == NULL)
		return NULL;

	for (i = 0, j = 0; i < input_length;) {
		uint32_t octet_a = i < input_length ? data[i++] : 0;
		uint32_t octet_b = i < input_length ? data[i++] : 0;
		uint32_t octet_c = i < input_length ? data[i++] : 0;

		uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

		encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
		encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
	}

	for (i = 0; i < mod_table[input_length % 3]; i++)
		encoded_data[length - 1 - i] = '=';

	if (output_length)
		*output_length = length;
	encoded_data[length] = '\0';
	return encoded_data;
}

unsigned char* ug_base64_decode (const char* data,
                                 int  input_length,
                                 int* output_length)
{
	int  i, j;
	int  length, pad_len;
	unsigned char *decoded_data;
	uint32_t sextet_a, sextet_b, sextet_c, sextet_d, triple;

	if (decoding_table == NULL)
		ug_base64_build_decoding_table ();

#if 1
	// for removed trailing "==" or "="
	pad_len = input_length & 3;  // pad_len = input_length % 4;
	if (pad_len > 0) {
		pad_len = 4 - pad_len;
		if (pad_len > 2)
			return NULL;
	}

	length = (input_length + pad_len) / 4 * 3;
	if (pad_len > 0)
		length -= pad_len;
	else {
		if (data[input_length - 1] == '=')
			length--;
		if (data[input_length - 2] == '=')
			length--;
	}
#else
	if (input_length % 4 != 0)
		return NULL;

	length = input_length / 4 * 3;
	if (data[input_length - 1] == '=')
		length--;
	if (data[input_length - 2] == '=')
		length--;
#endif

	decoded_data = ug_malloc (length + 1);  // + '\0' for decoded text data
	if (decoded_data == NULL)
		return NULL;

	for (i = 0, j = 0;  i < input_length;) {
		sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[(uint8_t)data[i++]];
		sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[(uint8_t)data[i++]];
		if (i == input_length)  // for removed trailing "=="
			sextet_c = 0;
		else
			sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[(uint8_t)data[i++]];
		if (i == input_length)  // for removed trailing "="
			sextet_d = 0;
		else
			sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[(uint8_t)data[i++]];

		triple  = (sextet_a << 3 * 6)
				+ (sextet_b << 2 * 6)
				+ (sextet_c << 1 * 6)
				+ (sextet_d << 0 * 6);

		if (j < length)
			decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
		if (j < length)
			decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
		if (j < length)
			decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
	}

	if (output_length)
		*output_length = length;
	decoded_data[length] = '\0';  // + '\0' for decoded text data
	return decoded_data;
}

// ----------------------------------------------------------------------------
// filename and path functions

char* ug_build_filename (const char* first_element, ...)
{
	va_list     arg_list;
	int         total_len;
	const char* temp;
	char*       result;

	// count total length
	total_len = 0;
	temp = first_element;
	va_start (arg_list, first_element);
	do {
		total_len += strlen (temp) + 1;  // + '/'
		temp = va_arg (arg_list, char*);
	} while (temp);
	va_end (arg_list);

	// concat path element
	result = ug_malloc (total_len + 1);     // + '\0'
	result[0] = 0;
	temp = first_element;
	va_start (arg_list, first_element);
	do {
		if (temp != first_element) {
#if defined _WIN32 || defined _WIN64
			if (strchr (temp, '\\') == NULL)
				strcat (result, "\\");
#else
			if (strchr (temp, '/') == NULL)
				strcat (result, "/");
#endif // _WIN32 || _WIN64
		}
		strcat (result, temp);
		temp = va_arg (arg_list, char*);
	} while (temp);
	va_end (arg_list);

	return result;
}

// ----------------------------------------------------------------------------
// Power Management

#if defined _WIN32 || defined _WIN64

static int  ug_win32_get_shutdown_privilege ()
{
	HANDLE hToken;
	TOKEN_PRIVILEGES tkp;

	if (OpenProcessToken (GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES|TOKEN_QUERY, &hToken))
	{
		LookupPrivilegeValue (NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid);
		tkp.PrivilegeCount = 1;
		tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
		AdjustTokenPrivileges (hToken, FALSE, &tkp, 0, (PTOKEN_PRIVILEGES)NULL, 0);
		return TRUE;
	}
	return FALSE;
}

void  ug_reboot (void)
{
	ug_win32_get_shutdown_privilege ();
	ExitWindowsEx (EWX_SHUTDOWN | EWX_REBOOT, 0);
}

void  ug_shutdown (void)
{
	ug_win32_get_shutdown_privilege ();
	ExitWindowsEx (EWX_SHUTDOWN | EWX_POWEROFF, 0);
}

void  ug_suspend (void)
{
	system ("powercfg -hibernate off");
//	SetSuspendState (0, 1, 0);
	system ("rundll32 powrprof.dll,SetSuspendState 0,1,0");
	system ("powercfg -hibernate on");
}

void  ug_hibernate (void)
{
//	SetSuspendState (1, 0, 0);
	system ("rundll32 powrprof.dll,SetSuspendState 1,0,0");
}

#else

// Wrapper to suppress warn_unused_result for fire-and-forget commands
static inline void run_cmd (const char* cmd)
{
	int ret = system (cmd);
	(void) ret;
}

void  ug_reboot (void)
{
	run_cmd ("reboot");

	// old system
	run_cmd ("dbus-send --system --print-reply "
	               "--dest=\"org.freedesktop.ConsoleKit\" "
	               "/org/freedesktop/ConsoleKit/Manager "
	               "org.freedesktop.ConsoleKit.Manager.Restart");

	run_cmd ("dbus-send --system --print-reply "
	               "--dest=org.freedesktop.login1 "
	               "/org/freedesktop/login1 "
	               "org.freedesktop.login1.Manager.Reboot "
	               "boolean:true");
}

void  ug_shutdown (void)
{
	run_cmd ("poweroff");

	// old system
	run_cmd ("dbus-send --system --print-reply "
	               "--dest=\"org.freedesktop.ConsoleKit\" "
	               "/org/freedesktop/ConsoleKit/Manager "
	               "org.freedesktop.ConsoleKit.Manager.Stop");

	run_cmd ("dbus-send --system --print-reply "
	               "--dest=org.freedesktop.login1 "
	               "/org/freedesktop/login1 "
	               "org.freedesktop.login1.Manager.PowerOff "
	               "boolean:true");
}

void  ug_suspend (void)
{
	run_cmd ("pm-suspend");

	// old system
	run_cmd ("dbus-send --system --print-reply "
	               "--dest=\"org.freedesktop.UPower\" "
	               "/org/freedesktop/UPower "
	               "org.freedesktop.UPower.Suspend");

	run_cmd ("dbus-send --system --print-reply "
	               "--dest=org.freedesktop.login1 "
	               "/org/freedesktop/login1 "
	               "org.freedesktop.login1.Manager.Suspend "
	               "boolean:true");
}

void  ug_hibernate (void)
{
	run_cmd ("pm-hibernate");

	// old system
	run_cmd ("dbus-send --system --print-reply "
	               "--dest=\"org.freedesktop.UPower\" "
	               "/org/freedesktop/UPower "
	               "org.freedesktop.UPower.Hibernate");

	run_cmd ("dbus-send --system --print-reply "
	               "--dest=org.freedesktop.login1 "
	               "/org/freedesktop/login1 "
	               "org.freedesktop.login1.Manager.Hibernate "
	               "boolean:true");

	// if system can't hibernate, try to suspend
	ug_suspend ();
}

#endif // _WIN32 || _WIN64

// ----------------------------------------------------------------------------
// Others

#if defined _WIN32 || defined _WIN64

char*  ug_sys_release (void)
{
	OSVERSIONINFO  info;

	memset (&info, 0, sizeof (info));
	info.dwOSVersionInfoSize = sizeof (info);
	GetVersionEx (&info);
	return ug_strdup_printf ("Windows-%u.%u",
			(unsigned) info.dwMajorVersion,
			(unsigned) info.dwMinorVersion);
}

#else

// lsb_release -i
// Distributor ID:
// lsb_release -r
// Release:
char* ug_sys_release (void)
{
	FILE*  file;
	char*  buf;
	char*  dist = NULL;
	char*  release = NULL;

	file = popen ("lsb_release -a", "r");
	if (file == NULL)
		return ug_strdup ("Unknown");
	buf = ug_malloc (80);
	while (fgets (buf, 80, file) != NULL) {
		if (strncmp (buf, "Distributor ID:", 15) == 0) {
			for (dist = buf + 15;  dist[0] == '\t';  dist++);
			dist = ug_strndup (dist, strcspn (dist, "\n"));
		}
		else if (strncmp (buf, "Release:", 8) == 0) {
			for (release = buf + 8;  release[0] == '\t';  release++);
			release = ug_strndup (release, strcspn (release, "\n"));
		}
	}
	ug_free (buf);
	pclose (file);

	if (dist && release)
		buf = ug_strdup_printf ("%s-%s", dist, release);
	else
		buf = ug_strdup ("Unknown");
	ug_free (dist);
	ug_free (release);
	return buf;
}

#endif // _WIN32 || _WIN64
