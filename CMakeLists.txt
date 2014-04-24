cmake_minimum_required(VERSION 2.6)


include(CheckCXXCompilerFlag)

project(nx)

set (nx_VERSION_MAJOR 2)
set (nx_VERSION_MINOR 1)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

find_package(SDL2)
find_package(SDL2_ttf)

include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2TTF_INCLUDE_DIR})

file(GLOB APP_SOURCES "[a-zA-Z]*.cpp")
file(GLOB TB_SOURCES "TextBox/[a-zA-Z]*.cpp")
file(GLOB_RECURSE AI_SOURCES "ai/[a-zA-Z]*.cpp")
file(GLOB AG_SOURCES "autogen/[a-zA-Z]*.cpp")
file(GLOB CM_SOURCES "common/[a-zA-Z]*.cpp")
file(GLOB EG_SOURCES "endgame/[a-zA-Z]*.cpp")
file(GLOB GR_SOURCES "graphics/[a-zA-Z]*.cpp")
file(GLOB IN_SOURCES "intro/[a-zA-Z]*.cpp")
file(GLOB PA_SOURCES "pause/[a-zA-Z]*.cpp")
file(GLOB SL_SOURCES "siflib/[a-zA-Z]*.cpp")
file(GLOB SN_SOURCES "sound/[a-zA-Z]*.cpp")

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
)

option(DEBUG "build application with debug traces" OFF) # OFF is the default
option(MUTABLE_SCALE "build application with changable scale" ON) # OFF is the default
option(USE_TTF "build application with ttf font support" ON) # OFF is the default

if(DEBUG)
  add_definitions(-DTRACE_SCRIPT)
endif(DEBUG)

if(MUTABLE_SCALE)
    add_definitions(-DCONFIG_MUTABLE_SCALE)
endif(MUTABLE_SCALE)

if(USE_TTF)
    add_definitions(-DCONFIG_ENABLE_TTF)
endif(USE_TTF)

add_definitions(-DNEWMENU)

add_executable(nx ${SOURCES})
target_link_libraries(nx ${SDL2_LIBRARY} ${SDL2TTF_LIBRARY})

