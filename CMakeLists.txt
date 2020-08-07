cmake_minimum_required(VERSION 2.6)
include(CheckCXXCompilerFlag)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

option(LOW_END_HARDWARE "Build for low-end hardware" ON)
set(PLATFORM "pc" CACHE STRING "Platform to build for")
set_property(CACHE PLATFORM PROPERTY STRINGS pc vita switch)

IF(PLATFORM STREQUAL "pc")
ELSEIF(PLATFORM STREQUAL "vita")
    set(LOW_END_HARDWARE ON)
    IF(DEFINED ENV{VITASDK})
        include("$ENV{VITASDK}/share/vita.toolchain.cmake" REQUIRED)
        include("${VITASDK}/share/vita.cmake" REQUIRED)
        set(VITA_APP_NAME "NXENGINE-EVO")
        set(VITA_TITLEID  "NXEV00001")
        set(VITA_VERSION  "02.64")
        set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")
    ELSE()
        message(FATAL_ERROR "Please define VITASDK to point to your SDK path!")
    ENDIF()
ELSEIF(PLATFORM STREQUAL "switch")
    set(LOW_END_HARDWARE ON)
    IF(DEFINED ENV{DEVKITPRO})
        include("cmake/switch.toolchain.cmake" REQUIRED)
        include("cmake/switch.tools.cmake" REQUIRED)
    ELSE()
        message(FATAL_ERROR "Please define DEVKITPRO to point to your SDK path!")
    ENDIF()

ELSE()
    message(FATAL_ERROR "Wrong platform")
ENDIF()

project(nx)

set (nx_VERSION_MAJOR 2)
set (nx_VERSION_MINOR 6)
set (nx_VERSION_RELEASE 4)

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(PNG REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL_MIXER_INCLUDE_DIR})
include_directories(${PNG_PNG_INCLUDE_DIR})
include_directories("${nx_SOURCE_DIR}/deps")

file(GLOB APP_SOURCES "src/[a-zA-Z]*.cpp")
file(GLOB TB_SOURCES "src/TextBox/[a-zA-Z]*.cpp")
file(GLOB_RECURSE AI_SOURCES "src/ai/[a-zA-Z]*.cpp")
file(GLOB AG_SOURCES "src/autogen/[a-zA-Z]*.cpp")
file(GLOB CM_SOURCES "src/common/[a-zA-Z]*.cpp")
file(GLOB UT_SOURCES "src/Utils/[a-zA-Z]*.cpp")
file(GLOB EG_SOURCES "src/endgame/[a-zA-Z]*.cpp")
file(GLOB GR_SOURCES "src/graphics/[a-zA-Z]*.cpp")
file(GLOB IN_SOURCES "src/intro/[a-zA-Z]*.cpp")
file(GLOB PA_SOURCES "src/pause/[a-zA-Z]*.cpp")
file(GLOB SL_SOURCES "src/siflib/[a-zA-Z]*.cpp")
file(GLOB SN_SOURCES "src/sound/[a-zA-Z]*.cpp")
file(GLOB I18N_SOURCES "src/i18n/[a-zA-Z]*.cpp")
file(GLOB EXTR_SOURCES "src/extract/[a-zA-Z]*.cpp")
set(EXTR_SOURCES
    ${EXTR_SOURCES}
    "src/common/misc.cpp"
    "src/Utils/Logger.cpp"
    "src/stagedata.cpp"
)

include_directories(${nx_SOURCE_DIR})

set(SOURCES
    ${APP_SOURCES}
    ${TB_SOURCES}
    ${AI_SOURCES}
    ${AG_SOURCES}
    ${CM_SOURCES}
    ${EG_SOURCES}
    ${GR_SOURCES}
    ${IN_SOURCES}
    ${PA_SOURCES}
    ${SL_SOURCES}
    ${SN_SOURCES}
    ${I18N_SOURCES}
    ${UT_SOURCES}
)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    message("debug mode")
    add_definitions(-DTRACE_SCRIPT)
    add_definitions(-DDEBUG)
ENDIF()

IF(LOW_END_HARDWARE)
    add_definitions(-D_LOW_END_HARDWARE)
ENDIF()

add_definitions("-Wall")

add_executable(nx ${SOURCES})

IF(PLATFORM STREQUAL "pc")
    add_definitions("-std=c++11")
    target_link_libraries(nx ${SDL2_LIBRARY} ${SDL_MIXER_LIBRARY} ${PNG_LIBRARY})

ELSEIF(PLATFORM STREQUAL "vita")
    add_definitions("-std=gnu++11")
    add_definitions("-D__VITA__")
    target_link_libraries(nx ${SDL_MIXER_LIBRARY} ${SDL2_LIBRARY} ${PNG_LIBRARY}
      vita2d
      SceDisplay_stub
      SceCtrl_stub
      SceAudio_stub
      SceSysmodule_stub
      SceGxm_stub
      SceCommonDialog_stub
      SceTouch_stub
      SceHid_stub
      m
      z
      FLAC
      vorbisfile
      vorbis
      ogg
      mikmod
      mpg123
    )

  vita_create_self(${PROJECT_NAME}.self ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/nx)
  vita_create_vpk(${PROJECT_NAME}.vpk ${VITA_TITLEID} ${PROJECT_NAME}.self
    VERSION ${VITA_VERSION}
    NAME ${VITA_APP_NAME}
    FILE platform/vita/sce_sys sce_sys
    FILE release/data data
  )
  add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.vpk
    COMMAND cp nx.vpk ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.vpk
    DEPENDS nx.vpk
    COMMENT "Moving vpk to release"
  )
  add_custom_target(nxbinvpk_ ALL DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.vpk)

ELSEIF(PLATFORM STREQUAL "switch")
    add_definitions("-std=gnu++11")
    add_definitions("-D__SWITCH__")
    target_link_libraries(nx SDL2_mixer SDL2 png
      m
      z
      FLAC
      vorbisidec
      ogg
      mikmod
      mpg123
      modplug
      EGL
      glapi
      drm_nouveau
      -lnx
      opusfile
      opus
    )

    add_nro_target(nx "NXEngine-evo" "NXEngine team" "${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}" "${CMAKE_SOURCE_DIR}/platform/switch/icon.jpg" "${CMAKE_SOURCE_DIR}/release")
    add_custom_command(OUTPUT ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.nro
      COMMAND cp nx.nro ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.nro
      DEPENDS nx.nro
      COMMENT "Moving nro to release"
    )
    add_custom_target(nxbinnro_ ALL DEPENDS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.nro)
ENDIF()

IF(PLATFORM STREQUAL "pc")
    add_executable(extract ${EXTR_SOURCES})
    target_link_libraries(extract ${SDL2_LIBRARY})
ENDIF()


