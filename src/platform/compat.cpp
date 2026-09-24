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

#include "compat.h"

#ifdef _WIN32

std::string wideToUtf8(const std::wstring& wide)
{
    if(wide.empty()) return std::string();
    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, &wide[0], wide.size(), NULL, 0, NULL, NULL);
    std::string utf8(sizeNeeded, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wide[0], wide.size(), &utf8[0], sizeNeeded, NULL, NULL);
    return utf8;
}

std::wstring utf8ToWide(const std::string& utf8)
{
    if(utf8.empty()) return std::wstring();
    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, &utf8[0], utf8.size(), NULL, 0);
    std::wstring wide(sizeNeeded, 0);
    MultiByteToWideChar(CP_UTF8, 0, &utf8[0], utf8.size(), &wide[0], sizeNeeded);
    return wide;
}

#else // !_WIN32

#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>

// wchar_t is UTF-32 on the platforms we support besides Windows

std::string wideToUtf8(const std::wstring& wide)
{
    std::string utf8;
    utf8.reserve(wide.size());
    for(size_t i = 0; i < wide.size(); i++)
    {
        unsigned int c = (unsigned int)wide[i];
        if(c > 0x10FFFF || (c >= 0xD800 && c <= 0xDFFF))
            c = 0xFFFD;

        if(c < 0x80)
        {
            utf8 += (char)c;
        } else if(c < 0x800)
        {
            utf8 += (char)(0xC0 | (c >> 6));
            utf8 += (char)(0x80 | (c & 0x3F));
        } else if(c < 0x10000)
        {
            utf8 += (char)(0xE0 | (c >> 12));
            utf8 += (char)(0x80 | ((c >> 6) & 0x3F));
            utf8 += (char)(0x80 | (c & 0x3F));
        } else
        {
            utf8 += (char)(0xF0 | (c >> 18));
            utf8 += (char)(0x80 | ((c >> 12) & 0x3F));
            utf8 += (char)(0x80 | ((c >> 6) & 0x3F));
            utf8 += (char)(0x80 | (c & 0x3F));
        }
    }
    return utf8;
}

std::wstring utf8ToWide(const std::string& utf8)
{
    std::wstring wide;
    wide.reserve(utf8.size());
    size_t i = 0;
    while(i < utf8.size())
    {
        unsigned char lead = (unsigned char)utf8[i];
        unsigned int c;
        int extra;

        if(lead < 0x80)                { c = lead;        extra = 0; }
        else if((lead & 0xE0) == 0xC0) { c = lead & 0x1F; extra = 1; }
        else if((lead & 0xF0) == 0xE0) { c = lead & 0x0F; extra = 2; }
        else if((lead & 0xF8) == 0xF0) { c = lead & 0x07; extra = 3; }
        else
        {
            wide += (wchar_t)0xFFFD; // invalid lead byte
            i++;
            continue;
        }

        if(i + extra >= utf8.size())
        {
            wide += (wchar_t)0xFFFD; // truncated sequence
            break;
        }

        bool valid = true;
        for(int k = 1; k <= extra; k++)
        {
            unsigned char cont = (unsigned char)utf8[i + k];
            if((cont & 0xC0) != 0x80)
            {
                valid = false;
                break;
            }
            c = (c << 6) | (cont & 0x3F);
        }

        if(!valid)
        {
            wide += (wchar_t)0xFFFD;
            i++;
            continue;
        }

        wide += (wchar_t)c;
        i += extra + 1;
    }
    return wide;
}

FILE* _wfopen(const wchar_t* filename, const wchar_t* mode)
{
    return fopen(wideToUtf8(filename).c_str(), wideToUtf8(mode).c_str());
}

int _wstat(const wchar_t* path, struct stat* buffer)
{
    return stat(wideToUtf8(path).c_str(), buffer);
}

BOOL SetCurrentDirectoryW(const wchar_t* path)
{
    return chdir(wideToUtf8(path).c_str()) == 0;
}

DWORD GetCurrentDirectoryW(DWORD bufferLength, wchar_t* buffer)
{
    char cwd[PATH_MAX];
    if(getcwd(cwd, sizeof(cwd)) == NULL)
        return 0;

    std::wstring wide = utf8ToWide(cwd);
    if(wide.size() + 1 > bufferLength)
        return wide.size() + 1; // required size, like the real thing

    wcscpy(buffer, wide.c_str());
    return wide.size();
}

BOOL CreateDirectoryW(const wchar_t* path, void* securityAttributes)
{
    (void)securityAttributes;
    return mkdir(wideToUtf8(path).c_str(), 0755) == 0;
}

// Rewrite a Win32 wsprintf format string for the C library's vswprintf,
// where %s/%c take narrow arguments unless prefixed by 'l'.
static std::wstring translateWsprintfFormat(const wchar_t* format)
{
    std::wstring out;
    const wchar_t* p = format;
    while(*p)
    {
        if(*p != L'%')
        {
            out += *p++;
            continue;
        }

        out += *p++; // '%'
        if(*p == L'%')
        {
            out += *p++;
            continue;
        }

        // flags, width, precision
        while(*p && wcschr(L"-+ #0", *p)) out += *p++;
        while(*p && ((*p >= L'0' && *p <= L'9') || *p == L'*')) out += *p++;
        if(*p == L'.')
        {
            out += *p++;
            while(*p && ((*p >= L'0' && *p <= L'9') || *p == L'*')) out += *p++;
        }

        // length modifier
        wchar_t length = 0;
        if(*p == L'h' || *p == L'l')
            length = *p++;

        wchar_t conversion = *p;
        if(!conversion)
            break;
        p++;

        switch(conversion)
        {
            case L's':
            case L'c':
                // wsprintfW: wide unless 'h' asks for narrow
                if(length != L'h')
                    out += L'l';
                out += conversion;
                break;
            case L'S':
            case L'C':
                // wsprintfW: the opposite of the default, i.e. narrow
                if(length == L'l')
                    out += L'l';
                out += (conversion == L'S') ? L's' : L'c';
                break;
            default:
                if(length)
                    out += length;
                out += conversion;
                break;
        }
    }
    return out;
}

int wsprintfW(wchar_t* buffer, const wchar_t* format, ...)
{
    std::wstring translated = translateWsprintfFormat(format);

    va_list args;
    va_start(args, format);
    int written = vswprintf(buffer, 1025, translated.c_str(), args);
    va_end(args);

    if(written < 0)
    {
        // output didn't fit or couldn't be encoded
        buffer[0] = 0;
        written = 0;
    }
    return written;
}

void Sleep(DWORD milliseconds)
{
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (long)(milliseconds % 1000) * 1000000L;
    while(nanosleep(&ts, &ts) == -1 && errno == EINTR)
        ;
}

#endif // _WIN32
