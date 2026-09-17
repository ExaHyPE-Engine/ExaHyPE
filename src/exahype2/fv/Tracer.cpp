// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Tracer.h"

/**
 * Returns the halo-free patch volume containing particleX.
 * Positions outside the cell are clamped to the closest volume.
 */
tarch::la::Vector<DIMENSIONS, int> exahype2::fv::internal::mapParticleOntoVoxel(
  const peano4::dataview::CellMarker&          marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<DIMENSIONS, double>& particleX
) {
  tarch::la::Vector<DIMENSIONS, int> voxelIndex;

  assertion(voxelsPerAxis > 0);
  const double volumeWidth = marker.getH()(0) / voxelsPerAxis;

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double positionWithinPatch = particleX(axis) - marker.getOffset()(axis);
    voxelIndex(axis)                 = static_cast<int>(std::floor(positionWithinPatch / volumeWidth));
    voxelIndex(axis)                 = std::max(0, voxelIndex(axis));
    voxelIndex(axis)                 = std::min(voxelsPerAxis - 1, voxelIndex(axis));
  }

  return voxelIndex;
}

/**
 * Returns the lower interpolation volume for particleX.
 * Each component is clamped to [0, voxelsPerAxis-2], so its upper neighbour exists.
 */
tarch::la::Vector<DIMENSIONS, int> exahype2::fv::internal::mapBiasedParticleOntoVoxel(
  const peano4::dataview::CellMarker&          marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<DIMENSIONS, double>& particleX
) {
  tarch::la::Vector<DIMENSIONS, int> lowerVoxelIndex;

  assertion(voxelsPerAxis >= 2, marker, voxelsPerAxis, particleX);
  const double volumeWidth = marker.getH()(0) / voxelsPerAxis;

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    const double biasedPositionWithinPatch = particleX(axis) - marker.getOffset()(axis) - volumeWidth / 2.0;
    lowerVoxelIndex(axis)                  = static_cast<int>(std::floor(biasedPositionWithinPatch / volumeWidth));
    lowerVoxelIndex(axis)                  = std::max(0, lowerVoxelIndex(axis));
    lowerVoxelIndex(axis)                  = std::min(voxelsPerAxis - 2, lowerVoxelIndex(axis));
  }

  return lowerVoxelIndex;
}
