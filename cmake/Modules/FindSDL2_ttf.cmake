# Locate SDL_ttf library
# This module defines
# SDL2TTF_LIBRARY, the name of the library to link against
# SDLTTF_FOUND, if false, do not try to link to SDL
# SDLTTF2_INCLUDE_DIR, where to find SDL/SDL.h
#
# $SDLDIR is an environment variable that would
# correspond to the ./configure --prefix=$SDLDIR
# used in building SDL.
#
# Created by Eric Wing. This was influenced by the FindSDL.cmake 
# module, but with modifications to recognize OS X frameworks and 
# additional Unix paths (FreeBSD, etc).

#=============================================================================
# Copyright 2005-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distributed this file outside of CMake, substitute the full
#  License text for the above reference.)

FIND_PATH(SDLTTF2_INCLUDE_DIR SDL_ttf.h
  HINTS
  $ENV{SDLTTFDIR}
  $ENV{SDLDIR}
  PATH_SUFFIXES include
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local/include/SDL2
  /usr/include/SDL2
  /sw/include/SDL2 # Fink
  /opt/local/include/SDL2 # DarwinPorts
  /opt/csw/include/SDL2 # Blastwave
  /opt/include/SDL2
)

FIND_LIBRARY(SDL2TTF_LIBRARY 
  NAMES SDL2_ttf
  HINTS
  $ENV{SDLTTFDIR}
  $ENV{SDLDIR}
  PATHS
  ~/Library/Frameworks
  /Library/Frameworks
  /usr/local
  /usr
  /sw
  /opt/local
  /opt/csw
  /opt
    PATH_SUFFIXES lib64 lib
)

SET(SDL2TTF_FOUND "NO")
IF(SDL2TTF_LIBRARY AND SDLTTF2_INCLUDE_DIR)
  SET(SDL2TTF_FOUND "YES")
ENDIF(SDL2TTF_LIBRARY AND SDLTTF2_INCLUDE_DIR)

