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

char* ResourceManager::getInstallResourceBasePath()
{
  char* resPath;

  #if defined(__SWITCH__)
    char resPathValue[] = "romfs:/";

    resPath = SDL_malloc(sizeof(resPathValue));
    assert(resPath != nullptr);

    SDL_memcpy(resPath, path, sizeof(resPathValue));
  #else
    resPath = SDL_GetBasePath();
    assert(resPath != nullptr);
  #endif

  return resPath;
}

char* ResourceManager::getUserResourceBasePath()
{
  #if defined(__SWITCH__)
    // Switch doesn't have any user data directory, but its SDL implementation
    // is lacking `SDL_GetPrefPath` to tell us that
    return nullptr;
  #elif defined(HAVE_UNIX_LIKE)
    //XXX: Use different parameters on Linux/BSD than elsewhere to match the
    //     previously generated generated paths (which incorrectly hard-coded
    //     ~/.local/share/nxengine) in most cases
    return SDL_GetPrefPath(NULL, "nxengine");
  #else
    return SDL_GetPrefPath("nxengine", "nxengine-evo");
  #endif
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

  char* userResBasePtr = getUserResourceBasePath();
  if (userResBasePtr != NULL)
  {
    std::string userResBase(userResBasePtr);
    SDL_free(userResBasePtr);

    if (!_mod.empty())
    {
      _paths.push_back(userResBase + "data/mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
      _paths.push_back(userResBase + "data/mods/" + _mod + "/" + filename);
    }
    _paths.push_back(userResBase + "data/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back(userResBase + "data/" + filename);
  }

  #if defined(DATADIR)
    std::string _data(DATADIR "/");
  #else
    char* installResBasePtr = getInstallResourceBasePath();
    std::string _data(installResBasePtr);
    SDL_free(installResBasePtr);
    #if defined(HAVE_UNIX_LIKE)
      _data += "../share/nxengine/data/";
    #else
      _data += "data/";
    #endif
  #endif

  if (!_mod.empty())
  {
    _paths.push_back(_data + "mods/" + _mod + "/lang/" + std::string(settings->language) + "/" + filename);
    _paths.push_back(_data + "mods/" + _mod + "/" + filename);
  }

  _paths.push_back(_data + "lang/" + std::string(settings->language) + "/" + filename);
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

#if defined(DATADIR)
  std::string _data (DATADIR);
#endif

  if (home != NULL)
  {
    if (!_mod.empty())
      _paths.push_back(std::string(home) + "/.local/share/nxengine/data/mods/" + _mod + "/" + dir);
    _paths.push_back(std::string(home) + "/.local/share/nxengine/data/" + dir);
  }

  if (!_mod.empty())
  {
#if defined(DATADIR)
    _paths.push_back(_data + "mods/" + _mod + "/" + dir);
#else
    _paths.push_back("/usr/local/share/nxengine/data/mods/" + _mod + "/" + dir);
    _paths.push_back("/usr/share/nxengine/data/mods/" + _mod + "/" + dir);
    _paths.push_back("../share/nxengine/data/mods/" + _mod + "/" + dir);
#endif
  }

#if defined(DATADIR)
  _paths.push_back(_data + dir);
#else
  _paths.push_back("/usr/local/share/nxengine/data/" + dir);
  _paths.push_back("/usr/share/nxengine/data/" + dir);
  _paths.push_back("../share/nxengine/data/" + dir);
#endif

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

#if !defined(__VITA__) and !defined(__SWITCH__)
  if (!_mod.empty())
    _paths.push_back("data/mods/" + _mod + "/" + dir);
  _paths.push_back("data/" + dir);
#endif

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
