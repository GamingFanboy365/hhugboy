/*
   hhugboy Game Boy emulator
   Copyright (C) 2026 the hhugboy contributors

   Frame mixing based on the DirectDraw renderer
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

#include <vector>

#include "SdlRenderer.h"
#include "SdlFont.h"

#include "../compat.h"
#include "../../main.h"
#include "../../state.h"
#include "../../SGB.h"
#include "../../debug.h"
#include "../../ui/strings.h"

// Used by the frame mixing code (see DirectDraw::applyPaletteShifts)
int RGB_BIT_MASK = 0x010101;

// The 32-bit pixel format the palette produces: 0x00RRGGBB
static const Uint32 PIXEL_FORMAT = SDL_PIXELFORMAT_RGB888;

// Nearest neighbour "filters" only scale the image up, which the GPU does for
// free when the texture is stretched to the window, so don't do it on the CPU.
static Filter* createFilter(videofiltertype type)
{
    if(type == VIDEO_FILTER_SOFT2X || type == VIDEO_FILTER_SOFTXX)
        type = VIDEO_FILTER_NONE;
    return Filter::getFilter(type);
}

SdlRenderer::SdlRenderer():
    drawBorder(&SdlRenderer::drawBorder32),
    drawScreen(&SdlRenderer::drawScreen32),
    window(NULL),
    sdlRenderer(NULL),
    borderTexture(NULL),
    bufferMix(NULL),
    filterBuffer(NULL),
    palette(NULL),
    gbFilter(new NoFilter()),
    borderFilter(new NoFilter()),
    savedGbFilter(NULL),
    savedBorderFilter(NULL),
    filtersToggledOff(false),
    gameboyFilterDimension(1),
    borderFilterDimension(1),
    borderVisible(false),
    gb2Visible(false),
    oddframe(0),
    messageDuration(0),
    messageGb(NULL)
{
    gbTexture[0] = gbTexture[1] = NULL;
}

SdlRenderer::~SdlRenderer()
{
    destroyWindow();

    delete [] bufferMix;
    delete [] filterBuffer;
}

bool SdlRenderer::createWindow(const wchar_t* title, int width, int height)
{
    window = SDL_CreateWindow(wideToUtf8(title).c_str(),
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              width, height,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if(!window)
    {
        debug_print(SDL_GetError());
        return false;
    }

    sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!sdlRenderer)
        sdlRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    if(!sdlRenderer)
    {
        debug_print(SDL_GetError());
        return false;
    }

    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0"); // nearest neighbour

    return true;
}

void SdlRenderer::destroyWindow()
{
    for(int i = 0; i < 2; i++)
    {
        if(gbTexture[i]) SDL_DestroyTexture(gbTexture[i]);
        gbTexture[i] = NULL;
    }
    if(borderTexture) SDL_DestroyTexture(borderTexture);
    borderTexture = NULL;

    if(sdlRenderer) SDL_DestroyRenderer(sdlRenderer);
    sdlRenderer = NULL;

    if(window) SDL_DestroyWindow(window);
    window = NULL;
}

bool SdlRenderer::init(Palette* palette)
{
    this->palette = palette;

    // 0x00RRGGBB: the shifts point at the top of each 8-bit channel minus the 5 bits of GBC colour
    palette->setPaletteShifts(16 + 3, 8 + 3, 0 + 3);
    RGB_BIT_MASK = 0x010101;

    if(!palette->initPalettes(32))
        return false;

    bufferMix = new DWORD[160*144];

    setDrawMode(false);
    drawBorder = &SdlRenderer::drawBorder32;

    return changeFilters();
}

void SdlRenderer::setDrawMode(bool mix)
{
    drawScreen = mix ? &SdlRenderer::drawScreenMix32 : &SdlRenderer::drawScreen32;
}

void SdlRenderer::setBorderFilter(videofiltertype type)
{
    borderFilter = createFilter(type);
    changeFilters();
}

void SdlRenderer::setGameboyFilter(videofiltertype type)
{
    gbFilter = createFilter(type);
    changeFilters();
}

void SdlRenderer::toggleFiltering(bool on)
{
    if(!on)
    {
        if(filtersToggledOff)
            return;
        savedGbFilter = gbFilter;
        savedBorderFilter = borderFilter;
        gbFilter = borderFilter = new NoFilter();
        filtersToggledOff = true;
        changeFilters();
    } else if(filtersToggledOff)
    {
        delete gbFilter;
        filtersToggledOff = false;
        gbFilter = savedGbFilter;
        borderFilter = savedBorderFilter;
        changeFilters();
    }
}

bool SdlRenderer::createTextures()
{
    if(!sdlRenderer)
        return false;

    for(int i = 0; i < 2; i++)
    {
        if(gbTexture[i]) SDL_DestroyTexture(gbTexture[i]);
        gbTexture[i] = SDL_CreateTexture(sdlRenderer, PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING,
                                         160 * gameboyFilterDimension, 144 * gameboyFilterDimension);
        if(!gbTexture[i])
        {
            debug_print(SDL_GetError());
            return false;
        }
    }

    if(borderTexture) SDL_DestroyTexture(borderTexture);
    borderTexture = SDL_CreateTexture(sdlRenderer, PIXEL_FORMAT, SDL_TEXTUREACCESS_STREAMING,
                                      256 * borderFilterDimension, 224 * borderFilterDimension);
    if(!borderTexture)
    {
        debug_print(SDL_GetError());
        return false;
    }

    // Start with black screens
    delete [] filterBuffer;
    int maxDimension = gameboyFilterDimension > borderFilterDimension ? gameboyFilterDimension : borderFilterDimension;
    filterBuffer = new DWORD[256 * 224 * maxDimension * maxDimension]();

    for(int i = 0; i < 2; i++)
        SDL_UpdateTexture(gbTexture[i], NULL, filterBuffer, 160 * gameboyFilterDimension * sizeof(DWORD));
    SDL_UpdateTexture(borderTexture, NULL, filterBuffer, 256 * borderFilterDimension * sizeof(DWORD));

    return true;
}

bool SdlRenderer::changeFilters()
{
    gameboyFilterDimension = gbFilter->getFilterDimension();
    borderFilterDimension = borderFilter->getFilterDimension();

    if(!createTextures())
        return false;

    if(GB1 && GB1->romloaded && sgb_mode)
        (this->*drawBorder)();

    return true;
}

int SdlRenderer::getBitCount()
{
    return 32;
}

void SdlRenderer::showMessage(std::wstring message, int duration, gb_system* targetGb)
{
    messageText = message;
    messageDuration = duration;
    messageGb = targetGb;
}

void SdlRenderer::handleWindowResize()
{
    if(sgb_mode || (options->GBC_SGB_border != OFF && border_uploaded))
        (this->*drawBorder)();
    else
        redraw();
}

void SdlRenderer::setRect(bool gb2open)
{
    // The layout is worked out from the window size every time we present
}

void SdlRenderer::setTitle(const wchar_t* title)
{
    if(window)
        SDL_SetWindowTitle(window, wideToUtf8(title).c_str());
}

void SdlRenderer::setWindowSize(int width, int height)
{
    if(!window)
        return;

    if(multiple_gb)
        width *= 2;

    SDL_SetWindowSize(window, width * options->video_size, height * options->video_size);
}

void SdlRenderer::redraw()
{
    present();
}

void SdlRenderer::drawScreen32()
{
    uploadScreen((DWORD*)GB->gfx_buffer);
}

void SdlRenderer::drawScreenMix32()
{
    mixFrames();
    uploadScreen(bufferMix);
}

// Blend the current frame with the previous ones to emulate LCD ghosting
// (32-bit version of DirectDraw::drawScreenMixGeneric)
void SdlRenderer::mixFrames()
{
    DWORD* current = (DWORD*)GB->gfx_buffer;
    DWORD* old = (DWORD*)GB->gfx_buffer_old;
    DWORD* older = (DWORD*)GB->gfx_buffer_older;
    DWORD* oldest = (DWORD*)GB->gfx_buffer_oldest;

    DWORD* target = bufferMix;

    if(options->video_mix_frames == MIX_FRAMES_MORE && !(GB->gbc_mode || sgb_mode)) { // More mixing for classic GB: 3/8 current, 3/8 old, 1/8 older, 1/8 oldest
        for (int y = 0; y <144*160; y++) {
            *target++ =((*current &0x0000FF)*3 +(*old &0x0000FF)*3 +(*older &0x0000FF) +(*oldest &0x0000FF)) >>3 &0x0000FF |
                       ((*current &0x00FF00)*3 +(*old &0x00FF00)*3 +(*older &0x00FF00) +(*oldest &0x00FF00)) >>3 &0x00FF00 |
                       ((*current &0xFF0000)*3 +(*old &0xFF0000)*3 +(*older &0xFF0000) +(*oldest &0xFF0000)) >>3 &0xFF0000;
            ++current;
            ++old;
            ++older;
            ++oldest;
        }
        void* temp1 = GB->gfx_buffer;
        void* temp2 = GB->gfx_buffer_older;
        GB->gfx_buffer = GB->gfx_buffer_oldest;
        GB->gfx_buffer_older = GB->gfx_buffer_old;
        GB->gfx_buffer_old = temp1;
        GB->gfx_buffer_oldest = temp2;
    } else { // Average previous and current picture
        for (int y = 0; y <144*160; y++) {
            *target++ =((*current &0x0000FF) + (*old &0x0000FF)) >>1 &0x0000FF |
                       ((*current &0x00FF00) + (*old &0x00FF00)) >>1 &0x00FF00 |
                       ((*current &0xFF0000) + (*old &0xFF0000)) >>1 &0xFF0000;
            ++current;
            ++old;
        }
        void* temp = GB->gfx_buffer;
        GB->gfx_buffer = GB->gfx_buffer_old;
        GB->gfx_buffer_old = temp;
    }
}

void SdlRenderer::uploadScreen(DWORD* buffer)
{
    int screen = (multiple_gb && GB == GB2) ? 1 : 0;
    if(!gbTexture[screen])
        return;

    int pitch = 160 * gameboyFilterDimension;
    gbFilter->filter32(filterBuffer, buffer, 160, 144, pitch);
    SDL_UpdateTexture(gbTexture[screen], NULL, filterBuffer, pitch * sizeof(DWORD));

    if(GB == messageGb && messageDuration)
        --messageDuration;

    oddframe ^= 1;

    present();
}

void SdlRenderer::drawBorder32()
{
    if(!borderTexture || !palette)
        return;

    std::vector<DWORD> border(256*224);
    for(int i = 0; i < 256*224; i++)
        border[i] = palette->gfxPal32[sgb_border_buffer[i]];

    int pitch = 256 * borderFilterDimension;
    borderFilter->filter32(filterBuffer, &border[0], 256, 224, pitch);
    SDL_UpdateTexture(borderTexture, NULL, filterBuffer, pitch * sizeof(DWORD));

    present();
}

void SdlRenderer::drawMessage(int x, int y, int scale)
{
    std::vector<SDL_Rect> outline;
    std::vector<SDL_Rect> text;

    for(size_t c = 0; c < messageText.size(); c++)
    {
        int charX = x + (int)c * FONT_GLYPH_ADVANCE * scale;
        for(int py = 0; py < FONT_GLYPH_HEIGHT; py++)
        {
            for(int px = 0; px < FONT_GLYPH_WIDTH; px++)
            {
                if(!fontPixel(messageText[c], px, py))
                    continue;

                SDL_Rect pixel = { charX + px * scale, y + py * scale, scale, scale };
                text.push_back(pixel);

                // one pixel outline around the text, like the Windows version
                for(int oy = -1; oy <= 1; oy++)
                    for(int ox = -1; ox <= 1; ox++)
                        if(ox || oy)
                        {
                            SDL_Rect edge = { pixel.x + ox * scale, pixel.y + oy * scale, scale, scale };
                            outline.push_back(edge);
                        }
            }
        }
    }

    if(outline.empty())
        return;

    SDL_SetRenderDrawColor(sdlRenderer, 255, 0, 128, 255);
    SDL_RenderFillRects(sdlRenderer, &outline[0], outline.size());
    SDL_SetRenderDrawColor(sdlRenderer, 255, 255, 255, 255);
    SDL_RenderFillRects(sdlRenderer, &text[0], text.size());
}

void SdlRenderer::present()
{
    if(!sdlRenderer || !gbTexture[0])
        return;

    int width, height;
    SDL_GetRendererOutputSize(sdlRenderer, &width, &height);

    SDL_SetRenderDrawColor(sdlRenderer, 0, 0, 0, 255);
    SDL_RenderClear(sdlRenderer);

    bool border = sgb_mode || (options->GBC_SGB_border != OFF && border_uploaded);
    int screens = multiple_gb ? 2 : 1;
    int areaWidth = width / screens;

    int vibeStrength = 0;
    if(options->video_visual_rumble && GB && GB->vibeCycles) {
        vibeStrength = GB->vibeCycles / 10000;
        if (vibeStrength > 8) vibeStrength = 8;
        if (oddframe) vibeStrength *= -1;
    }

    for(int screen = 0; screen < screens; screen++)
    {
        SDL_Rect area = { screen * areaWidth, 0, areaWidth, height };
        SDL_Rect gbRect = area;

        if(border)
        {
            SDL_RenderCopy(sdlRenderer, borderTexture, NULL, &area);
            gbRect.x = area.x + (int)(48.0 * area.w / 256.0 + 0.5);
            gbRect.y = area.y + (int)(40.0 * area.h / 224.0 + 0.5);
            gbRect.w = (int)(160.0 * area.w / 256.0 + 0.5);
            gbRect.h = (int)(144.0 * area.h / 224.0 + 0.5);
        }

        gbRect.x -= vibeStrength * gbRect.w / 160;

        SDL_RenderCopy(sdlRenderer, gbTexture[screen], NULL, &gbRect);

        gb_system* screenGb = screen ? GB2 : GB1;
        if(messageDuration && messageGb == screenGb)
        {
            int scale = gbRect.w / 160;
            if(scale < 1) scale = 1;
            drawMessage(gbRect.x + 2 * scale, gbRect.y + 2 * scale, scale);
        }
    }

    SDL_RenderPresent(sdlRenderer);
}
