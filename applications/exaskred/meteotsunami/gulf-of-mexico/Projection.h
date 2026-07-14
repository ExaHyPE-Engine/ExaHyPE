// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::exaskred::meteotsunami::projection::aea {
  std::pair<double, double> forwardEllipsoid(double lonDeg, double latDeg);
  std::pair<double, double> inverseEllipsoid(double x, double y);
} // namespace applications::exahype2::exaskred::meteotsunami::projection::aea
