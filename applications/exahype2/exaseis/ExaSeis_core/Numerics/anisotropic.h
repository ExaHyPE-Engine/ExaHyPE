#define ANISOTROPIC
namespace Numerics{
template <class Shortcuts>
  inline void get_stiffness_tensor(const double*Q, double& c11,double& c22,double& c33,double& c44,double& c55,double& c66,double& c12,double& c13,double& c23){
    Shortcuts s;
    c11 = Q[s.c + 0];
    c22 = Q[s.c + 1];
    c33 = Q[s.c + 2];
    c44 = Q[s.c + 3];
    c55 = Q[s.c + 4];
    c66 = Q[s.c + 5];
    
    c12 = Q[s.c + 6];
    c13 = Q[s.c + 7];
    c23 = Q[s.c + 8];
  }
}
#include "riemannsolver_routines.h"
#include "riemannsolver_anisotropic.h"


