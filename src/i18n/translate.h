#ifndef _TRANSLATE_H_
#define _TRANSLATE_H_

#include <map>
#include <string>

class I18N
{
public:
  I18N();
  ~I18N();
  bool load();
  const std::string &translate(const std::string &key);
  const bool isRTL();

private:
  std::map<std::string, std::string> _strings;
  bool _rtl = false;
};

#endif
