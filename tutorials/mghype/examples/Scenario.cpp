#include "Scenario.h"


double tests::multigrid::matrixfree::poisson::getPointWiseRhs(
  const tarch::la::Vector<Dimensions, double>&  x
) {
/*
  double tempRhs = 2 * tarch::la::PI * tarch::la::PI;
  for (int i = 0; i < Dimensions; i++){
    tempRhs *= std::sin( tarch::la::PI * x[i] );
  }
  return tempRhs;
*/
  static constexpr double fourDPiSquared = 4 * Dimensions * tarch::la::PI * tarch::la::PI;

  double sinSquared = 1;
  for (int i=0; i<Dimensions; i++) {
    sinSquared *= std::sin( 2 * tarch::la::PI * x(i) );
  }

  return fourDPiSquared * sinSquared;
}
