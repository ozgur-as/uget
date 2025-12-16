/*
 *
 *   Copyright (C) 2005-2020 by C.H. Huang
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

//  Support LFS(Large File Support) and UTF-8 filename
//  It can compile with MS Visual C++, Mingw, Unix...etc.

// To enable LFS (Large File Support) in UNIX platform
// add `getconf LFS_CFLAGS`  to CFLAGS
// add `getconf LFS_LDFLAGS` to LDFLAGS

#ifndef UG_STDIO_H
#define UG_STDIO_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#if _MSC_VER < 1900
#ifndef snprintf
#define snprintf	_snprintf
#endif // snprintf
#endif // _MSC_VER < 1900
#endif // _MSC_VER

#ifdef HAVE_GLIB
#include <glib.h>
#include <glib/gstdio.h>
#endif

#if defined _WIN32 || defined _WIN64
#include <io.h>
#else
#include <unistd.h>
#endif

#include <stdint.h>
#include <fcntl.h>       // for O_* flags
//#include <sys/types.h>
#include <sys/stat.h>    // for S_* mode
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

// To enable LFS (Large File Support) in UNIX platform
// add `getconf LFS_CFLAGS`  to CFLAGS
// add `getconf LFS_LDFLAGS` to LDFLAGS

// in Linux
// gcc -D_FILE_OFFSET_BITS=64
// define _FILE_OFFSET_BITS  64

// in Android
// use lseek64()

// ----------------------------------------------------------------------------
// low level file I/O
// wrapper functions/definitions for file descriptor.

// redefine flags: O_APPEND, O_RDONLY, O_WRONLY, O_RDWR, O_CREAT, O_EXCL
// redefine mode: S_IREAD, S_IWRITE
#if defined _WIN32 || defined _WIN64
   // flags
#  define UG_O_APPEND    _O_APPEND
#  define UG_O_BINARY    _O_BINARY   // Text is the default in MS platform
#  define UG_O_TEXT      _O_TEXT     // Text is the default in MS platform
#  define UG_O_RDONLY    _O_RDONLY
#  define UG_O_WRONLY    _O_WRONLY
#  define UG_O_RDWR      _O_RDWR
#  define UG_O_CREAT     _O_CREAT
#  define UG_O_EXCL      _O_EXCL     // Use with _O_CREAT, return error if file exist.
#  define UG_O_TRUNC     _O_TRUNC    // Use with O_RDWR or O_WRONLY, Truncate the file if it does exist.
   // mnemonic flags
#  define UG_O_READONLY  _O_RDONLY
#  define UG_O_WRITEONLY _O_WRONLY
#  define UG_O_READWRITE _O_RDWR
#  define UG_O_CREATE    _O_CREAT
#  define UG_O_TRUNCATE  _O_TRUNC
   // mode
#  define UG_S_IREAD     _S_IREAD    // Use with _O_CREAT
#  define UG_S_IWRITE    _S_IWRITE   // Use with _O_CREAT
#  define UG_S_IRUSR     _S_IREAD    // Use with _O_CREAT
#  define UG_S_IWUSR     _S_IWRITE   // Use with _O_CREAT
#  define UG_S_IXUSR     0           // Use with _O_CREAT
#  define UG_S_IRGRP     0           // Use with _O_CREAT
#  define UG_S_IWGRP     0           // Use with _O_CREAT
#  define UG_S_IROTH     0           // Use with _O_CREAT
#  define UG_S_IWOTH     0           // Use with _O_CREAT
#else
   // flags
#  define UG_O_APPEND    O_APPEND
#  ifdef O_BINARY
#    define UG_O_BINARY  O_BINARY    // Text is the default in MS platform
#    define UG_O_TEXT    O_TEXT      // Text is the default in MS platform
#  else
#    define UG_O_BINARY  0           // Text is the default in MS platform
#    define UG_O_TEXT    0           // Text is the default in MS platform
#  endif
#  define UG_O_RDONLY    O_RDONLY
#  define UG_O_WRONLY    O_WRONLY
#  define UG_O_RDWR      O_RDWR
#  define UG_O_CREAT     O_CREAT
#  define UG_O_EXCL      O_EXCL      // Use with O_CREAT, return error if file exist.
#  define UG_O_TRUNC     O_TRUNC     // Use with O_RDWR or O_WRONLY
   // mnemonic flags
#  define UG_O_READONLY  O_RDONLY
#  define UG_O_WRITEONLY O_WRONLY
#  define UG_O_READWRITE O_RDWR
#  define UG_O_CREATE    O_CREAT
#  define UG_O_TRUNCATE  O_TRUNC
   // mode
#  ifndef S_IREAD
#    define S_IREAD      S_IRUSR
#  endif
#  ifndef S_IWRITE
#    define S_IWRITE     S_IWUSR
#  endif
#  define UG_S_IREAD     S_IREAD     // Use with O_CREAT
#  define UG_S_IWRITE    S_IWRITE    // Use with O_CREAT
#  define UG_S_IRUSR     S_IRUSR     // Use with O_CREAT
#  define UG_S_IWUSR     S_IWUSR     // Use with O_CREAT
#  define UG_S_IXUSR     S_IXUSR     // Use with O_CREAT
#  define UG_S_IRGRP     S_IRGRP     // Use with O_CREAT // GROUP READ
#  define UG_S_IWGRP     S_IWGRP     // Use with O_CREAT // GROUP WRITE
#  define UG_S_IROTH     S_IROTH     // Use with O_CREAT // OTHERS READ
#  define UG_S_IWOTH     S_IWOTH     // Use with O_CREAT // OTHERS WRITE
#endif

// creat(path, mode) == open (path, O_WRONLY|O_CREAT|O_TRUNC, mode)
// Returns :  a new file descriptor, or -1 if an error occurred.
#if defined _WIN32 || defined _WIN64 || defined HAVE_GLIB || defined USE__ANDROID__SAF
int  ug_open (const char* filename_utf8, int flags, int mode);
int  ug_creat (const char* filename_utf8, int mode);
#else
#  define ug_open       open
#  define ug_creat      creat
#endif

#if defined _WIN32 || defined _WIN64
int  ug_truncate (int fd, int64_t length);
#endif

// ug_read() return 0 if end-of-file. return -1 on error.
#if defined _WIN32 || defined _WIN64
#  define  ug_close     _close
#  define  ug_read      _read
#  define  ug_write     _write
#  define  ug_sync      _commit
#  define  ug_seek      _lseeki64   // for MS VC
#  define  ug_tell      _telli64    // for MS VC
#else
#  define  ug_close     close
#  define  ug_read      read
#  define  ug_write     write
#  define  ug_sync      fsync
#  if defined __ANDROID__
#    define  ug_seek      lseek64
#    define  ug_tell(fd)  lseek64(fd, 0L, SEEK_CUR)
#    if __ANDROID_API__ >= 12
#      define  ug_truncate  ftruncate64 // Android API level 12+
#    else
#      define  ug_truncate  ftruncate
#    endif
#  else
#    define  ug_seek      lseek
#    define  ug_tell(fd)  lseek(fd, 0L, SEEK_CUR)
#    define  ug_truncate  ftruncate
#  endif
#endif

// ------------------------------------------------------------------
// streaming file I/O
// wrapper functions/definitions for file stream. (struct FILE)

//#if defined (USE_FILE32API)   // iOS
//#  define fopen64  fopen
//#  define ftello64 ftell
//#  define fseeko64 fseek
//#endif

#if defined _WIN32 || defined _WIN64 || defined HAVE_GLIB || defined USE__ANDROID__SAF
FILE* ug_fopen (const char *filename_utf8, const char *mode);
#else
#  define ug_fopen      fopen
#endif

// FILE* fdopen(int fd, const char *mode);
#define ug_fdopen       fdopen

#if defined __ANDROID__
int     fseek_64 (FILE *stream, int64_t offset, int origin);
int64_t ftell_64 (FILE *stream);
#endif  // __ANDROID__

int   ug_ftruncate (FILE* file, int64_t size);

#if defined _WIN32 || defined _WIN64
#  if defined _MSC_VER
#    define ug_fseek                _fseeki64
#    define ug_ftell                _ftelli64
#  elif defined __MINGW32__
#    define ug_fseek                fseeko64
#    define ug_ftell                ftello64
#  else
#    define ug_fseek                fseek
#    define ug_ftell                ftell
#  endif
#  define ug_fileno                 _fileno
#else
#  if defined __ANDROID__
#    define ug_fseek                fseek_64
#    define ug_ftell                ftell_64
#  else
#    define ug_fseek                fseek
#    define ug_ftell                ftell
#  endif
#  define ug_fileno                 fileno
#endif

#define ug_fclose                   fclose
#define ug_fread(file,data,len)     fread  (data, 1, len, file)
#define ug_fwrite(file,data,len)    fwrite (data, 1, len, file)
#define ug_fprintf                  fprintf
#define ug_fputs(file,string)       fputs (string, file)
#define ug_fputc(file,character)    fputc (character, file)
#define	ug_fgets(file,buf,len)      fgets (buf, len, file)
#define	ug_fgetc(file)              fgetc (file)
#define ug_fflush                   fflush
#define ug_fdget                    ug_fileno

// ----------------------------------------------------------------------------
// file & directory functions: these functions returns 0 if it is successful.

// ug_rename() renames a file, moving it between directories if required.
// ug_remove() deletes a name from the filesystem, it works for both files and directories.
// ug_unlink() If this was the last link to the file and no processes have it opened, the diskspace occupied by the file is freed.

#if defined _WIN32 || defined _WIN64 || defined HAVE_GLIB || defined USE__ANDROID__SAF
int  ug_rename (const char *old_file_utf8, const char *new_file_utf8);
int  ug_remove (const char *file_utf8);
int  ug_unlink (const char *file_utf8);
#else
#  define ug_rename             rename
#  define ug_remove             remove
#  define ug_unlink             unlink
#endif

#ifdef __cplusplus
}
#endif

#endif  // UG_STDIO_H
