// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleInterpolation.h"

int exahype2::fv::particles::interpolation::globalVoxelsPerAxis       = 0;
int exahype2::fv::particles::interpolation::globalUnknownsPerVoxel    = 0;
int exahype2::fv::particles::interpolation::globalInterpolationMethod = 0;

void exahype2::fv::particles::interpolation::setGlobalInterpolationParameters(
  const int voxelsPerAxis,
  const int unknownsPerVoxel,
  const int interpolationMethod
) {
  globalVoxelsPerAxis       = voxelsPerAxis;
  globalUnknownsPerVoxel    = unknownsPerVoxel;
  globalInterpolationMethod = interpolationMethod;
}

tarch::la::Vector<DIMENSIONS, int> exahype2::fv::particles::interpolation::mapParticleOntoVoxel(
  const peano4::dataview::CellMarker&          marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<DIMENSIONS, double>& particleX
) {
  tarch::la::Vector<DIMENSIONS, int> result;
  assertion(voxelsPerAxis > 0, voxelsPerAxis);

  const double volumeWidth = marker.getH()(0) / voxelsPerAxis;

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double positionWithinPatch = particleX(axis) - marker.getOffset()(axis);
    result(axis)                     = static_cast<int>(std::floor(positionWithinPatch / volumeWidth));
    result(axis)                     = std::max(0, std::min(result(axis), voxelsPerAxis - 1));
  }

  return result;
}

tarch::la::Vector<DIMENSIONS, int> exahype2::fv::particles::interpolation::mapBiasedParticleOntoVoxel(
  const peano4::dataview::CellMarker&          marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<DIMENSIONS, double>& particleX
) {
  tarch::la::Vector<DIMENSIONS, int> result;
  assertion(voxelsPerAxis >= 2, marker, voxelsPerAxis, particleX);

  const double volumeWidth = marker.getH()(0) / voxelsPerAxis;

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double biasedPositionWithinPatch = particleX(axis) - marker.getOffset()(axis) - volumeWidth / 2.0;
    result(axis)                           = static_cast<int>(std::floor(biasedPositionWithinPatch / volumeWidth));
    result(axis)                           = std::max(0, std::min(result(axis), voxelsPerAxis - 2));
  }

  return result;
}

double exahype2::fv::particles::interpolation::projectValueOntoParticlePiecewiseConstant(
  const peano4::dataview::CellMarker&          marker,
  int                                          voxelsPerAxis,
  int                                          unknownsPerVoxel,
  const double* NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  int                                          unknown
) {
  const tarch::la::Vector<DIMENSIONS, int> voxel =
    exahype2::fv::particles::interpolation::mapParticleOntoVoxel(marker, voxelsPerAxis, particleX);
  const int voxelIndex = peano4::dLinearised(voxel, voxelsPerAxis);

  return Q[voxelIndex * unknownsPerVoxel + unknown];
}

double exahype2::fv::particles::interpolation::projectValueOntoParticlePiecewiseLinear(
  const peano4::dataview::CellMarker&          marker,
  int                                          voxelsPerAxis,
  int                                          unknownsPerVoxel,
  const double* NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  int                                          unknown
) {
  const double                                volumeWidth = marker.getH()(0) / voxelsPerAxis;
  const tarch::la::Vector<DIMENSIONS, double> offset      = marker.getOffset();

  // The upper voxel of every axis pair has to lie within the patch.
  tarch::la::Vector<DIMENSIONS, int> lowerVoxel;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double relativePosition = (particleX(axis) - offset(axis)) / volumeWidth;
    lowerVoxel(axis)              = static_cast<int>(std::floor(relativePosition));
    lowerVoxel(axis)              = std::max(0, std::min(lowerVoxel(axis), voxelsPerAxis - 2));
  }

  double result     = 0.0;
  double sumWeights = 0.0;

  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& corner, int linearisedCorner) {
    const tarch::la::Vector<DIMENSIONS, int> currentVoxel = lowerVoxel + corner;
    bool                                     inside       = true;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      if (currentVoxel(axis) < 0 or currentVoxel(axis) >= voxelsPerAxis) {
        inside = false;
        break;
      }
    }
    if (not inside) {
      return;
    }

    tarch::la::Vector<DIMENSIONS, double> voxelCentre;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      voxelCentre(axis) = offset(axis) + volumeWidth * (currentVoxel(axis) + 0.5);
    }

    double weight = 1.0;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      const double axisWeight = 1.0 - std::abs(particleX(axis) - voxelCentre(axis)) / volumeWidth;
      weight *= std::max(0.0, axisWeight);
    }

    if (weight > 1e-10) {
      const int voxelIndex = peano4::dLinearised(currentVoxel, voxelsPerAxis);
      result += weight * Q[voxelIndex * unknownsPerVoxel + unknown];
      sumWeights += weight;
    }
  });

  if (sumWeights > 1e-10) {
    result /= sumWeights;
  }

  return result;
}
