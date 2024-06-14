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
   * Returns the base path to binary
   */
  std::string getBasePath();

  /**
   * Returns the path to the directory where user-data resides
   */
  std::string getUserPrefPath();

  void shutdown();
  std::string getPath(const std::string &filename, bool localized = true);

  std::string getPrefPath(const std::string &filename);

  std::string getPathForDir(const std::string &dir);

  std::vector<std::string> &languages();
  Mod& mod(std::string& name);
  void setMod(std::string name);
  Mod& mod();
  bool isMod();
  std::map<std::string, Mod> &mods();

  SDL_Texture* loadTexture(SDL_Renderer* renderer, const std::string& path);


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
