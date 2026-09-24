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

// WORD/DWORD are used throughout the renderer as 16/32-bit pixel types.
// On Windows they have to match windows.h exactly (DWORD is unsigned long,
// which is 32 bits there), but on LP64 platforms like x86_64 Linux an
// unsigned long is 64 bits, so use the fixed-width types instead.

#ifndef HHUGBOY_WINTYPES_H
#define HHUGBOY_WINTYPES_H

#ifdef _WIN32
typedef unsigned long DWORD;
typedef unsigned short WORD;
#else
#include <stdint.h>
typedef uint32_t DWORD;
typedef uint16_t WORD;
#endif

#endif // HHUGBOY_WINTYPES_H
