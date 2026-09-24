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

// SDL2 video output. Provides the same interface to the emulator core as the
// DirectDraw renderer used by the Windows frontend.

#ifndef HHUGBOY_SDLRENDERER_H
#define HHUGBOY_SDLRENDERER_H

#include <string>

#include "../../types.h"
#include "../../GB.h"
#include "../../options.h"
#include "../../rendering/palette.h"
#include "../../rendering/filters/filters.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class gb_system;

class SdlRenderer {

    public:

        void (SdlRenderer::*drawBorder)();
        void (SdlRenderer::*drawScreen)();

        SdlRenderer();
        ~SdlRenderer();

        // Creates the window; must be called before init()
        bool createWindow(const wchar_t* title, int width, int height);
        void destroyWindow();

        bool init(Palette* palette);

        void setDrawMode(bool mix);

        void setBorderFilter(videofiltertype type);
        void setGameboyFilter(videofiltertype type);

        void showMessage(std::wstring message, int duration, gb_system* targetGb);

        int getBitCount();

        void handleWindowResize();
        void setRect(bool gb2open);

        void toggleFiltering(bool on);

        // Frontend helpers
        void setTitle(const wchar_t* title);
        void setWindowSize(int width, int height);
        void redraw();

        SDL_Window* getWindow() { return window; }

    private:

        void drawScreen32();
        void drawScreenMix32();
        void drawBorder32();

        void mixFrames();
        void uploadScreen(DWORD* buffer);
        void present();
        void drawMessage(int x, int y, int scale);

        bool changeFilters();
        bool createTextures();

        SDL_Window* window;
        SDL_Renderer* sdlRenderer;
        SDL_Texture* gbTexture[2];
        SDL_Texture* borderTexture;

        DWORD* bufferMix;
        DWORD* filterBuffer;

        Palette* palette;

        Filter* gbFilter;
        Filter* borderFilter;
        Filter* savedGbFilter;
        Filter* savedBorderFilter;
        bool filtersToggledOff;

        int gameboyFilterDimension;
        int borderFilterDimension;

        bool borderVisible;
        bool gb2Visible;

        int oddframe;

        std::wstring messageText;
        int messageDuration;
        gb_system* messageGb;
};

#endif // HHUGBOY_SDLRENDERER_H
