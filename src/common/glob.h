#ifndef _GLOB_H_
#define _GLOB_H_

#include <cassert>
#include <string>

#ifdef _WIN32
  #include <windows.h>
#else
  #include <dirent.h>
#endif

class Glob {
 public:
  Glob(const std::string &pattern);
  ~Glob();

  bool Next();

 private:
  Glob(const Glob &);
  void operator=(const Glob &);
  std::string _base;

 public:

#ifdef _WIN32
  std::string GetFileName() const {
    assert(ok_);
    return _base+"/"+std::string(find_data_.cFileName);
  }

  operator bool() const {
    return ok_;
  }

 private:
  bool ok_;
  HANDLE find_handle_;
  WIN32_FIND_DATAA find_data_;
#else
  std::string GetFileName() const {
    assert(dir_entry_ != 0);
    return _base+"/"+std::string(dir_entry_->d_name);
  }

  operator bool() const {
    return dir_entry_ != 0;
  }

 private:
  std::string pattern_;
  DIR *dir_;
  struct dirent *dir_entry_;
#endif
};

#endif