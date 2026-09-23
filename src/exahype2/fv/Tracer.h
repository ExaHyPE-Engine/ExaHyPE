// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/PatchUtils.h"
#include "exahype2/fv/particles/ParticleHelper.h"
#include "exahype2/fv/particles/ParticleInterpolation.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"

namespace exahype2::fv {
  template <int SourceIndex, int DestinationIndex, typename Particle, typename DataType>
  void projectValueOntoParticlePiecewiseConstant(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <typename Particle, typename DataType>
  void projectAllValuesOntoParticlePiecewiseConstant(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <int SourceIndex, int DestinationIndex, typename Particle, typename DataType>
  void projectValueOntoParticlePiecewiseLinear(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <typename Particle, typename DataType>
  void projectAllValuesOntoParticlePiecewiseLinear(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <typename DataType, int SourceIndex, int... SourceIndices, typename Particle>
  void projectValuesOntoParticlePiecewiseConstant(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <typename DataType, int SourceIndex, int... SourceIndices, typename Particle>
  void projectValuesOntoParticlePiecewiseLinear(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <int SourceIndex, int... SourceIndices, typename Particle, typename DataType = double>
  void updateParticleState(
    Particle&                             particle,
    double                                timeStepSize,
    const peano4::dataview::CellMarker&   marker,
    int                                   voxelsPerAxis,
    int                                   unknownsPerVoxel,
    const DataType* NOALIAS               Q,
    tarch::la::Vector<DIMENSIONS, double> domainSize,
    tarch::la::Vector<DIMENSIONS, double> domainOffset,
    tarch::la::Vector<DIMENSIONS, double> periodicBoundaryConditions
  );

  template <typename Particle>
  tarch::la::Vector<DIMENSIONS, double> explicitEulerIntegration(Particle& particle, double timeStepSize);

  template <typename Particle>
  void reflectParticleAtDomainBoundary(
    Particle&                              particle,
    tarch::la::Vector<DIMENSIONS, double>& newPosition,
    tarch::la::Vector<DIMENSIONS, double>  domainSize,
    tarch::la::Vector<DIMENSIONS, double>  domainOffset
  );

  inline constexpr double MaximumParticleSpeed       = 10.0;
  inline constexpr double CollisionVelocityRetention = 0.9;

  template <typename DataType>
  DataType interpolateValueAtPositionPiecewiseConstant(
    const peano4::dataview::CellMarker&          marker,
    int                                          voxelsPerAxis,
    int                                          unknownsPerVoxel,
    const DataType* NOALIAS                      Q,
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    int                                          unknown
  );

  template <typename DataType>
  DataType interpolateValueAtPositionPiecewiseLinear(
    const peano4::dataview::CellMarker&          marker,
    int                                          voxelsPerAxis,
    int                                          unknownsPerVoxel,
    const DataType* NOALIAS                      Q,
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    int                                          unknown
  );

  template <typename DataType, typename Particle, int SourceIndex, int... SourceIndices>
  void projectValuePackOntoParticlePiecewiseConstant(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle,
    int                                 destinationIndex
  );

  template <typename DataType, typename Particle, int SourceIndex, int... SourceIndices>
  void projectValuePackOntoParticlePiecewiseLinear(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle,
    int                                 destinationIndex
  );
} // namespace exahype2::fv

#include "exahype2/fv/Tracer.cpph"
