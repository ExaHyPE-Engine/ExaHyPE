// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Tracer.h"

#include "PatchUtils.h"
#include "peano4/utils/Loop.h"
#include "tarch/logging/Log.h"

tarch::la::Vector<Dimensions, int> exahype2::fv::internal::mapParticleOntoVoxel(
  const peano4::datamanagement::CellMarker&    marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<Dimensions, double>& particleX
) {
  tarch::la::Vector<Dimensions, int> result;

  assertion(voxelsPerAxis > 0);
  double cartesianMeshH = marker.h()(0) / voxelsPerAxis;

  for (int d = 0; d < Dimensions; d++) {
    double positionWithinPatch = particleX(d) - marker.getOffset()(d);
    result(d)                  = static_cast<int>(std::floor(positionWithinPatch / cartesianMeshH));
    result(d)                  = std::max(0, result(d));
    result(d)                  = std::min(voxelsPerAxis - 1, result(d));
  }

  return result;
}

tarch::la::Vector<Dimensions, int> exahype2::fv::internal::mapBiasedParticleOntoVoxel(
  const peano4::datamanagement::CellMarker&    marker,
  int                                          voxelsPerAxis,
  const tarch::la::Vector<Dimensions, double>& particleX
) {
  tarch::la::Vector<Dimensions, int> result;

  assertion3(voxelsPerAxis>=2, marker, voxelsPerAxis, particleX);
  double cartesianMeshH = marker.h()(0) / voxelsPerAxis;

  for (int d = 0; d < Dimensions; d++) {
    double biasedPositionWithinPatch = particleX(d) - marker.getOffset()(d) - cartesianMeshH / 2.0;
    result(d)                        = static_cast<int>(std::floor(biasedPositionWithinPatch / cartesianMeshH));
    result(d)                        = std::max(0, result(d));
    result(d)                        = std::min(voxelsPerAxis - 2, result(d));
  }

  return result;
}

double exahype2::fv::internal::projectValueOntoParticle_piecewiseConstant(
  const peano4::datamanagement::CellMarker& marker,
  int                                       voxelsPerAxis,
  int                                       unknownsPerVoxel,
  const double* __restrict__ Q,
  const tarch::la::Vector<Dimensions, double>& particleX,
  int                                          unknown
) {
  tarch::la::Vector<Dimensions, int> voxel = exahype2::fv::internal::mapParticleOntoVoxel(
    marker, voxelsPerAxis, particleX
  );

  int voxelIndex = peano4::utils::dLinearised(voxel, voxelsPerAxis);

  return Q[voxelIndex * unknownsPerVoxel + unknown];
}

double exahype2::fv::internal::projectValueOntoParticle_piecewiseLinear(
  const peano4::datamanagement::CellMarker&     marker,
  int                                           voxelsPerAxis,
  int                                           unknownsPerVoxel,
  const double* __restrict__                    Q,
  const tarch::la::Vector<Dimensions, double>&  particleX,
  int                                           unknown
) {
  static tarch::logging::Log _log( "exahype2::fv::internal" );
  tarch::la::Vector<Dimensions, int> biasedVoxel = mapBiasedParticleOntoVoxel(marker, voxelsPerAxis, particleX);

  double h = getVolumeLength(marker.h(), voxelsPerAxis);

  double                                      result        = 0.0;
  dfor2(k)
    tarch::la::Vector<Dimensions, int> currentVoxel  = biasedVoxel + k;
    tarch::la::Vector<Dimensions, double>       currentVoxelX = getVolumeCentre(
      marker.x(), marker.h(), voxelsPerAxis, currentVoxel
    );
    int    currentVoxelIndex = peano4::utils::dLinearised(currentVoxel, voxelsPerAxis);
    double weight            = 1.0;
    for (int d = 0; d < Dimensions; d++) {
      double relativeDistance = std::abs(currentVoxelX(d) - particleX(d)) / h;
      if (particleX(d) < currentVoxelX(d) and currentVoxel(d) == 0) {
        weight = 1.0;
      }
      else if (particleX(d) > currentVoxelX(d) and currentVoxel(d) == voxelsPerAxis - 1) {
        weight = 1.0;
      } else if (relativeDistance < 1.0) {
        weight *= (1.0 - relativeDistance);
      } else {
        weight = 0.0;
      }
      assertion3( tarch::la::smallerEquals(weight, 1.0), weight, k, relativeDistance );
    }
    if (weight > 0.0) {
      result += weight * Q[currentVoxelIndex * unknownsPerVoxel + unknown];
    }
  enddforx
  //if (unknown==0){
  //  //std::cout << "exahype2::fv::internal::projectValueOntoParticle_piecewiseLinear(): Q_0(should close to 1.0 always)=" << result <<std::endl;
  //  logInfo( "projectValueOntoParticle_piecewiseLinear(...)", "particle x=" << ::toString(particleX) << ", marker=" << marker.toString() 
  //    << ", biasedVoxel=" << ::toString(biasedVoxel)<<", result=" << result );
  //}

  return result;
}
