// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AlbersEqualArea.h"

namespace {
  constexpr double DegreesToRadians = std::numbers::pi_v<double> / 180.0;
  constexpr double RadiansToDegrees = 180.0 / std::numbers::pi_v<double>;

  // Newton needs a handful of passes to reach the tolerance from the spherical
  // starting guess, and the tolerance is far below what a bathymetry raster resolves.
  constexpr double LatitudeTolerance  = 1e-12;
  constexpr int    LatitudeIterations = 10;

  double parallelRadius(const double latitude, const double eccentricitySquared) {
    const double sinLatitude = std::sin(latitude);
    return std::cos(latitude) / std::sqrt(1.0 - eccentricitySquared * sinLatitude * sinLatitude);
  }

  double authalicMeasure(const double latitude, const double eccentricity) {
    const double eccentricitySquared = eccentricity * eccentricity;
    const double sinLatitude         = std::sin(latitude);
    const double sinLatitudeSquared  = sinLatitude * sinLatitude;
    return (1.0 - eccentricitySquared)
      * (sinLatitude / (1.0 - eccentricitySquared * sinLatitudeSquared)
         - std::log((1.0 - eccentricity * sinLatitude) / (1.0 + eccentricity * sinLatitude)) / (2.0 * eccentricity));
  }

  double authalicMeasureDerivative(const double latitude, const double eccentricity) {
    const double eccentricitySquared = eccentricity * eccentricity;
    const double sinLatitude         = std::sin(latitude);
    const double denominator         = 1.0 - eccentricitySquared * sinLatitude * sinLatitude;
    return 2.0 * (1.0 - eccentricitySquared) * std::cos(latitude) / (denominator * denominator);
  }
} // namespace

std::pair<double, double> applications::exahype2::exaskred::projection::AlbersEqualArea::forward(
  const double longitudeInDegrees,
  const double latitudeInDegrees
) const {
  const double eccentricitySquared = eccentricity * eccentricity;
  const double firstParallel       = firstStandardParallel * DegreesToRadians;
  const double secondParallel      = secondStandardParallel * DegreesToRadians;

  const double firstRadius   = parallelRadius(firstParallel, eccentricitySquared);
  const double secondRadius  = parallelRadius(secondParallel, eccentricitySquared);
  const double firstMeasure  = authalicMeasure(firstParallel, eccentricity);
  const double secondMeasure = authalicMeasure(secondParallel, eccentricity);
  const double originMeasure = authalicMeasure(referenceLatitude * DegreesToRadians, eccentricity);
  const double pointMeasure  = authalicMeasure(latitudeInDegrees * DegreesToRadians, eccentricity);

  const double cone     = (firstRadius * firstRadius - secondRadius * secondRadius) / (secondMeasure - firstMeasure);
  const double constant = firstRadius * firstRadius + cone * firstMeasure;
  const double originDistance = semiMajorAxis * std::sqrt(constant - cone * originMeasure) / cone;
  const double pointDistance  = semiMajorAxis * std::sqrt(constant - cone * pointMeasure) / cone;
  const double coneAngle      = cone * (longitudeInDegrees * DegreesToRadians - referenceLongitude * DegreesToRadians);

  return {pointDistance * std::sin(coneAngle), originDistance - pointDistance * std::cos(coneAngle)};
}

std::pair<double, double> applications::exahype2::exaskred::projection::AlbersEqualArea::inverse(
  const double x,
  const double y
) const {
  const double eccentricitySquared         = eccentricity * eccentricity;
  const double oneMinusEccentricitySquared = 1.0 - eccentricitySquared;
  const double firstParallel               = firstStandardParallel * DegreesToRadians;
  const double secondParallel              = secondStandardParallel * DegreesToRadians;

  const double firstRadius   = parallelRadius(firstParallel, eccentricitySquared);
  const double secondRadius  = parallelRadius(secondParallel, eccentricitySquared);
  const double firstMeasure  = authalicMeasure(firstParallel, eccentricity);
  const double secondMeasure = authalicMeasure(secondParallel, eccentricity);
  const double originMeasure = authalicMeasure(referenceLatitude * DegreesToRadians, eccentricity);

  const double cone     = (firstRadius * firstRadius - secondRadius * secondRadius) / (secondMeasure - firstMeasure);
  const double constant = firstRadius * firstRadius + cone * firstMeasure;
  const double originDistance = semiMajorAxis * std::sqrt(constant - cone * originMeasure) / cone;
  const double pointDistance  = std::hypot(x, originDistance - y);
  const double pointMeasure =
    (constant - pointDistance * pointDistance * cone * cone / (semiMajorAxis * semiMajorAxis)) / cone;

  double latitude = std::asin(pointMeasure / 2.0);
  for (int iteration = 0; iteration < LatitudeIterations; iteration++) {
    const double sinLatitude        = std::sin(latitude);
    const double sinLatitudeSquared = sinLatitude * sinLatitude;
    const double denominator        = 1.0 - eccentricitySquared * sinLatitudeSquared;
    const double increment          = denominator * denominator
      * (pointMeasure / oneMinusEccentricitySquared - sinLatitude / denominator
         + std::log((1.0 - eccentricity * sinLatitude) / (1.0 + eccentricity * sinLatitude)) / (2.0 * eccentricity))
      / (2.0 * std::cos(latitude));
    latitude += increment;
    if (std::abs(increment) < LatitudeTolerance) {
      break;
    }
  }

  const double coneAngle = std::atan2(x, originDistance - y);
  return {(referenceLongitude * DegreesToRadians + coneAngle / cone) * RadiansToDegrees, latitude * RadiansToDegrees};
}

/**
 * The forward map is closed form, so its Jacobian is differentiated directly and
 * inverted, which avoids differentiating the iteration the inverse latitude uses.
 * With the cone angle theta and the distance from the cone apex rho, the forward
 * derivatives are rho*n*cos(theta) and rho*n*sin(theta) along longitude, and
 * drho/dlatitude times sin(theta) and -cos(theta) along latitude, whose determinant
 * is -rho*drho*n.
 */
tarch::la::Matrix<2, 2, double> applications::exahype2::exaskred::projection::AlbersEqualArea::inverseJacobian(
  const double x,
  const double y
) const {
  const double eccentricitySquared = eccentricity * eccentricity;
  const double firstParallel       = firstStandardParallel * DegreesToRadians;
  const double secondParallel      = secondStandardParallel * DegreesToRadians;

  const double firstRadius   = parallelRadius(firstParallel, eccentricitySquared);
  const double secondRadius  = parallelRadius(secondParallel, eccentricitySquared);
  const double firstMeasure  = authalicMeasure(firstParallel, eccentricity);
  const double secondMeasure = authalicMeasure(secondParallel, eccentricity);
  const double originMeasure = authalicMeasure(referenceLatitude * DegreesToRadians, eccentricity);

  const double cone     = (firstRadius * firstRadius - secondRadius * secondRadius) / (secondMeasure - firstMeasure);
  const double constant = firstRadius * firstRadius + cone * firstMeasure;
  const double originDistance = semiMajorAxis * std::sqrt(constant - cone * originMeasure) / cone;

  const double pointDistance = std::hypot(x, originDistance - y);
  const double coneAngle     = std::atan2(x, originDistance - y);
  const double latitude      = inverse(x, y).second * DegreesToRadians;
  const double distanceDerivative =
    -semiMajorAxis * semiMajorAxis * authalicMeasureDerivative(latitude, eccentricity) / (2.0 * pointDistance * cone);

  tarch::la::Matrix<2, 2, double> jacobian;
  jacobian(0, 0) = RadiansToDegrees * std::cos(coneAngle) / (pointDistance * cone);
  jacobian(0, 1) = RadiansToDegrees * std::sin(coneAngle) / (pointDistance * cone);
  jacobian(1, 0) = RadiansToDegrees * std::sin(coneAngle) / distanceDerivative;
  jacobian(1, 1) = -RadiansToDegrees * std::cos(coneAngle) / distanceDerivative;
  return jacobian;
}
