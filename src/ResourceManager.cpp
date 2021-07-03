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
  char *path = SDL_GetPrefPath(NULL, "nxengine");
  if (NULL != path)
  {
    std::string strpath(path);
    SDL_free(path);
    return strpath;
  }

  return std::string("");
}

ResourceManager::ResourceManager()
{
  findLanguages();
  findMods();
}

ResourceManager::~ResourceManager() {}

ResourceManager *ResourceManager::getInstance()
{
  return Singleton<ResourceManager>::get();
}

void ResourceManager::shutdown() {}

std::string ResourceManager::getPath(const std::string &filename, bool localized)
{
  std::vector<std::string> _paths;

  std::string userResBase = getUserPrefPath();

  if (!userResBase.empty())
  {
    if (!_mod.empty())
    {
      if (localized) _paths.push_back(userResBase + "data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
      _paths.push_back(userResBase + "data/mods/" + _mod + "/" + filename);
    }
    if (localized) _paths.push_back(userResBase + "data/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back(userResBase + "data/" + filename);
  }

  #if defined(DATADIR)
    std::string _data(DATADIR);
  #else
    std::string _data = getBasePath();

    #if defined(HAVE_UNIX_LIKE) and !defined(PORTABLE)
      _data += "../share/nxengine/data/";
    #else
      _data += "data/";
    #endif
  #endif

  if (!_mod.empty())
  {
    if (localized) _paths.push_back(_data + "mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back(_data + "mods/" + _mod + "/" + filename);
  }

  if (localized) _paths.push_back(_data + "lang/" + std::string(settings->language) + "/" + filename);
  _paths.push_back(_data + filename);

  for (auto &_tryPath: _paths)
  {
    if (fileExists(_tryPath))
      return _tryPath;
  }

  return _paths.back();
}

std::string ResourceManager::getPrefPath(const std::string &filename)
{
  return std::string(getUserPrefPath()) + std::string(filename);
}

std::string ResourceManager::getPathForDir(const std::string &dir)
{
  return getPath(dir, false);
}

inline std::vector<std::string> glob(const std::string &pat)
{
  Glob search(pat);
  std::vector<std::string> ret;
  while (search)
  {
    ret.push_back(search.GetFileName());
    search.Next();
  }
  return ret;
}

void ResourceManager::findLanguages()
{
  std::vector<std::string> langs = glob(getPathForDir("lang/") + "*");
  _languages.push_back("english");
  for (auto &l : langs)
  {
    std::ifstream ifs(widen(l + "/system.json"));
    if (ifs.is_open())
    {
      ifs.close();
      _languages.push_back(l.substr(l.find_last_of('/') + 1));
    }
  }
}

std::vector<std::string> &ResourceManager::languages()
{
  return _languages;
}

void ResourceManager::findMods()
{
  std::vector<std::string> mods=glob(getPathForDir("mods/")+"*");
  for (auto &l: mods)
  {
//    std::cout << l << std::endl;
    std::ifstream ifs(widen(l+"/mod.json"), std::ifstream::in | std::ifstream::binary);
    if (ifs.is_open())
    {
      nlohmann::json modfile = nlohmann::json::parse(ifs);
      _mods.insert(
        {
          l.substr(l.find_last_of('/')+1),
          Mod{l.substr(l.find_last_of('/')+1), modfile["name"].get<std::string>(), modfile["skip-intro"].get<bool>()}
        }
      );

      ifs.close();
    }
  }
}

Mod& ResourceManager::mod(std::string& name)
{
  return _mods[name];
}

void ResourceManager::setMod(std::string name)
{
  _mod = name;
}

Mod& ResourceManager::mod()
{
  return _mods[_mod];
}

bool ResourceManager::isMod()
{
  return !_mod.empty();
}

std::map<std::string, Mod> &ResourceManager::mods()
{
  return _mods;
}
