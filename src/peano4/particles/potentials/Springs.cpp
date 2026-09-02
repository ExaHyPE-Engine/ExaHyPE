// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Springs.h"

tarch::la::Vector<DIMENSIONS, double> peano4::particles::potentials::springPotential(
  const tarch::la::Vector<DIMENSIONS, double>& p1,
  const tarch::la::Vector<DIMENSIONS, double>& p2,
  double                                       h
) {
  const tarch::la::Vector<DIMENSIONS, double> deltaX   = p2 - p1;
  const double                                distance = tarch::la::norm2(deltaX);

  const double force = std::max(-1.0, std::pow(h / distance, 6.0) - std::pow(h / distance, 12.0));
  return force * 1.0 / distance * deltaX;
}
