#ifndef _BASE_SINGLETON_H
#define _BASE_SINGLETON_H

#include <new>

template <typename Type> class Singleton
{
private:
  // Classes using the Singleton<T> pattern should declare a getInstance()
  // method and call Singleton::get() from within that.
  friend Type *Type::getInstance();

  static Type *get()
  {
    if (!_instance)
    {
      _instance = new Type();
    }

    return _instance;
  }

  static Type *_instance;
};

template <typename Type> Type *Singleton<Type>::_instance = nullptr;

#endif
