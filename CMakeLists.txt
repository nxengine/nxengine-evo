cmake_minimum_required(VERSION 2.6)


include(CheckCXXCompilerFlag)

project(nx)

set (nx_VERSION_MAJOR 2)
set (nx_VERSION_MINOR 4)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(SDL2  REQUIRED)
find_package(SDL2_ttf  REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(PNG)

include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2TTF_INCLUDE_DIR})

file(GLOB APP_SOURCES "src/[a-zA-Z]*.cpp")
file(GLOB TB_SOURCES "src/TextBox/[a-zA-Z]*.cpp")
file(GLOB_RECURSE AI_SOURCES "src/ai/[a-zA-Z]*.cpp")
file(GLOB AG_SOURCES "src/autogen/[a-zA-Z]*.cpp")
file(GLOB CM_SOURCES "src/common/[a-zA-Z]*.cpp")
file(GLOB EG_SOURCES "src/endgame/[a-zA-Z]*.cpp")
file(GLOB GR_SOURCES "src/graphics/[a-zA-Z]*.cpp")
file(GLOB IN_SOURCES "src/intro/[a-zA-Z]*.cpp")
file(GLOB PA_SOURCES "src/pause/[a-zA-Z]*.cpp")
file(GLOB SL_SOURCES "src/siflib/[a-zA-Z]*.cpp")
file(GLOB SN_SOURCES "src/sound/[a-zA-Z]*.cpp")
file(GLOB EXTR_SOURCES "src/extract/[a-zA-Z]*.cpp")
set(EXTR_SOURCES
    ${EXTR_SOURCES}
    "src/common/misc.cpp"
    "src/common/stat.cpp"
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

add_definitions("-Wall -std=c++11")
add_executable(nx ${SOURCES})
target_link_libraries(nx ${SDL2_LIBRARY} ${SDL2TTF_LIBRARY} ${SDL_MIXER_LIBRARY} ${PNG_LIBRARY})

add_executable(extract ${EXTR_SOURCES})


