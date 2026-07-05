// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ParticleInterpolation.h"

// ================================================================================================
// Global Configuration Variables (set once externally)
// ================================================================================================

int exahype2::fv::particles::interpolation::_voxelsPerAxis       = 0;
int exahype2::fv::particles::interpolation::_unknownsPerVoxel    = 0;
int exahype2::fv::particles::interpolation::_interpolationMethod = 0;

// ================================================================================================
// Initialization Function
// ================================================================================================

void exahype2::fv::particles::interpolation::setGlobalInterpolationParameters(
  int voxelsPerAxis,
  int unknownsPerVoxel,
  int interpolationMethod
) {
  _voxelsPerAxis       = voxelsPerAxis;
  _unknownsPerVoxel    = unknownsPerVoxel;
  _interpolationMethod = interpolationMethod;
}

// ================================================================================================
// Internal helper functions
// ================================================================================================

tarch::la::Vector<DIMENSIONS, int> exahype2::fv::particles::interpolation::internal::mapParticleOntoVoxel(
  const peano4::datamanagement::CellMarker&    marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<DIMENSIONS, double>& particleX
) {
  tarch::la::Vector<DIMENSIONS, int> result;
  assertion(voxelsPerAxis > 0);

  double cartesianMeshH = marker.getH()(0) / voxelsPerAxis;

  for (int d = 0; d < DIMENSIONS; d++) {
    double positionWithinPatch = particleX(d) - marker.getOffset()(d);
    result(d)                  = static_cast<int>(std::floor(positionWithinPatch / cartesianMeshH));
    result(d)                  = std::max(0, std::min(result(d), voxelsPerAxis - 1));
  }

  return result;
}

tarch::la::Vector<DIMENSIONS, int> exahype2::fv::particles::interpolation::internal::mapBiasedParticleOntoVoxel(
  const peano4::datamanagement::CellMarker&    marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<DIMENSIONS, double>& particleX
) {
  tarch::la::Vector<DIMENSIONS, int> result;
  assertion(voxelsPerAxis >= 2, marker, voxelsPerAxis, particleX);

  double cartesianMeshH = marker.getH()(0) / voxelsPerAxis;

  for (int d = 0; d < DIMENSIONS; d++) {
    double biasedPositionWithinPatch = particleX(d) - marker.getOffset()(d) - cartesianMeshH / 2.0;
    result(d)                        = static_cast<int>(std::floor(biasedPositionWithinPatch / cartesianMeshH));
    result(d)                        = std::max(0, std::min(result(d), voxelsPerAxis - 2));
  }

  return result;
}

double exahype2::fv::particles::interpolation::internal::projectValueOntoParticle_piecewiseConstant(
  const peano4::datamanagement::CellMarker& marker,
  int                                       voxelsPerAxis,
  int                                       unknownsPerVoxel,
  const double* __restrict__ Q,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  int                                          unknown
) {
  tarch::la::Vector<DIMENSIONS, int> voxel      = mapParticleOntoVoxel(marker, voxelsPerAxis, particleX);
  int                                voxelIndex = peano4::dLinearised(voxel, voxelsPerAxis);

  return Q[voxelIndex * unknownsPerVoxel + unknown];
}

double exahype2::fv::particles::interpolation::internal::projectValueOntoParticle_piecewiseLinear(
  const peano4::datamanagement::CellMarker& marker,
  int                                       voxelsPerAxis,
  int                                       unknownsPerVoxel,
  const double* __restrict__ Q,
  const tarch::la::Vector<DIMENSIONS, double>& particleX,
  int                                          unknown
) {
  double                                h      = marker.getH()(0) / voxelsPerAxis;
  tarch::la::Vector<DIMENSIONS, double> offset = marker.getOffset();

  // Compute lower-left voxel of stencil
  tarch::la::Vector<DIMENSIONS, int> ijk;
  for (int d = 0; d < DIMENSIONS; d++) {
    double xi = (particleX(d) - offset(d)) / h;
    ijk(d)    = static_cast<int>(std::floor(xi));
    ijk(d)    = std::max(0, std::min(ijk(d), voxelsPerAxis - 2)); // ensure stencil fits
  }

  double result     = 0.0;
  double sumWeights = 0.0;

  peano4::dForEachCorner([&](const tarch::la::Vector<DIMENSIONS, int>& k, int kScalar) {
    tarch::la::Vector<DIMENSIONS, int> currentVoxel = ijk + k;
    // Bounds check
    bool inside = true;
    for (int d = 0; d < DIMENSIONS; d++) {
      if (currentVoxel(d) < 0 or currentVoxel(d) >= voxelsPerAxis) {
        inside = false;
        break;
      }
    }
    if (not inside) {
      return;
    }

    // Compute voxel center
    tarch::la::Vector<DIMENSIONS, double> xVoxel;
    for (int d = 0; d < DIMENSIONS; d++) {
      xVoxel(d) = offset(d) + h * (currentVoxel(d) + 0.5);
    }

    // Compute 1D linear weights
    double weight = 1.0;
    for (int d = 0; d < DIMENSIONS; d++) {
      double w = 1.0 - std::abs(particleX(d) - xVoxel(d)) / h;
      weight *= std::max(0.0, w);
    }

    if (weight > 1e-10) {
      int idx = peano4::dLinearised(currentVoxel, voxelsPerAxis);
      result += weight * Q[idx * unknownsPerVoxel + unknown];
      sumWeights += weight;
    }
  });

  // Normalize
  if (sumWeights > 1e-10) {
    result /= sumWeights;
  }

  return result;
}
