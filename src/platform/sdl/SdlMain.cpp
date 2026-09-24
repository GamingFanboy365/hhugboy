/*
   hhugboy Game Boy emulator
   Copyright (C) 2026 the hhugboy contributors

   Main loop and key actions based on the Windows frontend
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

// Entry point of the SDL2 frontend (used on Linux and other non-Windows systems)

#include <SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <unistd.h>

#include <fstream>
#include <string>

#include "SdlFrontend.h"
#include "SdlRenderer.h"

#include "../compat.h"
#include "../../main.h"
#include "../../GB.h"
#include "../../GB_gfx.h"
#include "../../SGB.h"
#include "../../state.h"
#include "../../input.h"
#include "../../config.h"
#include "../../debug.h"
#include "../../rendering/render.h"
#include "../../ui/strings.h"
#include "../../ui/window.h"
#include "../../ui/dialogs/LinkerLog.h"
#include "../../memory/linker/LinkerWrangler.h"

using namespace std;

SdlRenderer renderer;

wchar_t w_title_text[ROM_FILENAME_SIZE + 16];

bool sdlDebugLog = false;
bool sdlShowDialogs = true;

// Command line options
static long framesToRun = -1;       // exit after this many frames
static string screenshotOnExit;     // save a screenshot here before exiting
static bool unthrottled = false;    // run as fast as possible

// Messages --------------------------------------------------------------------

void debug_print(const char* message)
{
    fprintf(stderr, "%s\n", message);
    if(sdlShowDialogs)
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "hhugboy", message, renderer.getWindow());
}

void debug_print(const wchar_t* message)
{
    debug_print(wideToUtf8(message).c_str());
}

void debug_win(const char* message)
{
    if(sdlDebugLog)
        fprintf(stderr, "[debug] %s\n", message);
}

void debug_win(const wchar_t* message)
{
    if(sdlDebugLog)
        debug_win(wideToUtf8(message).c_str());
}

// There's no linker log window, so the linker's messages go to the terminal and
// "spawning" it toggles the connection (like the dialog's start/stop buttons).
static bool linkerStarted = false;

void LinkerLog::SpawnLinkerLog()
{
    if(linkerStarted)
    {
        LinkerWrangler::deinitLinker();
        linkerStarted = false;
        addMessage("Linker stopped");
    } else
    {
        linkerStarted = LinkerWrangler::initLinker();
        if(linkerStarted)
            addMessage("Linker started");
    }
}

void LinkerLog::addMessage(const char* message)
{
    fprintf(stderr, "[linker] %s\n", message);
}

void LinkerLog::addMessage(const wchar_t* message)
{
    addMessage(wideToUtf8(message).c_str());
}

// Window ----------------------------------------------------------------------

void setWinSize(int width,int height)
{
    renderer.setWindowSize(width, height);
}

static void updateTitle()
{
    if(multiple_gb)
        wsprintfW(w_title_text,L"%s - %s --- %s",w_emu_title,GB1->rom_filename,GB2->rom_filename);
    else if(GB1->romloaded)
        wsprintfW(w_title_text,L"%s - %s",w_emu_title,GB1->rom_filename);
    else
        wsprintfW(w_title_text,L"%s",w_emu_title);
    renderer.setTitle(w_title_text);
}

static void showMessage(const wchar_t* message, int duration, gb_system* gb)
{
    renderer.showMessage(message, duration, gb);
    if(!GB1->romloaded || paused)
        renderer.redraw();
}

// Setup -----------------------------------------------------------------------

// hhugboy keeps its config, saves and states next to the executable, like on
// Windows. If that's not writable (e.g. installed system wide), it uses the
// user's data directory instead (~/.local/share/hhugboy).
static wstring findProgramDirectory()
{
    string directory;

    char* basePath = SDL_GetBasePath();
    if(basePath)
    {
        directory = basePath;
        SDL_free(basePath);
    }

    if(directory.empty() || access(directory.c_str(), W_OK) != 0)
    {
        char* prefPath = SDL_GetPrefPath(NULL, "hhugboy");
        if(prefPath)
        {
            directory = prefPath;
            SDL_free(prefPath);
        }
    }

    if(directory.empty())
        directory = ".";

    while(directory.size() > 1 && directory[directory.size()-1] == '/')
        directory.erase(directory.size()-1);

    return utf8ToWide(directory);
}

static void initConfigs()
{
    options->program_directory = findProgramDirectory();

    if(!read_config_file())
        debug_print(str_table[ERROR_CFG_FILE_READ]);
}

static void loadBootstrap()
{
    FILE* F;
    SetCurrentDirectory(options->program_directory.c_str());

    haveBootstrap_DMG = false;
    F = fopen("dmg_boot.bin", "rb");
    if (F) {
        haveBootstrap_DMG = fread(bootstrapDMG, 1, 256, F) == 256;
        fclose(F);
    }

    haveBootstrap_CGB = false;
    F = fopen("cgb_boot.bin", "rb");
    if (F) {
        haveBootstrap_CGB = fread(bootstrapCGB, 1, 2304, F) == 2304;
        fclose(F);
    }
}

static bool initGraphics()
{
    if(!renderer.createWindow(w_emu_title, 160 * options->video_size, 144 * options->video_size))
        return false;

    if(!renderer.init(&palette))
    {
        debug_print(str_table[ERROR_DDRAW]);
        return false;
    }

    gb_system::gfx_bit_count = renderer.getBitCount();
    GB->init_gfx();
    return true;
}

static void initSound()
{
    if(!sdlAudioInit())
    {
        options->sound_on = -1;
        return;
    }

    sdlAudioSetPaused(options->sound_on <= 0);
}

// The emulator changes the current directory when saving, so resolve paths
// given on the command line straight away
static string absolutePath(const string& path)
{
    if(path.empty() || path[0] == '/')
        return path;

    char cwd[4096];
    if(!getcwd(cwd, sizeof(cwd)))
        return path;

    return string(cwd) + "/" + path;
}

static bool hasRomExtension(const wstring& filename)
{
    const wchar_t* extensions[] = { L".zip", L".ZIP", L".sgb", L".SGB", L".GB", L".gb", L".GBX", L".gbx" };
    for(size_t i = 0; i < sizeof(extensions) / sizeof(extensions[0]); i++)
        if(filename.find(extensions[i]) != wstring::npos)
            return true;
    return false;
}

static void loadRom(const wstring& filename)
{
    bool romwasloaded = GB1->romloaded;

    if(GB1->romloaded && !GB1->write_save())
        debug_print(str_table[ERROR_SAVE_FILE_WRITE]);

    gb1_loaded_file_name = filename;
    GB1->load_rom(filename.c_str());

    if(GB1->romloaded)
    {
        GB1->reset();

        if(sgb_mode)
            setWinSize(256,224);
        else
            setWinSize(160,144);

        if(!GB1->load_save())
            debug_print(str_table[ERROR_SAVE_FILE_READ]);

        updateTitle();

        if(!paused && options->sound_on > 0)
            sdlAudioSetPaused(false);
    } else if(romwasloaded && GB1->cartROM != NULL)
        GB1->romloaded = true;
}

static void cleanup()
{
    if(GB1 != NULL)
    {
        delete GB1;
        GB1 = NULL;
    }

    if(GB2 != NULL)
    {
        delete GB2;
        GB2 = NULL;
    }

    GB = NULL;

    sgb_end();

    sdlInputShutdown();
    sdlAudioShutdown();
    renderer.destroyWindow();

    if(options != NULL)
    {
        delete options;
        options = NULL;
    }
}

// Key actions (same keys as the Windows frontend) -----------------------------

static void takeScreenshot(gb_system* screenshotGb, const string& requestedFilename)
{
    string final_filename = requestedFilename;

    if(final_filename.empty())
    {
        // first available "<rom name>_nnnn.png" in the current directory
        string base = wideToUtf8(screenshotGb->rom_filename);
        for(int fileno = 1; ; fileno++)
        {
            char tmp_filename[32];
            snprintf(tmp_filename, sizeof(tmp_filename), "_%04d.png", fileno);
            string candidate = base + tmp_filename;
            ifstream ifile(candidate.c_str());
            if(!ifile)
            {
                final_filename = candidate;
                break;
            }
        }
    }

    screenshotPng((char*)final_filename.c_str(), screenshotGb);
}

static void toggleVideoLayer(int layer, const wchar_t* onMessage, const wchar_t* offMessage)
{
    video_enable ^= layer;
    showMessage((video_enable & layer) ? onMessage : offMessage, 40, GB1);
}

static void setPaused(bool pause)
{
    paused = pause;
    if(pause)
        sdlAudioSetPaused(true);
    else if(GB1->romloaded && options->sound_on > 0)
        sdlAudioSetPaused(false);
}

// Returns false to quit
static bool keyAction(const SDL_KeyboardEvent& key)
{
    bool control = (key.keysym.mod & KMOD_CTRL) != 0;
    control_pressed = control;

    switch(key.keysym.sym)
    {
        case SDLK_ESCAPE: // QUIT
            return false;
        case SDLK_F2:
            if(GB1->romloaded)
                GB1->save_state();
            break;
        case SDLK_F3:
        {
            if(++GB1_state_slot > 9)
                GB1_state_slot = 0;

            wchar_t dx_message[50];
            wsprintfW(dx_message,L"%s %d",str_table[STATE_SLOT],GB1_state_slot);
            showMessage(dx_message,60,GB1);
            break;
        }
        case SDLK_F4:
            if(GB1->romloaded)
                GB1->load_state();
            break;
        case SDLK_F5:
            toggleVideoLayer(VID_EN_BG, L"BG on", L"BG off");
            break;
        case SDLK_F6:
            toggleVideoLayer(VID_EN_WIN, L"WIN on", L"WIN off");
            break;
        case SDLK_F7:
            toggleVideoLayer(VID_EN_SPRITE, L"Sprites on", L"Sprites off");
            break;
        case SDLK_F12:
        {
            gb_system* screenshotGb = GB1;
            if (control && GB2) {
                screenshotGb = GB2;
            }

            if (!screenshotGb->romloaded) {
                break;
            }

            showMessage(L"Screenshot",40,screenshotGb);
            takeScreenshot(screenshotGb, "");
            break;
        }
        case SDLK_p: // PAUSE
            if(!control)
                break;
            menupause = !menupause;
            setPaused(!paused);
            break;
        case SDLK_f: // SOFT RESET
            if(!control)
                break;
            soft_reset = 1;
            break;
        case SDLK_r: // RESET
            if(!control)
                break;

            if(GB1->romloaded && !GB1->write_save())
                debug_print(str_table[ERROR_SAVE_FILE_WRITE]);

            GB1->reset();

            if(GB1->romloaded && !GB1->load_save())
                debug_print(str_table[ERROR_SAVE_FILE_READ]);
            break;
        case SDLK_d: // debug messages (the Windows version opens a log window)
            if(!control)
                break;
            sdlDebugLog = !sdlDebugLog;
            fprintf(stderr, "Debug messages %s\n", sdlDebugLog ? "on" : "off");
            showMessage(sdlDebugLog ? L"Debug log on" : L"Debug log off",40,GB1);
            break;
        case SDLK_l:
            if(!control)
                break;
            LinkerLog::SpawnLinkerLog();
            break;
        // change GBC palette for mono games
        case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4: case SDLK_5:
        case SDLK_6: case SDLK_7: case SDLK_8: case SDLK_9: case SDLK_0:
        case SDLK_BACKSLASH: case SDLK_EQUALS:
        {
            if(control)
                break;

            SDL_Keycode sym = key.keysym.sym;
            int paletteNo;
            if(sym >= SDLK_1 && sym <= SDLK_9) {
                paletteNo = sym - SDLK_1;
            } else if (sym == SDLK_0) {
                paletteNo = 9;
            } else if (sym == SDLK_BACKSLASH) {
                paletteNo = 10;
            } else {
                paletteNo = 11;
            }
            if(GB1->romloaded && GB1->gbc_mode && !GB1->cartridge->header.CGB) {
                memcpy(GB1->GBC_BGP,GBC_DMGBG_palettes[paletteNo],sizeof(unsigned int)*4);
                memcpy(GB1->GBC_OBP,GBC_DMGOBJ0_palettes[paletteNo],sizeof(unsigned int)*4);
                memcpy(GB1->GBC_OBP+4,GBC_DMGOBJ1_palettes[paletteNo],sizeof(unsigned int)*4);
            }
            if(GB2 && GB2->romloaded && GB2->gbc_mode && !GB2->cartridge->header.CGB) {
                memcpy(GB2->GBC_BGP,GBC_DMGBG_palettes[paletteNo],sizeof(unsigned int)*4);
                memcpy(GB2->GBC_OBP,GBC_DMGOBJ0_palettes[paletteNo],sizeof(unsigned int)*4);
                memcpy(GB2->GBC_OBP+4,GBC_DMGOBJ1_palettes[paletteNo],sizeof(unsigned int)*4);
            }
            break;
        }
    }

    return true;
}

// Returns false to quit
static bool handleEvent(const SDL_Event& event)
{
    switch(event.type)
    {
        case SDL_QUIT:
            return false;
        case SDL_KEYDOWN:
            return keyAction(event.key);
        case SDL_KEYUP:
            control_pressed = (event.key.keysym.mod & KMOD_CTRL) != 0;
            break;
        case SDL_DROPFILE:
        {
            wstring filename = utf8ToWide(event.drop.file);
            SDL_free(event.drop.file);

            if(!hasRomExtension(filename))
                debug_print(str_table[NOT_A_ROM]);
            else
                loadRom(filename);

            SDL_RaiseWindow(renderer.getWindow());
            break;
        }
        case SDL_WINDOWEVENT:
            switch(event.window.event)
            {
                case SDL_WINDOWEVENT_FOCUS_GAINED:
                    control_pressed = 0;
                    if(!menupause)
                        setPaused(false);
                    break;
                case SDL_WINDOWEVENT_FOCUS_LOST:
                    control_pressed = 0;
                    speedup = 0;
                    setPaused(true);
                    break;
                case SDL_WINDOWEVENT_EXPOSED:
                case SDL_WINDOWEVENT_SIZE_CHANGED:
                    renderer.handleWindowResize();
                    break;
            }
            break;
        default:
            sdlInputHandleEvent(event);
            break;
    }
    return true;
}

// Main loop -------------------------------------------------------------------

static void runFrame()
{
    // Runs until frame
    if(!multiple_gb)
    {
        do
        {
            GB1->mainloop();
        } while(GB1->frames < 1);
    } else
    {
        do
        {
            GB = GB1;

            GB1->mainloop();

            GB = GB2;

            GB2->mainloop();
        } while(GB1->frames < 1);
    }

    GB1->frames = 0;
}

static void mainLoop()
{
    const Uint64 frequency = SDL_GetPerformanceFrequency();
    const Uint64 frameTime = (Uint64)(frequency / 59.7275);

    Uint64 nextTime = SDL_GetPerformanceCounter();
    long framesRun = 0;

    for(;;)
    {
        SDL_Event event;
        bool quit = false;
        while(SDL_PollEvent(&event))
        {
            if(!handleEvent(event))
                quit = true;
        }
        if(quit)
            break;

        if(!emulating || !GB1->romloaded || paused)
        {
            if(framesToRun >= 0 && !GB1->romloaded)
                break; // nothing to run
            SDL_Delay(50);
            nextTime = SDL_GetPerformanceCounter();
            continue;
        }

        // When sound is playing, the audio output keeps us at the right speed
        bool timed = !speedup && !unthrottled && !sdlAudioIsPacing();

        Uint64 now = SDL_GetPerformanceCounter();
        if(timed && now < nextTime)
        {
            Uint32 sleepMs = (Uint32)((nextTime - now) * 1000 / frequency);
            if(sleepMs > 1)
                SDL_Delay(sleepMs - 1);
            while((now = SDL_GetPerformanceCounter()) < nextTime)
                ;
        }

        runFrame();

        nextTime = now + frameTime;

        if(framesToRun >= 0 && ++framesRun >= framesToRun)
            break;

        //-----Auto frameskip----
        if(timed && options->video_auto_frameskip)
        {
            if(SDL_GetPerformanceCounter() > nextTime)
            {
                if(options->video_frameskip < 9)
                    ++options->video_frameskip;
            }
            else if(options->video_frameskip > 0)
            {
                --options->video_frameskip;
            }
        }
    }
}

// Entry point -----------------------------------------------------------------

static void printUsage(const char* program)
{
    printf("hhugboy %s\n\n", wideToUtf8(prg_version).c_str());
    printf("Usage: %s [options] [rom file]\n\n", program);
    printf("Options:\n");
    printf("  --debug-log          print debug messages to the terminal (toggle with Ctrl+D)\n");
    printf("  --frames N           exit after running N frames\n");
    printf("  --screenshot FILE    save a PNG screenshot to FILE before exiting\n");
    printf("  --unthrottled        run as fast as possible\n");
    printf("  --no-dialogs         print errors to the terminal only\n");
    printf("  --help               show this message\n\n");
    printf("Keys: Esc quit, F2 save state, F3 change slot, F4 load state,\n");
    printf("      F5/F6/F7 toggle BG/window/sprites, F12 screenshot, Ctrl+P pause,\n");
    printf("      Ctrl+R reset, Ctrl+F soft reset, 1-0 \\ = GBC palette for mono games.\n");
    printf("Other settings are in hhugboy.cfg in the program directory.\n");
}

int main(int argc, char* argv[])
{
    setlocale(LC_ALL, "");

    string romArgument;

    for(int i = 1; i < argc; i++)
    {
        string arg = argv[i];
        if(arg == "--help" || arg == "-h")
        {
            printUsage(argv[0]);
            return 0;
        } else if(arg == "--debug-log")
        {
            sdlDebugLog = true;
        } else if(arg == "--frames" && i + 1 < argc)
        {
            framesToRun = atol(argv[++i]);
        } else if(arg == "--screenshot" && i + 1 < argc)
        {
            screenshotOnExit = absolutePath(argv[++i]);
        } else if(arg == "--unthrottled")
        {
            unthrottled = true;
        } else if(arg == "--no-dialogs")
        {
            sdlShowDialogs = false;
        } else if(arg.size() > 1 && arg[0] == '-')
        {
            fprintf(stderr, "Unknown option %s\n\n", arg.c_str());
            printUsage(argv[0]);
            return 1;
        } else
        {
            romArgument = absolutePath(arg);
        }
    }

    if(framesToRun >= 0)
        sdlShowDialogs = false; // don't block unattended runs

    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        fprintf(stderr, "Couldn't initialise SDL: %s\n", SDL_GetError());
        return 1;
    }

    options = new program_configuration;
    GB1 = new gb_system;

    if(!options || !GB1 || !GB1->init() || !sgb_init())
    {
        debug_print(str_table[ERROR_MEMORY]);
        SDL_Quit();
        return 1;
    }

    GB = GB1;

    if(!initGraphics())
    {
        SDL_Quit();
        return 1;
    }

    initConfigs();
    loadBootstrap();
    sdlInputInit();
    initSound();

    if(unthrottled)
        sdlAudioSetThrottle(false);

    if(!romArgument.empty())
        loadRom(utf8ToWide(romArgument));

    updateTitle();
    renderer.redraw();

    mainLoop();

    if(!screenshotOnExit.empty() && GB1->romloaded)
        takeScreenshot(GB1, screenshotOnExit);

    if(GB1->romloaded && !GB1->write_save())
        debug_print(str_table[ERROR_SAVE_FILE_WRITE]);

    if(GB2 && GB2->romloaded && !GB2->write_save())
        debug_print(str_table[ERROR_SAVE_FILE_WRITE]);

    write_config_file();
    cleanup();

    SDL_Quit();
    return 0;
}
