/*
   hhugboy Game Boy emulator
   Copyright (C) 2026 the hhugboy contributors

   Joystick and system key handling based on the DirectInput code
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

#include <SDL.h>
#include <string.h>

#include "SdlFrontend.h"

#include "../dikcodes.h"
#include "../../input.h"
#include "../../main.h"
#include "../../mainloop.h"
#include "../../state.h"
#include "../../config.h"
#include "../../ui/window.h"

extern int speedup;

static int old_sound_on = 0;

static SDL_Joystick* joystick = NULL;

static const int JOYSTICK_DEADZONE = 8000;

// Key bindings are stored as DirectInput key codes, which are PC scan codes,
// so translate SDL's (also position based) scancodes to them
static const struct { SDL_Scancode scancode; int dik; } keyMap[] = {
    { SDL_SCANCODE_ESCAPE, DIK_ESCAPE },
    { SDL_SCANCODE_1, DIK_1 }, { SDL_SCANCODE_2, DIK_2 }, { SDL_SCANCODE_3, DIK_3 },
    { SDL_SCANCODE_4, DIK_4 }, { SDL_SCANCODE_5, DIK_5 }, { SDL_SCANCODE_6, DIK_6 },
    { SDL_SCANCODE_7, DIK_7 }, { SDL_SCANCODE_8, DIK_8 }, { SDL_SCANCODE_9, DIK_9 },
    { SDL_SCANCODE_0, DIK_0 },
    { SDL_SCANCODE_MINUS, DIK_MINUS }, { SDL_SCANCODE_EQUALS, DIK_EQUALS },
    { SDL_SCANCODE_BACKSPACE, DIK_BACK }, { SDL_SCANCODE_TAB, DIK_TAB },
    { SDL_SCANCODE_Q, DIK_Q }, { SDL_SCANCODE_W, DIK_W }, { SDL_SCANCODE_E, DIK_E },
    { SDL_SCANCODE_R, DIK_R }, { SDL_SCANCODE_T, DIK_T }, { SDL_SCANCODE_Y, DIK_Y },
    { SDL_SCANCODE_U, DIK_U }, { SDL_SCANCODE_I, DIK_I }, { SDL_SCANCODE_O, DIK_O },
    { SDL_SCANCODE_P, DIK_P },
    { SDL_SCANCODE_LEFTBRACKET, DIK_LBRACKET }, { SDL_SCANCODE_RIGHTBRACKET, DIK_RBRACKET },
    { SDL_SCANCODE_RETURN, DIK_RETURN }, { SDL_SCANCODE_LCTRL, DIK_LCONTROL },
    { SDL_SCANCODE_A, DIK_A }, { SDL_SCANCODE_S, DIK_S }, { SDL_SCANCODE_D, DIK_D },
    { SDL_SCANCODE_F, DIK_F }, { SDL_SCANCODE_G, DIK_G }, { SDL_SCANCODE_H, DIK_H },
    { SDL_SCANCODE_J, DIK_J }, { SDL_SCANCODE_K, DIK_K }, { SDL_SCANCODE_L, DIK_L },
    { SDL_SCANCODE_SEMICOLON, DIK_SEMICOLON }, { SDL_SCANCODE_APOSTROPHE, DIK_APOSTROPHE },
    { SDL_SCANCODE_GRAVE, DIK_GRAVE }, { SDL_SCANCODE_LSHIFT, DIK_LSHIFT },
    { SDL_SCANCODE_BACKSLASH, DIK_BACKSLASH },
    { SDL_SCANCODE_Z, DIK_Z }, { SDL_SCANCODE_X, DIK_X }, { SDL_SCANCODE_C, DIK_C },
    { SDL_SCANCODE_V, DIK_V }, { SDL_SCANCODE_B, DIK_B }, { SDL_SCANCODE_N, DIK_N },
    { SDL_SCANCODE_M, DIK_M },
    { SDL_SCANCODE_COMMA, DIK_COMMA }, { SDL_SCANCODE_PERIOD, DIK_PERIOD },
    { SDL_SCANCODE_SLASH, DIK_SLASH }, { SDL_SCANCODE_RSHIFT, DIK_RSHIFT },
    { SDL_SCANCODE_KP_MULTIPLY, DIK_MULTIPLY }, { SDL_SCANCODE_LALT, DIK_LMENU },
    { SDL_SCANCODE_SPACE, DIK_SPACE }, { SDL_SCANCODE_CAPSLOCK, DIK_CAPITAL },
    { SDL_SCANCODE_F1, DIK_F1 }, { SDL_SCANCODE_F2, DIK_F2 }, { SDL_SCANCODE_F3, DIK_F3 },
    { SDL_SCANCODE_F4, DIK_F4 }, { SDL_SCANCODE_F5, DIK_F5 }, { SDL_SCANCODE_F6, DIK_F6 },
    { SDL_SCANCODE_F7, DIK_F7 }, { SDL_SCANCODE_F8, DIK_F8 }, { SDL_SCANCODE_F9, DIK_F9 },
    { SDL_SCANCODE_F10, DIK_F10 }, { SDL_SCANCODE_F11, DIK_F11 }, { SDL_SCANCODE_F12, DIK_F12 },
    { SDL_SCANCODE_NUMLOCKCLEAR, DIK_NUMLOCK }, { SDL_SCANCODE_SCROLLLOCK, DIK_SCROLL },
    { SDL_SCANCODE_KP_7, DIK_NUMPAD7 }, { SDL_SCANCODE_KP_8, DIK_NUMPAD8 }, { SDL_SCANCODE_KP_9, DIK_NUMPAD9 },
    { SDL_SCANCODE_KP_MINUS, DIK_SUBTRACT },
    { SDL_SCANCODE_KP_4, DIK_NUMPAD4 }, { SDL_SCANCODE_KP_5, DIK_NUMPAD5 }, { SDL_SCANCODE_KP_6, DIK_NUMPAD6 },
    { SDL_SCANCODE_KP_PLUS, DIK_ADD },
    { SDL_SCANCODE_KP_1, DIK_NUMPAD1 }, { SDL_SCANCODE_KP_2, DIK_NUMPAD2 }, { SDL_SCANCODE_KP_3, DIK_NUMPAD3 },
    { SDL_SCANCODE_KP_0, DIK_NUMPAD0 }, { SDL_SCANCODE_KP_PERIOD, DIK_DECIMAL },
    { SDL_SCANCODE_NONUSBACKSLASH, DIK_OEM_102 },
    { SDL_SCANCODE_KP_ENTER, DIK_NUMPADENTER }, { SDL_SCANCODE_RCTRL, DIK_RCONTROL },
    { SDL_SCANCODE_KP_DIVIDE, DIK_DIVIDE }, { SDL_SCANCODE_PRINTSCREEN, DIK_SYSRQ },
    { SDL_SCANCODE_RALT, DIK_RMENU }, { SDL_SCANCODE_PAUSE, DIK_PAUSE },
    { SDL_SCANCODE_HOME, DIK_HOME }, { SDL_SCANCODE_UP, DIK_UP }, { SDL_SCANCODE_PAGEUP, DIK_PRIOR },
    { SDL_SCANCODE_LEFT, DIK_LEFT }, { SDL_SCANCODE_RIGHT, DIK_RIGHT },
    { SDL_SCANCODE_END, DIK_END }, { SDL_SCANCODE_DOWN, DIK_DOWN }, { SDL_SCANCODE_PAGEDOWN, DIK_NEXT },
    { SDL_SCANCODE_INSERT, DIK_INSERT }, { SDL_SCANCODE_DELETE, DIK_DELETE },
    { SDL_SCANCODE_LGUI, DIK_LWIN }, { SDL_SCANCODE_RGUI, DIK_RWIN }, { SDL_SCANCODE_APPLICATION, DIK_APPS },
};

// Fills in a DirectInput style keyboard state: 0x80 for each key held down
static void readKeyboard(char* buffer)
{
    memset(buffer, 0, 256);

    int numKeys = 0;
    const Uint8* state = SDL_GetKeyboardState(&numKeys);

    for(size_t k = 0; k < sizeof(keyMap) / sizeof(keyMap[0]); k++)
    {
        if(keyMap[k].scancode < numKeys && state[keyMap[k].scancode])
            buffer[keyMap[k].dik] = (char)0x80;
    }
}

static void openJoystick()
{
    if(joystick || options->use_joystick_input < 0)
        return;

    for(int i = 0; i < SDL_NumJoysticks(); i++)
    {
        joystick = SDL_JoystickOpen(i);
        if(joystick)
            return;
    }
}

void sdlInputInit()
{
    if(SDL_InitSubSystem(SDL_INIT_JOYSTICK) == 0)
        openJoystick();
}

void sdlInputShutdown()
{
    if(joystick)
        SDL_JoystickClose(joystick);
    joystick = NULL;
}

void sdlInputHandleEvent(const SDL_Event& event)
{
    switch(event.type)
    {
        case SDL_JOYDEVICEADDED:
            openJoystick();
            break;
        case SDL_JOYDEVICEREMOVED:
            if(joystick && event.jdevice.which == SDL_JoystickInstanceID(joystick))
            {
                SDL_JoystickClose(joystick);
                joystick = NULL;
                openJoystick();
            }
            break;
    }
}

static bool joystickButton(int button)
{
    return button >= 0 && button < SDL_JoystickNumButtons(joystick) && SDL_JoystickGetButton(joystick, button);
}

static void check_joystick_input()
{
    int x = SDL_JoystickNumAxes(joystick) > 0 ? SDL_JoystickGetAxis(joystick, 0) : 0;
    int y = SDL_JoystickNumAxes(joystick) > 1 ? SDL_JoystickGetAxis(joystick, 1) : 0;

    if(SDL_JoystickNumHats(joystick) > 0)
    {
        Uint8 hat = SDL_JoystickGetHat(joystick, 0);
        if(hat & SDL_HAT_LEFT)  x = -32768;
        if(hat & SDL_HAT_RIGHT) x = 32767;
        if(hat & SDL_HAT_UP)    y = -32768;
        if(hat & SDL_HAT_DOWN)  y = 32767;
    }

    if(x < -JOYSTICK_DEADZONE)
    {
        GB->button_pressed[B_RIGHT] = 1;
        GB->button_pressed[B_LEFT] = 0;
    } else if(x > JOYSTICK_DEADZONE)
    {
        GB->button_pressed[B_RIGHT] = 0;
        GB->button_pressed[B_LEFT] = 1;
    }

    if(y < -JOYSTICK_DEADZONE)
    {
        GB->button_pressed[B_UP] = 0;
        GB->button_pressed[B_DOWN] = 1;
    } else if(y > JOYSTICK_DEADZONE)
    {
        GB->button_pressed[B_DOWN] = 0;
        GB->button_pressed[B_UP] = 1;
    }

    if(joystickButton(options->joystick_config[0]))
        GB->button_pressed[B_A] = 0;

    if(joystickButton(options->joystick_config[1]))
        GB->button_pressed[B_B] = 0;

    if(joystickButton(options->joystick_config[2]))
        GB->button_pressed[B_START] = 0;

    if(joystickButton(options->joystick_config[3]))
        GB->button_pressed[B_SELECT] = 0;

    if(joystickButton(options->joystick_config[4]))
    {
        if(--autofire_delay[options->use_joystick_input][0] <= 0)
        {
            autofire_delay[options->use_joystick_input][0] = options->autofire_speed;
            GB->button_pressed[B_A] = !GB->button_pressed[B_A];
        }
    } else
        autofire_delay[options->use_joystick_input][0] = 0;

    if(joystickButton(options->joystick_config[5]))
    {
        if(--autofire_delay[options->use_joystick_input][1] <= 0)
        {
            autofire_delay[options->use_joystick_input][1] = options->autofire_speed;
            GB->button_pressed[B_B] = !GB->button_pressed[B_B];
        }
    } else
        autofire_delay[options->use_joystick_input][1] = 0;
}

static void systemKeyActions(char* buffer)
{
    if(KEYDOWN(buffer,options->special_keys[BUTTON_SPEEDUP]))
    {
        if(speedup == 0 && options->speedup_sound_off)
        {
            old_sound_on = options->sound_on;
            options->sound_on = 0;
            sdlAudioSetPaused(true);
        }
        if(speedup == 0 && options->speedup_filter_off)
            renderer.toggleFiltering(false);
        speedup = 1;
    }
    else if(speedup)
    {
        if(options->speedup_sound_off)
        {
            options->sound_on = old_sound_on;
            if(options->sound_on > 0)
                sdlAudioSetPaused(false);
        }
        if(options->speedup_filter_off)
            renderer.toggleFiltering(true);
        speedup = 0;
    }

    if(KEYDOWN(buffer,options->special_keys[BUTTON_L]) && !multiple_gb)
    {
        if(GB1->system_type == SYS_GBA)
            setWinSize(240,144);
    } else
    if(KEYDOWN(buffer,options->special_keys[BUTTON_R]) && !multiple_gb)
    {
        if(GB1->system_type == SYS_GBA)
            setWinSize(160,144);
    }
}

void Check_KBInput(int i)
{
    char buffer[256];

    if(GB == GB2)
        i = 1;

    readKeyboard(buffer);

    if(!apply_keyboard_state(buffer, i))
        return;

    if(joystick != NULL && i == options->use_joystick_input)
        check_joystick_input();

    systemKeyActions(buffer);
}

void check_system_keys()
{
    char buffer[256];

    readKeyboard(buffer);

    systemKeyActions(buffer);
}
