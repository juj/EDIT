#pragma once

#include <assert.h>

template<typename T>
class vec
{
public:
  vec():ptr(0), sz(0), cap(0){}
  ~vec() { ::free(ptr); }
        T *data()       { return ptr; }
  const T *data() const { return ptr; }
        T *begin()       { return ptr; }
  const T *begin() const { return ptr; }
        T *end()        { return ptr+sz; }
  const T *end() const  { return ptr+sz; }
        T &back()       { assert(sz > 0); return ptr[sz-1]; }
  const T &back() const { assert(sz > 0); return ptr[sz-1]; }
        T &operator[](unsigned int i)       { assert(i >= 0 && i < sz); return ptr[i]; }
  const T &operator[](unsigned int i) const { assert(i >= 0 && i < sz); return ptr[i]; }
  unsigned int size() const { return sz; }
  T pop() { assert(sz > 0); return ptr[--sz]; }
  int index(const T &value) const { for(unsigned int i = 0; i < sz; ++i) if (ptr[i] == value) return i; return -1; }
  void clear() { sz = 0; }
  void push(const T &val)
  {
    ensure_space();
    ptr[sz++] = val;
  }
  void free() { ::free(ptr); ptr = 0; sz = cap = 0; }
private:
  void ensure_space() { if (sz >= cap) { cap = (cap<<1)+16; ptr = (T*)realloc(ptr, cap*sizeof(T)); } }
  T *ptr;
  unsigned int sz, cap;
};
