#include "CGHigherOrderPoisson.h"


tutorials::mghype::examples::CGHigherOrderPoisson::CGHigherOrderPoisson() {
}


tutorials::mghype::examples::CGHigherOrderPoisson::~CGHigherOrderPoisson() {
}



void tutorials::mghype::examples::CGHigherOrderPoisson::initCellDoF(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  tarch::la::Vector< UnknownsPerCellNode, double >&  value,
  tarch::la::Vector< UnknownsPerCellNode, double >&  rhs,
  tarch::la::Vector< Dimensions, int >&              node
) {
  value(0) = 0.0;
  rhs(0)   = Dimensions * tarch::la::PI * tarch::la::PI;
  for (int d=0; d<Dimensions; d++) {
    rhs(0) *= std::sin( tarch::la::PI * x(d) );
  }
}


