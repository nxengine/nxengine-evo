#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#if defined(__unix__) || defined(__APPLE__) || defined(__VITA__) || defined(__SWITCH__)
#include <sys/stat.h>
#elif defined(__HAIKU__)
#include <posix/sys/stat.h> // ugh
#elif defined(_WIN32) || defined(WIN32)
#include <windows.h>
#endif

#if defined(__VITA__)
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#endif

#include "ResourceManager.h"
#include "common/glob.h"
#include "common/misc.h"
#include "settings.h"

#include <json.hpp>

bool ResourceManager::fileExists(const std::string &filename)
{
#if defined(__unix__) || defined(__APPLE__) || defined(__HAIKU__) || defined(__VITA__) || defined(__SWITCH__)
  struct stat st;

  if (stat(filename.c_str(), &st) == 0)
  {
    return true;
  }
  else
  {
    return false;
  }
#elif defined(_WIN32) || defined(WIN32) // Windows
  DWORD attrs = GetFileAttributes(widen(filename).c_str());

  // Assume path exists
  if (attrs != INVALID_FILE_ATTRIBUTES)
  {
    return true;
  }
  else
  {
    return false;
  }
#else
#error Platform not supported
#endif
  return false;
}

std::string ResourceManager::getBasePath()
{
  #if defined(__SWITCH__)
    return std::string("romfs:/");
  #else
    char* resPath = SDL_GetBasePath();
    if (NULL != resPath)
    {
        std::string strpath(resPath);
        SDL_free(resPath);
        return strpath;
    }
    return std::string("");
  #endif
}

std::string ResourceManager::getUserPrefPath()
{
#if defined(__SWITCH__)
  return std::string("/switch/nxengine/");
#else
  char *path = SDL_GetPrefPath(NULL, "nxengine");
  if (NULL != path)
  {
    std::string strpath(path);
    SDL_free(path);
    return strpath;

