// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Tracer.h"

#include "tarch/instrumentation/Log.h"

tarch::la::Vector<DIMENSIONS, int> exahype2::fv::internal::mapParticleOntoVoxel(
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
    result(d)                  = std::max(0, result(d));
    result(d)                  = std::min(voxelsPerAxis - 1, result(d));
  }

  return result;
}

tarch::la::Vector<DIMENSIONS, int> exahype2::fv::internal::mapBiasedParticleOntoVoxel(
  const peano4::datamanagement::CellMarker&    marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<DIMENSIONS, double>& particleX
) {
  tarch::la::Vector<DIMENSIONS, int> result;

  assertion(voxelsPerAxis>=2, marker, voxelsPerAxis, particleX);
  double cartesianMeshH = marker.getH()(0) / voxelsPerAxis;

  for (int d = 0; d < DIMENSIONS; d++) {
    double biasedPositionWithinPatch = particleX(d) - marker.getOffset()(d) - cartesianMeshH / 2.0;
    result(d)                        = static_cast<int>(std::floor(biasedPositionWithinPatch / cartesianMeshH));
    result(d)                        = std::max(0, result(d));
    result(d)                        = std::min(voxelsPerAxis - 2, result(d));
  }

  return result;
}
