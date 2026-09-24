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

// Functions shared between the parts of the SDL frontend

#ifndef HHUGBOY_SDLFRONTEND_H
#define HHUGBOY_SDLFRONTEND_H

union SDL_Event;

// Audio (SdlAudio.cpp)
bool sdlAudioInit();
void sdlAudioShutdown();
void sdlAudioSetPaused(bool paused);
// Whether the audio output is currently what limits the emulation speed
bool sdlAudioIsPacing();
// When false, audio that would have to wait for the output is dropped instead
void sdlAudioSetThrottle(bool throttle);

// Input (SdlInput.cpp)
void sdlInputInit();
void sdlInputShutdown();
void sdlInputHandleEvent(const SDL_Event& event);

// Messages (SdlMain.cpp)
extern bool sdlDebugLog;
extern bool sdlShowDialogs;

#endif // HHUGBOY_SDLFRONTEND_H
