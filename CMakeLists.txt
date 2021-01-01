cmake_minimum_required(VERSION 2.6)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")

option(LINEAR_INTERPOLATION "Use linear interpolation instead of cubic interpolation." ON)
option(LOW_END_HARDWARE "Build for low-end hardware" ON)
set(PLATFORM "pc" CACHE STRING "Platform to build for")
set_property(CACHE PLATFORM PROPERTY STRINGS pc vita switch)

IF(PLATFORM STREQUAL "pc")
ELSEIF(PLATFORM STREQUAL "vita")
    set(LOW_END_HARDWARE ON)
    IF(DEFINED ENV{DOLCESDK})
        include("$ENV{DOLCESDK}/share/dolce.toolchain.cmake" REQUIRED)
        include("$ENV{DOLCESDK}/share/dolce.cmake" REQUIRED)
        set(DOLCE_APP_NAME "NXENGINE-EVO")
        set(DOLCE_TITLEID  "NXEV00001")
        set(DOLCE_VERSION  "02.64")
        set(DOLCE_MKSFOEX_FLAGS "${DOLCE_MKSFOEX_FLAGS} -d PARENTAL_LEVEL=1")
        set(DOLCE_ELF_CREATE_FLAGS "${DOLCE_ELF_CREATE_FLAGS} -h 4194304")
    ELSE()
        message(FATAL_ERROR "Please define DOLCESDK to point to your SDK path!")
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

project(nx CXX)

set (nx_VERSION_MAJOR 2)
set (nx_VERSION_MINOR 6)
set (nx_VERSION_RELEASE 4)
set (nx_APP_ID org.nxengine.nxengine_evo)

include(CheckCXXCompilerFlag)
include(GNUInstallDirs)


find_package(SDL2 REQUIRED)
find_package(SDL2_mixer REQUIRED)
find_package(SDL2_image REQUIRED)
find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)

include_directories(${SDL2_INCLUDE_DIR})
include_directories(${SDL2_MIXER_INCLUDE_DIR})
include_directories(${SDL2_IMAGE_INCLUDE_DIR})
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

IF(LINEAR_INTERPOLATION)
    add_definitions(-D_LINEAR_INTERPOLATION)
ENDIF()

IF(LOW_END_HARDWARE)
    add_definitions(-D_LOW_END_HARDWARE)
ENDIF()

add_definitions("-Wall")

add_executable(nx ${SOURCES})

IF(PLATFORM STREQUAL "pc")
set_property(TARGET nx PROPERTY OUTPUT_NAME nxengine-evo)
    add_definitions("-std=c++11")
    target_link_libraries(nx ${SDL2_LIBRARY} ${SDL2_MIXER_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${PNG_LIBRARY} ${JPEG_LIBRARY})

ELSEIF(PLATFORM STREQUAL "vita")
    add_definitions("-std=gnu++11")
    add_definitions("-D__VITA__")
    target_link_libraries(nx ${SDL2_MIXER_LIBRARY} ${SDL2_IMAGE_LIBRARY} ${SDL2_LIBRARY} ${PNG_LIBRARY} ${JPEG_LIBRARY}
      pib
      SceDisplay_stub
      SceCtrl_stub
      SceAppMgr_stub
      SceAudio_stub
      SceSysmodule_stub
      SceSharedFb_stub
      SceDisplay_stub
      SceDisplayUser_stub
      SceCtrl_stub
      SceAppMgr_stub
      SceAppMgrUser_stub
      SceAudio_stub
      SceIofilemgr_stub
      SceSysmodule_stub
      SceGxm_stub
      SceGxmInternalForVsh_stub
      SceCommonDialog_stub
      SceTouch_stub
      SceHid_stub
      SceMotion_stub
      m
      z
      FLAC
      vorbisfile
      vorbis
      ogg
      mikmod
      mpg123
    )

  dolce_create_self(${PROJECT_NAME}.self nx UNSAFE)
  dolce_create_vpk(${PROJECT_NAME}.vpk ${DOLCE_TITLEID} ${PROJECT_NAME}.self
    VERSION ${DOLCE_VERSION}
    NAME ${DOLCE_APP_NAME}
    FILE platform/vita/sce_sys sce_sys
    FILE release/data data
  )
  add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.vpk
    COMMAND cp nx.vpk ${CMAKE_SOURCE_DIR}/release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.vpk
    DEPENDS nx.vpk
    COMMENT "Moving vpk to release"
  )
  add_custom_target(nxbinvpk_ ALL DEPENDS ${CMAKE_SOURCE_DIR}/release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.vpk)

ELSEIF(PLATFORM STREQUAL "switch")
    add_definitions("-std=gnu++11")
    add_definitions("-D__SWITCH__")
    target_link_libraries(nx SDL2_mixer SDL2_image SDL2 png jpeg webp
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
    add_custom_command(OUTPUT ${CMAKE_SOURCE_DIR}/release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.nro
      COMMAND cp nx.nro ${CMAKE_SOURCE_DIR}/release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.nro
      DEPENDS nx.nro
      COMMENT "Moving nro to release"
    )
    add_custom_target(nxbinnro_ ALL DEPENDS ${CMAKE_SOURCE_DIR}/release/NXEngine_Evo-${nx_VERSION_MAJOR}.${nx_VERSION_MINOR}.${nx_VERSION_RELEASE}.nro)
ENDIF()

IF(PLATFORM STREQUAL "pc")
    add_executable(extract ${EXTR_SOURCES})
    target_link_libraries(extract ${SDL2_LIBRARY})
    set_property(TARGET extract PROPERTY OUTPUT_NAME nxextract)

    install(TARGETS nx extract RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
    install(DIRECTORY data DESTINATION ${CMAKE_INSTALL_DATADIR}/nxengine)

    # Install XDG metadata on Desktop Linux like platforms
    IF(UNIX AND NOT APPLE AND NOT ANDROID)
        install(FILES platform/xdg/${nx_APP_ID}.desktop     DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
        install(FILES platform/xdg/${nx_APP_ID}.png         DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/256x256/apps)
        install(FILES platform/xdg/${nx_APP_ID}.appdata.xml DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
    ENDIF()
ENDIF()


