cmake_minimum_required(VERSION 2.6)


include(CheckCXXCompilerFlag)

project(nx)

set (nx_VERSION_MAJOR 2)
set (nx_VERSION_MINOR 6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin)

find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(PNG REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})

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
file(GLOB I18N_SOURCES "src/i18n/[a-zA-Z]*.cpp")
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
    ${I18N_SOURCES}
)

IF(CMAKE_BUILD_TYPE MATCHES Debug)
    message("debug mode")
    add_definitions(-DTRACE_SCRIPT)
    add_definitions(-DDEBUG)
ENDIF()

add_definitions("-Wall -std=c++11")
add_executable(nx ${SOURCES})
target_link_libraries(nx ${SDL2_LIBRARY} ${SDL_MIXER_LIBRARY} ${PNG_LIBRARY})

add_executable(extract ${EXTR_SOURCES})


