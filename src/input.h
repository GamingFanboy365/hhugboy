/*
   hhugboy Game Boy emulator
   copyright 2013 taizou

   Based on GEST
   Copyright (C) 2003-2010 TM

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

// Platform independent part of the input handling.
// Each frontend implements Check_KBInput and check_system_keys by reading the
// keyboard into an array indexed by DirectInput key code (the format key
// bindings are stored in the config file) and passing it to
// apply_keyboard_state.

#ifndef INPUT_H
#define INPUT_H

// implemented by the frontend
void Check_KBInput(int);
void check_system_keys();

extern int soft_reset;

extern int autofire_delay[4][4];

const int AUTOFIRE_DELAY_FASTEST = 0;
const int AUTOFIRE_DELAY_FAST = 4;
const int AUTOFIRE_DELAY_MEDIUM = 10;
const int AUTOFIRE_DELAY_SLOW = 20;

#define KEYDOWN(name,key) (name[key] & 0x80)

// Updates the current GB's buttons and the MBC7 sensor from the keyboard state
// for the given player. Returns false if the input was consumed by a soft reset.
bool apply_keyboard_state(const char* buffer, int i);

#endif
