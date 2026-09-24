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

// SDL audio output. The sound core hands over one frame's worth of samples at
// a time (735 stereo samples at 44100Hz); while sound is on, waiting for the
// output queue to drain is what keeps the emulator running at the right speed.

#include <SDL.h>
#include <stdio.h>

#include "SdlFrontend.h"

#include "../../sound.h"
#include "../../config.h"

extern int speedup;

static SDL_AudioDeviceID audioDevice = 0;
static bool audioThrottle = true;

// Keep around three frames of audio queued: enough to avoid dropouts,
// short enough to keep the latency low
static const Uint32 MAX_QUEUED_BYTES = 735 * 4 * 3;

bool sdlAudioInit()
{
    if(SDL_InitSubSystem(SDL_INIT_AUDIO) != 0)
    {
        fprintf(stderr, "Couldn't initialise audio: %s\n", SDL_GetError());
        return false;
    }

    SDL_AudioSpec wanted;
    SDL_zero(wanted);
    wanted.freq = 44100;
    wanted.format = AUDIO_S16SYS;
    wanted.channels = 2;
    wanted.samples = 512;
    wanted.callback = NULL; // we queue audio instead

    audioDevice = SDL_OpenAudioDevice(NULL, 0, &wanted, NULL, 0);
    if(!audioDevice)
    {
        fprintf(stderr, "Couldn't open audio device: %s\n", SDL_GetError());
        return false;
    }

    return true;
}

void sdlAudioShutdown()
{
    if(audioDevice)
        SDL_CloseAudioDevice(audioDevice);
    audioDevice = 0;
}

void sdlAudioSetPaused(bool paused)
{
    if(!audioDevice)
        return;

    if(paused)
        SDL_ClearQueuedAudio(audioDevice);
    SDL_PauseAudioDevice(audioDevice, paused ? 1 : 0);
}

bool sdlAudioIsPacing()
{
    return audioDevice && audioThrottle && !speedup && options->sound_on > 0
        && SDL_GetAudioDeviceStatus(audioDevice) == SDL_AUDIO_PLAYING;
}

void sdlAudioSetThrottle(bool throttle)
{
    audioThrottle = throttle;
}

void sound_output_write(signed short* samples, int bytes)
{
    if(!audioDevice || SDL_GetAudioDeviceStatus(audioDevice) != SDL_AUDIO_PLAYING)
        return;

    if(!speedup && audioThrottle)
    {
        while(SDL_GetQueuedAudioSize(audioDevice) > MAX_QUEUED_BYTES)
            SDL_Delay(1);
    } else if(SDL_GetQueuedAudioSize(audioDevice) > MAX_QUEUED_BYTES)
    {
        return; // running faster than real time, drop it
    }

    SDL_QueueAudio(audioDevice, samples, bytes);
}

void sound_output_reset()
{
    if(audioDevice)
        SDL_ClearQueuedAudio(audioDevice);
}
