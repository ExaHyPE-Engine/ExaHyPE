#pragma once

namespace Numerics{
  template <class Shortcuts, typename T>
  inline void getNormals(const T* Q, int direction, T& norm, T* n){
    norm = 1;
    n[0] = 0;
    n[1] = 0;
    n[2] = 0;	
    n[direction] = 1;
  }
}
