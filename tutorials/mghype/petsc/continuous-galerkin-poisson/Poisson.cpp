#include "Poisson.h"
constexpr double pi = 3.14159265358979323846264338328;

double initRhs(
  const tarch::la::Vector<Dimensions, double>&  x
){
  double rhs = Dimensions * pi * pi;
  for (int i = 0; i < Dimensions; i++){
    rhs *= std::sin( pi * x[i] );
  }
  return rhs;  
}


benchmarks::multigrid::petsc::poisson::Poisson::Poisson() {
  // @todo add your stuff here
}


benchmarks::multigrid::petsc::poisson::Poisson::~Poisson() {
  // @todo add your stuff here
}


void benchmarks::multigrid::petsc::poisson::Poisson::initVertex(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  
  double&                                       value,
  double&                                       rhs
  
) {
  value = 0.0;
  rhs = initRhs(x);
}