// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleInterpolation.h"

int exahype2::fv::particles::interpolation::_voxelsPerAxis       = 0;
int exahype2::fv::particles::interpolation::_unknownsPerVoxel    = 0;
int exahype2::fv::particles::interpolation::_interpolationMethod = 0;

void exahype2::fv::particles::interpolation::setGlobalInterpolationParameters(
  int voxelsPerAxis,
  int unknownsPerVoxel,
  int interpolationMethod
) {
  _voxelsPerAxis       = voxelsPerAxis;
  _unknownsPerVoxel    = unknownsPerVoxel;
  _interpolationMethod = interpolationMethod;
}

tarch::la::Vector<DIMENSIONS, int> exahype2::fv::particles::interpolation::internal::mapParticleOntoVoxel(
  const peano4::dataview::CellMarker&          marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<DIMENSIONS, double>& particleX
) {
  tarch::la::Vector<DIMENSIONS, int> result;
  assertion(voxelsPerAxis > 0);

  const double cartesianMeshH = marker.getH()(0) / voxelsPerAxis;

  for (int d = 0; d < DIMENSIONS; d++) {
    const double positionWithinPatch = particleX(d) - marker.getOffset()(d);
    result(d)                        = static_cast<int>(std::floor(positionWithinPatch / cartesianMeshH));
    result(d)                        = std::max(0, std::min(result(d), voxelsPerAxis - 1));
  }

  return result;
}

tarch::la::Vector<DIMENSIONS, int> exahype2::fv::particles::interpolation::internal::mapBiasedParticleOntoVoxel(
  const peano4::dataview::CellMarker&          marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<DIMENSIONS, double>& particleX
) {
  tarch::la::Vector<DIMENSIONS, int> result;
  assertion(voxelsPerAxis >= 2, marker, voxelsPerAxis, particleX);

  const double cartesianMeshH = marker.getH()(0) / voxelsPerAxis;

  for (int d = 0; d < DIMENSIONS; d++) {
    const double biasedPositionWithinPatch = particleX(d) - marker.getOffset()(d) - cartesianMeshH / 2.0;
    result(d)                              = static_cast<int>(std::floor(biasedPositionWithinPatch / cartesianMeshH));
    result(d)                              = std::max(0, std::min(result(d), voxelsPerAxis - 2));
  }

  return result;
}

double exahype2::fv::particles::interpolation::internal::projectValueOntoParticle_piecewiseConstant(
  const peano4::dataview::CellMarker&          marker,
  int                                          voxelsPerAxis,
  int                                          unknownsPerVoxel,
  const double* NOALIAS                        Q,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  int                                          unknown
) {
  const tarch::la::Vector<DIMENSIONS, int> voxel      = mapParticleOntoVoxel(marker, voxelsPerAxis, particleX);
  const int                                voxelIndex = peano4::dLinearised(voxel, voxelsPerAxis);

  return Q[voxelIndex * unknownsPerVoxel + unknown];
}

double exahype2::fv::particles::interpolation::internal::projectValueOntoParticle_piecewiseLinear(
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
  for (int d = 0; d < DIMENSIONS; d++) {
    const double relativePosition = (particleX(d) - offset(d)) / volumeWidth;
    lowerVoxel(d)                 = static_cast<int>(std::floor(relativePosition));
    lowerVoxel(d)                 = std::max(0, std::min(lowerVoxel(d), voxelsPerAxis - 2));
  }

  double result     = 0.0;
  double sumWeights = 0.0;

  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& corner, int linearisedCorner) {
    const tarch::la::Vector<DIMENSIONS, int> currentVoxel = lowerVoxel + corner;
    bool                                     inside       = true;
    for (int d = 0; d < DIMENSIONS; d++) {
      if (currentVoxel(d) < 0 or currentVoxel(d) >= voxelsPerAxis) {
        inside = false;
        break;
      }
    }
    if (not inside) {
      return;
    }

    tarch::la::Vector<DIMENSIONS, double> voxelCentre;
    for (int d = 0; d < DIMENSIONS; d++) {
      voxelCentre(d) = offset(d) + volumeWidth * (currentVoxel(d) + 0.5);
    }

    double weight = 1.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      const double axisWeight = 1.0 - std::abs(particleX(d) - voxelCentre(d)) / volumeWidth;
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
