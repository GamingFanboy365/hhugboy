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

// Tiny built-in 5x7 bitmap font for on-screen messages, so the SDL frontend
// doesn't need a font rendering library. Lowercase letters are drawn as
// uppercase.

#ifndef HHUGBOY_SDLFONT_H
#define HHUGBOY_SDLFONT_H

const int FONT_GLYPH_WIDTH = 5;
const int FONT_GLYPH_HEIGHT = 7;
const int FONT_GLYPH_ADVANCE = 6;

// Returns whether the pixel at (x, y) of the character's glyph is set
bool fontPixel(wchar_t character, int x, int y);

#endif // HHUGBOY_SDLFONT_H
