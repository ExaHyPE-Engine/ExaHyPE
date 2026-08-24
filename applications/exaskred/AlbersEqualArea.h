// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Matrix.h"

namespace applications::exahype2::exaskred::projection {
  struct AlbersEqualArea;
} // namespace applications::exahype2::exaskred::projection

/**
 * Albers equal-area conic projection on an ellipsoid.
 *
 * A scenario whose forcing or bathymetry is given in geographic coordinates needs
 * to move between those and the metric coordinates the mesh uses. The projection
 * carries its own reference point, standard parallels and ellipsoid, so several
 * scenarios can use different ones at the same time.
 */
struct applications::exahype2::exaskred::projection::AlbersEqualArea final {
  double referenceLongitude{0.0};
  double referenceLatitude{0.0};
  double firstStandardParallel{0.0};
  double secondStandardParallel{0.0};
  double semiMajorAxis{0.0};
  double eccentricity{0.0};

  [[nodiscard]] std::pair<double, double> forward(double longitudeInDegrees, double latitudeInDegrees) const;

  [[nodiscard]] std::pair<double, double> inverse(double x, double y) const;

  /**
   * Derivatives of longitude and latitude with respect to the metric coordinates.
   *
   * Row zero holds the longitude derivatives and row one the latitude ones, both
   * in degrees per metre. A field defined in geographic coordinates gets its
   * metric gradient by multiplying its geographic gradient with this matrix.
   */
  [[nodiscard]] tarch::la::Matrix<2, 2, double> inverseJacobian(double x, double y) const;
};
