#ifndef _RESOURCEMANAGER_H
#define _RESOURCEMANAGER_H

#include "Singleton.h"

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

typedef struct {
  std::string dir;
  std::string name;
  bool skip_intro;
} Mod;

class ResourceManager
{

public:
  static ResourceManager *getInstance();
  static bool fileExists(const std::string &filename);

  /**
   * Returns the path to the directory where the installed game resources
   * may be found terminated by a path separator
   *
   * This function never returns a `nullptr`.
   * Returned pointer must be freed using `SDL_free`.
   */
  static char* getInstallResourceBasePath();

  /**
   * Returns the path to the directory where user-defined game resources
   * may be found terminated by a path separator
   *
   * This function may return `nullptr` if no such directory exists.
   * Returned pointer must be freed using `SDL_free`.
   */
  static char* getUserResourceBasePath();

  void shutdown();
  std::string getLocalizedPath(const std::string &filename);
  std::string getPrefPath(const std::string &filename);
  std::string getPathForDir(const std::string &dir);
  std::vector<std::string> &languages();
  Mod& mod(std::string& name);
  void setMod(std::string name);
  Mod& mod();
  bool isMod();
  std::map<std::string, Mod> &mods();
protected:
  friend class Singleton<ResourceManager>;

  ResourceManager();
  ~ResourceManager();
  ResourceManager(const ResourceManager &) = delete;
  ResourceManager &operator=(const ResourceManager &) = delete;
  std::string _language;
  void findLanguages();
  std::vector<std::string> _languages;
  std::string _mod = "";
  void findMods();
  std::map<std::string,Mod> _mods;
};

#endif
