// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"

namespace exahype2::fv::particles::interpolation {
  /** Number of voxels per cell axis. setGlobalInterpolationParameters() writes it. */
  extern int globalVoxelsPerAxis;

  /** Number of unknowns per voxel. setGlobalInterpolationParameters() writes it. */
  extern int globalUnknownsPerVoxel;

  /** 0 selects no interpolation, 1 a piecewise constant one, 2 a piecewise linear one. */
  extern int globalInterpolationMethod;

  template <int... Indices, typename Particle>
  void projectValuesPiecewiseConstant(
    const peano4::dataview::CellMarker&   marker,
    int                                   voxelsPerAxis,
    int                                   unknownsPerVoxel,
    const double* NOALIAS                 Q,
    Particle&                             particle,
    tarch::la::Vector<DIMENSIONS, double> pos
  );

  template <int... Indices, typename Particle>
  void projectValuesPiecewiseLinear(
    const peano4::dataview::CellMarker&   marker,
    int                                   voxelsPerAxis,
    int                                   unknownsPerVoxel,
    const double* NOALIAS                 Q,
    Particle&                             particle,
    tarch::la::Vector<DIMENSIONS, double> pos
  );

  /**
   * Interpolates values at an integration-stage position.
   *
   * The caller provides data for the current patch only. A stage outside
   * this patch therefore samples the closest boundary volume.
   */
  template <int... Indices, typename Particle>
  void interpolateMidpoints(
    const peano4::dataview::CellMarker&          marker,
    const double* NOALIAS                        Q,
    Particle&                                    particle,
    const tarch::la::Vector<DIMENSIONS, double>& midpointPosition
  );

  /** Interpolates with the scheme globalInterpolationMethod selects. */
  template <int... Indices, typename Particle>
  void applyInterpolation(
    Particle&                             particle,
    tarch::la::Vector<DIMENSIONS, double> pos,
    const peano4::dataview::CellMarker&   marker,
    const double* NOALIAS                 Q
  );

  /** Overwrites globalVoxelsPerAxis, globalUnknownsPerVoxel and globalInterpolationMethod. */
  void setGlobalInterpolationParameters(int voxelsPerAxis, int unknownsPerVoxel, int interpolationMethod);

  /** Returns the voxel that contains particleX. */
  tarch::la::Vector<DIMENSIONS, int> mapParticleOntoVoxel(
    const peano4::dataview::CellMarker&          marker,
    int                                          voxelsPerAxis,
    const tarch::la::Vector<DIMENSIONS, double>& particleX
  );

  /** Returns the lower voxel of the pair the linear interpolation reads along each axis. */
  tarch::la::Vector<DIMENSIONS, int> mapBiasedParticleOntoVoxel(
    const peano4::dataview::CellMarker&          marker,
    int                                          voxelsPerAxis,
    const tarch::la::Vector<DIMENSIONS, double>& particleX
  );

  double projectValueOntoParticlePiecewiseConstant(
    const peano4::dataview::CellMarker&          marker,
    int                                          voxelsPerAxis,
    int                                          unknownsPerVoxel,
    const double* NOALIAS                        Q,
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    int                                          unknown
  );

  double projectValueOntoParticlePiecewiseLinear(
    const peano4::dataview::CellMarker&          marker,
    int                                          voxelsPerAxis,
    int                                          unknownsPerVoxel,
    const double* NOALIAS                        Q,
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    int                                          unknown
  );
} // namespace exahype2::fv::particles::interpolation

#include "exahype2/fv/particles/ParticleInterpolation.cpph"
