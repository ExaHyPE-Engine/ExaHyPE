// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Matrix.h"

namespace applications::exahype2::exaskred::projection {
  struct AlbersEqualArea;
} // namespace applications::exahype2::exaskred::projection

/**
 * Maps geographic coordinates on an ellipsoid to the metric coordinates of the mesh and back with
 * the Albers equal-area conic projection.
 */
struct applications::exahype2::exaskred::projection::AlbersEqualArea final {
  double referenceLongitude{0.0};
  double referenceLatitude{0.0};
  double firstStandardParallel{0.0};
  double secondStandardParallel{0.0};
  double semiMajorAxis{0.0};
  double eccentricity{0.0};

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD std::pair<double, double> forward(
    double longitudeInDegrees,
    double latitudeInDegrees
  ) const;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD std::pair<double, double> inverse(double x, double y) const;

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Matrix<2, 2, double> inverseJacobian(
    double x,
    double y
  ) const;

private:
  static constexpr double DegreesToRadians   = std::numbers::pi_v<double> / 180.0;
  static constexpr double RadiansToDegrees   = 180.0 / std::numbers::pi_v<double>;
  static constexpr double LatitudeTolerance  = 1e-12;
  static constexpr int    LatitudeIterations = 10;

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getParallelRadius(
    double latitude,
    double eccentricitySquared
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getAuthalicMeasure(
    double latitude,
    double eccentricity
  );

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double getAuthalicMeasureDerivative(
    double latitude,
    double eccentricity
  );
};

#include "AlbersEqualArea.cpph"
