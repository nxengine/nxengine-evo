cmake_minimum_required(VERSION 2.6)


include(CheckCXXCompilerFlag)

project(nx)

set (nx_VERSION_MAJOR 2)
set (nx_VERSION_MINOR 0)


set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
#include(${CMAKE_SOURCE_DIR}/cmake/Modules/misc.cmake)

find_package(SDL)
find_package(SDL_ttf)
#find_package(SDL2_mixer)

include_directories(${SDL_INCLUDE_DIR})
include_directories(${SDLTTF_INCLUDE_DIR})

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

#add_subdirectory(game)
#add_subdirectory(gameui)
#add_subdirectory(scripting)
#add_subdirectory(sound)
#add_subdirectory(ui)
#add_subdirectory(utils)
#add_subdirectory(render/opengl1)

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

add_executable(nx ${SOURCES})
target_link_libraries(nx ${SDL_LIBRARY} ${SDLTTF_LIBRARY})
#target_link_libraries(rde ${SDL_LIBRARY} ${SDL_IMAGE_LIBRARY} ${SDL_MIXER_LIBRARY} ${LUA_LIBRARY} ${PHYSFS_LIBRARY} ${FREETYPE_LIBRARY} ${PNG_LIBRARY})

