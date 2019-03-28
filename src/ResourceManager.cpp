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
#endif

#include "ResourceManager.h"
#include "common/glob.h"
#include "common/json.hpp"
#include "common/misc.h"
#include "settings.h"

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

std::string ResourceManager::getLocalizedPath(const std::string &filename)
{
  std::vector<std::string> _paths;

#if defined(__linux__)
  char *home = getenv("HOME");

  if (home != NULL)
  {
    if (!_mod.empty())
    {
      _paths.push_back(std::string(home) + "/.local/share/nxengine/data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
      _paths.push_back(std::string(home) + "/.local/share/nxengine/data/mods/" + _mod + "/" + filename);
    }
    _paths.push_back(std::string(home) + "/.local/share/nxengine/data/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back(std::string(home) + "/.local/share/nxengine/data/" + filename);
  }

  if (!_mod.empty())
  {
    _paths.push_back("/usr/share/nxengine/data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back("/usr/share/nxengine/data/mods/" + _mod + "/" + filename);

    _paths.push_back("/usr/local/share/nxengine/data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back("/usr/local/share/nxengine/data/mods/" + _mod + "/" + filename);
  }
  _paths.push_back("/usr/share/nxengine/data/lang/" + std::string(settings->language) + "/" + filename);
  _paths.push_back("/usr/share/nxengine/data/" + filename);

  _paths.push_back("/usr/local/share/nxengine/data/lang/" + std::string(settings->language) + "/" + filename);
  _paths.push_back("/usr/local/share/nxengine/data/" + filename);

#elif defined(__APPLE__)
  char *home = SDL_GetPrefPath("nxengine", "nxengine-evo");

  if (home != NULL)
  {
    if (!_mod.empty())
    {
      _paths.push_back(std::string(home) + "/data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
      _paths.push_back(std::string(home) + "/data/mods/" + _mod + "/" + filename);
    }
    _paths.push_back(std::string(home) + "/data/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back(std::string(home) + "/data/" + filename);
    SDL_free(home);
  }


#elif defined(__VITA__)
  if (!_mod.empty())
  {
    _paths.push_back("ux0:/data/nxengine/data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back("ux0:/data/nxengine/data/mods/" + _mod + "/" + filename);
    _paths.push_back("app0:/data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back("app0:/data/mods/" + _mod + "/" + filename);
  }
  _paths.push_back("ux0:/data/nxengine/data/lang/" + std::string(settings->language) + "/" + filename);
  _paths.push_back("ux0:/data/nxengine/data/" + filename);
  _paths.push_back("app0:/data/lang/" + std::string(settings->language) + "/" + filename);
  _paths.push_back("app0:/data/" + filename);

#elif defined(__SWITCH__)
  if (!_mod.empty())
  {
    _paths.push_back("data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back("data/mods/" + _mod + "/" + filename);
    _paths.push_back("romfs:/data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back("romfs:/data/mods/" + _mod + "/" + filename);
  }

  _paths.push_back("data/lang/" + std::string(settings->language) + "/" + filename);
  _paths.push_back("data/" + filename);
  _paths.push_back("romfs:/data/lang/" + std::string(settings->language) + "/" + filename);
  _paths.push_back("romfs:/data/" + filename);

#endif

  if (!_mod.empty())
  {
    _paths.push_back("data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back("data/mods/" + _mod + "/" + filename);
  }
  _paths.push_back("data/lang/" + std::string(settings->language) + "/" + filename);
  _paths.push_back("data/" + filename);

  for (auto &_tryPath: _paths)
  {
    if (fileExists(_tryPath))
      return _tryPath;
  }

  return _paths.back();
}

std::string ResourceManager::getPrefPath(const std::string &filename)
{
  std::string _tryPath;

#if defined(__VITA__)
  sceIoMkdir("ux0:/data/nxengine/", 0700);
  _tryPath = std::string("ux0:/data/nxengine/") + std::string(filename);
#elif defined(__SWITCH__)
//  mkdir("nxengine/", 0700);
  _tryPath = std::string(filename);
#else
  char *prefpath      = SDL_GetPrefPath("nxengine", "nxengine-evo");
  _tryPath = std::string(prefpath) + std::string(filename);
  SDL_free(prefpath);
#endif

  return _tryPath;
}

std::string ResourceManager::getPathForDir(const std::string &dir)
{
  std::vector<std::string> _paths;

#if defined(__linux__)
  char *home = getenv("HOME");
  if (home != NULL)
  {
    if (!_mod.empty())
      _paths.push_back(std::string(home) + "/.local/share/nxengine/data/mods/" + _mod + "/" + dir);
    _paths.push_back(std::string(home) + "/.local/share/nxengine/data/" + dir);
  }

  if (!_mod.empty())
  {
    _paths.push_back("/usr/share/nxengine/data/mods/" + _mod + "/" + dir);
    _paths.push_back("/usr/local/share/nxengine/data/mods/" + _mod + "/" + dir);
  }

  _paths.push_back("/usr/share/nxengine/data/" + dir);
  _paths.push_back("/usr/local/share/nxengine/data/" + dir);

#elif defined(__APPLE__)
  char *home = SDL_GetPrefPath("nxengine", "nxengine-evo");
  if (home != NULL)
  {
    if (!_mod.empty())
      _paths.push_back(std::string(home) + "/data/mods/" + _mod + "/" + dir);
    _paths.push_back(std::string(home) + "/data/" + dir);
    SDL_free(home);
  }

#elif defined(__VITA__)
  if (!_mod.empty())
  {
    _paths.push_back("ux0:/data/nxengine/data/mods/" + _mod + "/" + dir);
    _paths.push_back("app0:/data/mods/" + _mod + "/" + dir);
  }
  _paths.push_back("ux0:/data/nxengine/data/" + dir);
  _paths.push_back("app0:/data/" + dir);

#elif defined(__SWITCH__)
  if (!_mod.empty())
  {
    _paths.push_back("data/mods/" + _mod + "/" + dir);
    _paths.push_back("romfs:/data/mods/" + _mod + "/" + dir);
  }
  _paths.push_back("data/" + dir);
  _paths.push_back("romfs:/data/" + dir);

#endif

  if (!_mod.empty())
    _paths.push_back("data/mods/" + _mod + "/" + dir);
  _paths.push_back("data/" + dir);

  for (auto &_tryPath: _paths)
  {
    if (fileExists(_tryPath))
      return _tryPath;
  }

  return _paths.back();
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
    std::cout << l << std::endl;
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
