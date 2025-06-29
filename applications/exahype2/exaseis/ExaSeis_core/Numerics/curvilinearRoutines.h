#pragma once

namespace Numerics{
  template <class Shortcuts, typename T>
  inline void getNormals(const T* Q, int direction, T& norm, T* n){
    using s = Shortcuts;
    T v_x = Q[s::metric_derivative + 0 + direction * 3];
    T v_y = Q[s::metric_derivative + 1 + direction * 3];
    T v_z = Q[s::metric_derivative + 2 + direction * 3];  
    
    norm = std::sqrt(v_x*v_x + v_y*v_y + v_z*v_z);
    n[0] = v_x/norm;
    n[1] = v_y/norm;
    n[2] = v_z/norm;	
  }
}
