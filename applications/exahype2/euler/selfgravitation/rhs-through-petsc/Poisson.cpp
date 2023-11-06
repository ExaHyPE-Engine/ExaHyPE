#include "Poisson.h"


applications::exahype2::euler::selfgravitation::Poisson::Poisson() {
  // @todo add your stuff here
}


applications::exahype2::euler::selfgravitation::Poisson::~Poisson() {
  // @todo add your stuff here
}


void applications::exahype2::euler::selfgravitation::Poisson::vertexInit(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< 1, double >&  value,
  tarch::la::Vector< 1, double >&  rhs
) {
  double tempRhs = Dimensions * tarch::la::PI * tarch::la::PI;
  for (int i = 0; i < Dimensions; i++){

    //we add scaling by cell width here
    tempRhs *= std::sin( tarch::la::PI * x[i] ) * h[i];
  }
  // put this into the rhs vector
  for (int i=0; i<rhs.size(); i++)
  {
    rhs[i] = tempRhs;
  }

  // similarly, set all of the entries of "value" to 0
  for (int i=0; i<value.size(); i++)
  {
    value[i] = 0.0;
  }
}





