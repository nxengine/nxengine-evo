#include <fstream>
#include <string>
#include <glob.h>
#include <iostream>
#include <stdexcept>

#if defined(__unix__) || defined(__APPLE__)
    #include <sys/stat.h>
#elif defined(_WIN32) || defined(WIN32)
    #include <windows.h>
#endif


#include "ResourceManager.h"
#include "settings.h"

ResourceManager::ResourceManager()
{
    findLanguages();
}

ResourceManager::~ResourceManager()
{
}

ResourceManager* ResourceManager::getInstance()
{
    return Singleton<ResourceManager>::get();
}


void ResourceManager::shutdown()
{
}

std::string ResourceManager::getLocalizedPath(const std::string& filename)
{
    std::string _tryPath;
    std::ifstream ifs;

#if defined(__linux__)
    char *home = getenv("HOME");
    if (home != NULL)
    {
        _tryPath=std::string(home)+"/.local/share/nxengine/data/lang/"+std::string(settings->language)+"/"+filename;
        ifs.open(_tryPath);
        if (ifs.is_open())
        {
            ifs.close();
            return _tryPath;
        }

        _tryPath=std::string(home)+"/.local/share/nxengine/data/"+filename;
        ifs.open(_tryPath);
        if (ifs.is_open())
        {
            ifs.close();
            return _tryPath;
        }
    }

    _tryPath="/usr/share/nxengine/data/lang/"+std::string(settings->language)+"/"+filename;
    ifs.open(_tryPath);
    if (ifs.is_open())
    {
        ifs.close();
        return _tryPath;
    }

    _tryPath="/usr/share/nxengine/data/"+filename;
    ifs.open(_tryPath);
    if (ifs.is_open())
    {
        ifs.close();
        return _tryPath;
    }

    _tryPath="/usr/local/share/nxengine/data/lang/"+std::string(settings->language)+"/"+filename;
    ifs.open(_tryPath);
    if (ifs.is_open())
    {
        ifs.close();
        return _tryPath;
    }

    _tryPath="/usr/local/share/nxengine/data/"+filename;
    ifs.open(_tryPath);
    if (ifs.is_open())
    {
        ifs.close();
        return _tryPath;
    }
#elif defined (__APPLE__)
    char *home = SDL_GetPrefPath("nxengine","nxengine-evo");
    if (home != NULL)
    {
        _tryPath=std::string(home)+"/data/lang/"+std::string(settings->language)+"/"+filename;
        ifs.open(_tryPath);
        if (ifs.is_open())
        {
            ifs.close();
            SDL_free(home);
            return _tryPath;
        }

        _tryPath=std::string(home)+"/data/"+filename;
        ifs.open(_tryPath);
        if (ifs.is_open())
        {
            ifs.close();
            SDL_free(home);
            return _tryPath;
        }
        SDL_free(home);
    }

#endif

    _tryPath="data/lang/"+std::string(settings->language)+"/"+filename;
    ifs.open(_tryPath);
    if (ifs.is_open())
    {
        ifs.close();
        return _tryPath;
    }

    _tryPath="data/"+filename;
    ifs.open(_tryPath);
    if (ifs.is_open())
    {
        ifs.close();
        return _tryPath;
    }
    ifs.close();

    throw std::runtime_error("Can't open file "+_tryPath);

}

inline bool fileExists(const std::string& file)
{
#if defined(__unix__) || defined(__APPLE__) // Linux, OS X, BSD
    struct stat st;

    if (stat(file.c_str(), &st) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
#elif defined(_WIN32) || defined(WIN32) // Windows
    DWORD attrs = GetFileAttributes(file.c_str());

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


std::string ResourceManager::getPathForDir(const std::string& dir)
{
    std::string _tryPath;
    std::ifstream ifs;

#if defined(__linux__)
    char *home = getenv("HOME");
    if (home != NULL)
    {
        _tryPath=std::string(home)+"/.local/share/nxengine/data/"+dir;
        if (fileExists(_tryPath))
        {
            return _tryPath;
        }
    }

    _tryPath="/usr/share/nxengine/data/"+dir;
    if (fileExists(_tryPath))
    {
        return _tryPath;
    }

    _tryPath="/usr/local/share/nxengine/data/"+dir;
    if (fileExists(_tryPath))
    {
        return _tryPath;
    }
#elif defined (__APPLE__)
    char *home = SDL_GetPrefPath("nxengine","nxengine-evo");
    if (home != NULL)
    {
        _tryPath=std::string(home)+"/data/"+dir;
        if (fileExists(_tryPath))
        {
            SDL_free(home);
            return _tryPath;
        }
        SDL_free(home);
    }

#endif

    _tryPath="data/"+dir;
    if (fileExists(_tryPath))
    {
        return _tryPath;
    }

    throw std::runtime_error("Can't find directory "+dir);

}

inline std::vector<std::string> glob(const std::string& pat){
    glob_t glob_result;
    glob(pat.c_str(),GLOB_TILDE,NULL,&glob_result);
    std::vector<std::string> ret;
    for(unsigned int i=0;i<glob_result.gl_pathc;++i){
        ret.push_back(std::string(glob_result.gl_pathv[i]));
    }
    globfree(&glob_result);
    return ret;
}

void ResourceManager::findLanguages()
{
    std::vector<std::string> langs=glob(getPathForDir("lang/")+"*");
    _languages.push_back("english");
    for (auto &l: langs)
    {
        std::cout << l << std::endl;
        std::ifstream ifs(l+"/system.json");
        if (ifs.is_open())
        {
            ifs.close();
            _languages.push_back(l.substr(l.find_last_of('/')+1));
        }
    }
}

std::vector<std::string>& ResourceManager::languages()
{
    return _languages;
}