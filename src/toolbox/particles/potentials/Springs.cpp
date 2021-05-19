#include "Springs.h"


tarch::la::Vector<Dimensions,double> toolbox::particles::potentials::springPotential(
  const tarch::la::Vector<Dimensions,double>& p1,
  const tarch::la::Vector<Dimensions,double>& p2,
  double h
) {
  tarch::la::Vector<Dimensions,double> deltaX = p2-p1;
  double distance           = tarch::la::norm2( deltaX );

  double force = 0.0;
  force =
    std::max(
      -1.0,
      std::pow(h/distance,6.0) - std::pow(h/distance,12.0)
    );
  return force * 1.0/distance * deltaX;
}
