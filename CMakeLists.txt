cmake_minimum_required(VERSION 3.6)
project(hhugboy)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

# Emulator core, shared by all platforms
set(CORE_SOURCE_FILES
        src/rendering/filters/Filter.cpp
        src/rendering/filters/Filter.h
        src/rendering/filters/filters.cpp
        src/rendering/filters/filters.h
        src/rendering/filters/scale2x.cpp
        src/rendering/filters/scale2x.h
        src/rendering/filters/scale3x.cpp
        src/rendering/filters/scale3x.h
        src/rendering/palette.cpp
        src/rendering/palette.h
        src/rendering/render.cpp
        src/rendering/render.h
        src/zlib/unzip.c
        src/zlib/unzip.h
        src/cheats.cpp
        src/cheats.h
        src/config.cpp
        src/config.h
        src/state.cpp
        src/state.h
        src/cpu.h
        src/cpu.cpp
        src/cpucore.h
        src/debug.cpp
        src/debug.h
        src/devices.cpp
        src/devices.h
        src/globals.cpp
        src/input.cpp
        src/input.h
        src/GB.cpp
        src/GB.h
        src/GB_gfx.cpp
        src/GB_gfx.h
        src/memory/Cartridge.h
        src/memory/GB_ioregs.cpp
        src/memory/Cartridge.cpp
        src/memory/GB_mem.cpp
        src/GB_sound.cpp
        src/licnames.h
        src/main.h
        src/mainloop.cpp
        src/mainloop.h
        src/ui/menu.h
        src/options.h
        src/rom.cpp
        src/rom.h
        src/SGB.cpp
        src/SGB.h
        src/sound.h
        src/ui/strings.cpp
        src/ui/strings.h
        src/types.h
        src/memory/mbc/BasicMbc.cpp
        src/memory/mbc/BasicMbc.h
        src/memory/mbc/MbcNin3.cpp
        src/memory/mbc/MbcNin3.h
        src/memory/mbc/MbcNin7.cpp
        src/memory/mbc/MbcNin7.h
        src/memory/mbc/MbcLicHuc3.cpp
        src/memory/mbc/MbcLicHuc3.h
        src/memory/mbc/MbcLicTama5.cpp
        src/memory/mbc/MbcLicTama5.h
        src/memory/mbc/MbcUnlSintax.cpp
        src/memory/mbc/MbcUnlSintax.h
        src/memory/mbc/AbstractMbc.cpp
        src/memory/mbc/AbstractMbc.h
        src/memory/mbc/MbcNinCamera.cpp
        src/memory/mbc/MbcNinCamera.h
        src/memory/mbc/RomOnly.cpp
        src/memory/mbc/RomOnly.h
        src/memory/mbc/MbcNin5.cpp
        src/memory/mbc/MbcNin5.h
        src/memory/mbc/MbcUnlLiCheng.cpp
        src/memory/mbc/MbcUnlLiCheng.h
        src/memory/mbc/MbcUnlPoke2in1.cpp
        src/memory/mbc/MbcUnlPoke2in1.h
        src/memory/mbc/MbcNin2.cpp
        src/memory/mbc/MbcNin2.h
        src/memory/mbc/MbcUnlRockman8.cpp
        src/memory/mbc/MbcUnlRockman8.h
        src/memory/mbc/MbcLicMk12.cpp
        src/memory/mbc/MbcLicMk12.h
        src/memory/mbc/MbcNin1Multi.cpp
        src/memory/mbc/MbcNin1Multi.h
        src/memory/mbc/MbcLicMmm01.cpp
        src/memory/mbc/MbcLicMmm01.h
        src/memory/mbc/MbcNin1.cpp
        src/memory/mbc/MbcNin1.h
        src/memory/mbc/MbcUnlLbMulti.cpp
        src/memory/mbc/MbcUnlLbMulti.h
        src/memory/mbc/MbcUnlBbd.cpp
        src/memory/mbc/MbcUnlBbd.h
        src/memory/mbc/MbcUnlDbzTrans.cpp
        src/memory/mbc/MbcUnlDbzTrans.h
        src/memory/MbcType.h
        src/memory/mbc/MbcUnlHitek.cpp
        src/memory/mbc/MbcUnlHitek.h
        src/memory/mbc/MbcUnlNtOld1.cpp
        src/memory/mbc/MbcUnlNtOld1.h
        src/memory/mbc/MbcUnlNtOld2.cpp
        src/memory/mbc/MbcUnlNtOld2.h
        src/memory/CartDetection.h
        src/memory/CartDetection.cpp
        src/ui/window.h
        src/memory/mbc/MbcUnlNtNew.cpp
        src/memory/mbc/MbcUnlNtNew.h
        src/memory/GbxParser.cpp
        src/memory/GbxParser.h
        src/memory/mbc/MbcUnlPokeJadeDia.cpp
        src/memory/mbc/MbcUnlPokeJadeDia.h
        src/memory/mbc/MbcUnlWisdomTree.cpp
        src/memory/mbc/MbcUnlWisdomTree.h
        src/memory/mbc/MbcUnlSachenMMC1.cpp
        src/memory/mbc/MbcUnlSachenMMC1.h
        src/memory/mbc/MbcUnlSachenMMC2.cpp
        src/memory/mbc/MbcUnlSachenMMC2.h
        src/memory/mbc/MbcLicM161.cpp
        src/memory/mbc/MbcLicM161.h
        src/memory/mbc/MbcUnlRocketGames.cpp
        src/memory/mbc/MbcUnlRocketGames.h
        src/memory/mbc/MbcNin5_LogoSwitch.cpp
        src/memory/mbc/MbcNin5_LogoSwitch.h
        src/memory/mbc/MbcUnlNewGbHk.cpp
        src/memory/mbc/MbcUnlNewGbHk.h
        src/memory/mbc/MbcUnlGgb81.cpp
        src/memory/mbc/MbcUnlGgb81.h
        src/ui/dialogs/LinkerLog.h
        src/memory/linker/LinkerWrangler.cpp
        src/memory/linker/LinkerWrangler.h
        src/memory/mbc/MbcUnlVf001.cpp
        src/memory/mbc/MbcUnlVf001.h
        src/memory/mbc/MbcUnlSkobLee8.cpp
        src/memory/mbc/MbcUnlSkobLee8.h
        src/platform/compat.cpp
        src/platform/compat.h
        src/platform/wintypes.h
        src/platform/dikcodes.h
)

if(WIN32)

    # Windows frontend: Win32 UI, DirectDraw, DirectInput and FMOD, using the bundled libraries

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc -static-libstdc++ -std=c++11 -U__STRICT_ANSI__ -mwindows")

    set(PLATFORM_SOURCE_FILES
            src/rendering/directdraw.cpp
            src/rendering/directdraw.h
            src/rendering/renderer.cpp
            src/rendering/renderer.h
            src/ui/dialogs.cpp
            src/ui/dialogs.h
            src/directinput.cpp
            src/directinput.h
            src/ui/keynames.h
            src/main.cpp
            src/ui/menu.cpp
            src/ui/menu.rc
            src/version.rc
            src/ui/menuactions.cpp
            src/ui/menuactions.h
            src/ui/keyactions.cpp
            src/ui/keyactions.h
            src/ui/window.cpp
            src/ui/dialogs/MemorySearcher.h
            src/ui/dialogs/MemorySearcher.rc
            src/ui/dialogs/MemorySearcher.cpp
            src/rendering/DirectDrawStuff.cpp
            src/rendering/DirectDrawStuff.h
            src/ui/dialogs/LinkerLog.cpp
            src/ui/dialogs/LinkerLog.rc
            src/sound_fmod.cpp
    )

    include_directories(
            includes
            includes/zlib
            includes/libpng12
            includes/fmod
            includes/directx-aug2009
            includes/png++-0.2.5
    )

    link_libraries(
            "${CMAKE_SOURCE_DIR}/libs/libfmod.a"
            "${CMAKE_SOURCE_DIR}/libs/libwinmm.a"
            "${CMAKE_SOURCE_DIR}/libs/libdinput.a"
            "${CMAKE_SOURCE_DIR}/libs/libddraw.a"
            "${CMAKE_SOURCE_DIR}/libs/libdinput.a"
            "${CMAKE_SOURCE_DIR}/libs/libdinput8.a"
            "${CMAKE_SOURCE_DIR}/libs/libdxguid.a"
            "${CMAKE_SOURCE_DIR}/libs/libpng12.dll.a"
            "${CMAKE_SOURCE_DIR}/libs/libz.a"
    )

else()

    # SDL2 frontend for Linux and other platforms, using the system's libraries

    set(CMAKE_CXX_STANDARD 11)

    find_package(SDL2 REQUIRED)
    find_package(PNG REQUIRED)
    find_package(ZLIB REQUIRED)

    add_definitions(-DHHUGBOY_SDL)

    set(PLATFORM_SOURCE_FILES
            src/platform/sdl/SdlAudio.cpp
            src/platform/sdl/SdlFont.cpp
            src/platform/sdl/SdlFont.h
            src/platform/sdl/SdlFrontend.h
            src/platform/sdl/SdlInput.cpp
            src/platform/sdl/SdlMain.cpp
            src/platform/sdl/SdlRenderer.cpp
            src/platform/sdl/SdlRenderer.h
    )

    include_directories(
            includes/png++-0.2.5
    )

    if(TARGET SDL2::SDL2)
        set(HHUGBOY_SDL2_LIBRARIES SDL2::SDL2)
    else()
        include_directories(${SDL2_INCLUDE_DIRS})
        set(HHUGBOY_SDL2_LIBRARIES ${SDL2_LIBRARIES})
    endif()

    link_libraries(
            ${HHUGBOY_SDL2_LIBRARIES}
            PNG::PNG
            ZLIB::ZLIB
            ${CMAKE_DL_LIBS}
    )

endif()

set(SOURCE_FILES ${CORE_SOURCE_FILES} ${PLATFORM_SOURCE_FILES})

add_executable(hhugboy ${SOURCE_FILES})
set_target_properties(hhugboy PROPERTIES LINKER_LANGUAGE CXX)
add_executable(hhugboy-debug ${SOURCE_FILES})
set_target_properties(hhugboy-debug PROPERTIES LINKER_LANGUAGE CXX)