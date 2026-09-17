// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/PatchUtils.h"
#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"

namespace exahype2::fv {
  template <int SourceIndex, int DestIndex, typename Particle, typename DataType>
  void projectValueOntoParticle_piecewiseConstant(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <typename Particle, typename DataType>
  void projectAllValuesOntoParticle_piecewiseConstant(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <int SourceIndex, int DestIndex, typename Particle, typename DataType>
  void projectValueOntoParticle_piecewiseLinear(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <typename Particle, typename DataType>
  void projectAllValuesOntoParticle_piecewiseLinear(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <typename Particle, typename DataType>
  void projectAllValuesOntoParticle_piecewiseLinear_explicit_Euler(
    const peano4::dataview::CellMarker&   marker,
    int                                   voxelsPerAxis,
    int                                   unknownsPerVoxel,
    const DataType* NOALIAS               Q,
    double                                timeStepSize,
    tarch::la::Vector<DIMENSIONS, int>    indices,
    tarch::la::Vector<DIMENSIONS, double> factors,
    Particle&                             particle
  );

  template <typename DataType, int SourceIndex, int... SourceIndices, typename Particle>
  void projectValuesOntoParticle_piecewiseConstant(
    const peano4::dataview::CellMarker& marker,
    int                                 voxelsPerAxis,
    int                                 unknownsPerVoxel,
    const DataType* NOALIAS             Q,
    Particle&                           particle
  );

  template <typename DataType, int SourceIndex, int... SourceIndices, typename Particle>
  void projectValuesOntoParticle_piecewiseLinear(
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

  namespace internal {
    // A tracer stores density followed by one entry per axis for the fluid
    // velocity, particle velocity, and acceleration.
    inline constexpr int    ParticleVelocityAttributeOffset     = 1 + DIMENSIONS;
    inline constexpr int    ParticleAccelerationAttributeOffset = 1 + 2 * DIMENSIONS;
    inline constexpr double MaximumParticleSpeed                = 10.0;
    inline constexpr double CollisionVelocityRetention          = 0.9;

    tarch::la::Vector<DIMENSIONS, int> mapParticleOntoVoxel(
      const peano4::dataview::CellMarker&          marker,
      int                                          voxelsPerAxis,
      const tarch::la::Vector<DIMENSIONS, double>& particleX
    );

    tarch::la::Vector<DIMENSIONS, int> mapBiasedParticleOntoVoxel(
      const peano4::dataview::CellMarker&          marker,
      int                                          voxelsPerAxis,
      const tarch::la::Vector<DIMENSIONS, double>& particleX
    );

    template <typename DataType>
    DataType projectValueOntoParticle_piecewiseConstant(
      const peano4::dataview::CellMarker&          marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const DataType* NOALIAS                      Q,
      const tarch::la::Vector<DIMENSIONS, double>& particleX,
      int                                          unknown
    );

    template <typename DataType>
    DataType projectValueOntoParticle_piecewiseLinear(
      const peano4::dataview::CellMarker&          marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const DataType* NOALIAS                      Q,
      const tarch::la::Vector<DIMENSIONS, double>& particleX,
      int                                          unknown
    );

    template <typename DataType, typename Particle, int SourceIndex, int... SourceIndices>
    void projectValuesOntoParticle_piecewiseConstant(
      const peano4::dataview::CellMarker& marker,
      int                                 voxelsPerAxis,
      int                                 unknownsPerVoxel,
      const DataType* NOALIAS             Q,
      Particle&                           particle,
      int                                 destinationIndex
    );

    template <typename DataType, typename Particle, int SourceIndex, int... SourceIndices>
    void projectValuesOntoParticle_piecewiseLinear(
      const peano4::dataview::CellMarker& marker,
      int                                 voxelsPerAxis,
      int                                 unknownsPerVoxel,
      const DataType* NOALIAS             Q,
      Particle&                           particle,
      int                                 destinationIndex
    );
  } // namespace internal
} // namespace exahype2::fv

#include "exahype2/fv/Tracer.cpph"
