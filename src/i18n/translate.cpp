#include "translate.h"
#include "../common/json.hpp"
#include "../common/misc.h"
#include "../ResourceManager.h"
#include <fstream>

I18N::I18N() : _strings() {}

I18N::~I18N() {}

const bool I18N::load()
{
    std::string path=ResourceManager::getInstance()->getLocalizedPath("system.json");
    std::ifstream fl;
    _strings.clear();
    fl.open(widen(path), std::ifstream::in | std::ifstream::binary);
    if (fl.is_open()) {
        nlohmann::json langfile = nlohmann::json::parse(fl);

        for (auto it = langfile.begin(); it != langfile.end(); ++it)
        {
            _strings[ it.key() ] = it.value();
        }
        return true;
    }
    return false;
}



const std::string& I18N::translate(const std::string& key)
{
    if (_strings.find(key) != _strings.end())
    {
        return _strings.at(key);
    }
    else
    {
        return key;
    }
}

