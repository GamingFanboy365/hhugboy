/*
   hhugboy Game Boy emulator
   Copyright (C) 2026 the hhugboy contributors

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   as published by the Free Software Foundation; either version 2
   of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

// Platform compatibility layer.
//
// The emulator core was written against the Win32 API. On Windows this header
// just pulls in windows.h as before. Everywhere else it provides the small set
// of Win32 types and functions the core relies on (wide-character file access,
// current directory handling, wsprintfW, Sleep...) implemented on top of POSIX,
// so the core can be shared between the Windows and non-Windows frontends.
//
// Wide strings are UTF-16 on Windows and UTF-32 elsewhere; on non-Windows
// systems filenames are converted to UTF-8 before they reach the OS.

#ifndef HHUGBOY_COMPAT_H
#define HHUGBOY_COMPAT_H

#include <string>

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>

#define HHUGBOY_PATH_SEPARATOR L'\\'
#define HHUGBOY_PATH_SEPARATOR_STR L"\\"

#else // !_WIN32

#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <sys/stat.h>

#include "wintypes.h"

typedef unsigned char BYTE;
typedef unsigned int UINT;
typedef int BOOL;
typedef void* HWND;
typedef void* HINSTANCE;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#define __cdecl

#define HHUGBOY_PATH_SEPARATOR L'/'
#define HHUGBOY_PATH_SEPARATOR_STR L"/"

// File access with wide filenames
FILE* _wfopen(const wchar_t* filename, const wchar_t* mode);

#define _stat stat
int _wstat(const wchar_t* path, struct stat* buffer);

// Directories
BOOL SetCurrentDirectoryW(const wchar_t* path);
DWORD GetCurrentDirectoryW(DWORD bufferLength, wchar_t* buffer);
BOOL CreateDirectoryW(const wchar_t* path, void* securityAttributes);

#define SetCurrentDirectory SetCurrentDirectoryW
#define GetCurrentDirectory GetCurrentDirectoryW
#define CreateDirectory CreateDirectoryW

// wsprintfW follows Win32 semantics: %s and %c take wide arguments.
// Like the real thing, the output is limited to 1024 characters.
int wsprintfW(wchar_t* buffer, const wchar_t* format, ...);
#define wsprintf wsprintfW

void Sleep(DWORD milliseconds);

#define ZeroMemory(destination, length) memset((destination), 0, (length))

#endif // _WIN32

// UTF-8 <-> wide string conversion, available on all platforms
std::string wideToUtf8(const std::wstring& wide);
std::wstring utf8ToWide(const std::string& utf8);

#endif // HHUGBOY_COMPAT_H
