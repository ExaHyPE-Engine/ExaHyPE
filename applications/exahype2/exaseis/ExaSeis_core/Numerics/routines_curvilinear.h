namespace Numerics{
  template <class Shortcuts>
  inline void get_normals(const double* Q, int direction, double& norm, double* n){
    Shortcuts s;
    double v_x = Q[s.metric_derivative + 0 + direction * 3];
    double v_y = Q[s.metric_derivative + 1 + direction * 3];
    double v_z = Q[s.metric_derivative + 2 + direction * 3];  
    
    norm = std::sqrt(v_x*v_x + v_y*v_y + v_z*v_z);
    n[0] = v_x/norm;
    n[1] = v_y/norm;
    n[2] = v_z/norm;	
  }
}
