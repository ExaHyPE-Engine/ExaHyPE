#include "Poisson.h"


benchmarks::multigrid::petsc::poisson::Poisson::Poisson() {
  // previously, this code would segfault without reserving
  // some places for members. 16 is hardcoded.
  // _valueAndRhs.reserve(16);
}


benchmarks::multigrid::petsc::poisson::Poisson::~Poisson() {
  // @todo add your stuff here
}


void benchmarks::multigrid::petsc::poisson::Poisson::init(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  
  double&                                       value,
  double&                                       rhs
  
) {
  // add this values to our vector. we pass these on to Petsc later
  // TODO - make sure these are in order. push_back is not what we want to 
  // do here.
  _valueAndRhs.push_back( std::make_pair( value, rhs ) );
}

std::vector< std::pair<double, double> > benchmarks::multigrid::petsc::poisson::Poisson::getValuesAndRhs() const{
  return _valueAndRhs;
}
