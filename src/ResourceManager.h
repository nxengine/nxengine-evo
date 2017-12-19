#ifndef _RESOURCEMANAGER_H
#define _RESOURCEMANAGER_H

#include <string>
#include <map>
#include <memory>
#include <unordered_map>
#include <vector>

#include "Singleton.h"


class ResourceManager
{

public:
    static ResourceManager* getInstance();

    void shutdown();
    std::string getLocalizedPath(const std::string& filename);
    std::string getPathForDir(const std::string& dir);
    std::vector<std::string>& languages();
protected:
    friend class Singleton<ResourceManager>;

    ResourceManager();
    ~ResourceManager();
    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;
    std::string _language;
    void findLanguages();
    std::vector<std::string> _languages;

};

#endif
