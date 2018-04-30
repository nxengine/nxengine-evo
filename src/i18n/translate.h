#ifndef _TRANSLATE_H_
#define _TRANSLATE_H_

#include <string>
#include <map>

class I18N {
public:
    I18N();
    ~I18N();
    bool load();
    const std::string& translate(const std::string& key);
private:
    std::map<std::string, std::string> _strings;
};



#endif
