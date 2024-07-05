namespace Numerics{
  template <class Shortcuts>
  inline void get_normals(const double* Q, int direction, double& norm, double* n){
    norm = 1;
    n[0] = 0;
    n[1] = 0;
    n[2] = 0;	
    n[direction] = 1;
  }
}
