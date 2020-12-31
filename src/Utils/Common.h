#ifndef _COMMON_H
#define _COMMON_H

template<class T>
constexpr const T& clamp(const T& val, const T& min, const T& max)
{
  return (val < min) ? min : (max < val) ? max : val;
}

#endif