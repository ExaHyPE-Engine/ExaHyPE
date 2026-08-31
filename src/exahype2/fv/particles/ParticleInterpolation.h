// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "peano4/datamanagement/CellMarker.h"

namespace exahype2::fv::particles::interpolation {
  /** Number of voxels per cell axis. setGlobalInterpolationParameters() writes it. */
  extern int _voxelsPerAxis;

  /** Number of unknowns per voxel. setGlobalInterpolationParameters() writes it. */
  extern int _unknownsPerVoxel;

  /** 0 selects no interpolation, 1 a piecewise constant one, 2 a piecewise linear one. */
  extern int _interpolationMethod;

  template <int... Indices, typename Particle>
  void projectValues_piecewiseConstant(
    const peano4::datamanagement::CellMarker& marker,
    int                                       voxelsPerAxis,
    int                                       unknownsPerVoxel,
    const double* NOALIAS                     Q,
    Particle&                                 particle,
    tarch::la::Vector<DIMENSIONS, double>     pos
  );

  template <int... Indices, typename Particle>
  void projectValues_piecewiseLinear(
    const peano4::datamanagement::CellMarker& marker,
    int                                       voxelsPerAxis,
    int                                       unknownsPerVoxel,
    const double* NOALIAS                     Q,
    Particle&                                 particle,
    tarch::la::Vector<DIMENSIONS, double>     pos
  );

  /**
   * Interpolates values at an integration-stage position.
   *
   * The caller provides data for the current patch only. A stage outside
   * this patch therefore samples the closest boundary volume.
   */
  template <int... Indices, typename Particle>
  void handleMidpoints(
    const peano4::datamanagement::CellMarker&    marker,
    const double* NOALIAS                        Q,
    Particle&                                    particle,
    const tarch::la::Vector<DIMENSIONS, double>& midpointPosition
  );

  /** Interpolates with the scheme _interpolationMethod selects. */
  template <int... Indices, typename Particle>
  void applyInterpolation(
    Particle&                                 particle,
    tarch::la::Vector<DIMENSIONS, double>     pos,
    const peano4::datamanagement::CellMarker& marker,
    const double* NOALIAS                     Q
  );

  /** Overwrites _voxelsPerAxis, _unknownsPerVoxel and _interpolationMethod. */
  void setGlobalInterpolationParameters(int voxels, int unknowns, int method);

  namespace internal {
    /** Returns the voxel that contains particleX. */
    tarch::la::Vector<DIMENSIONS, int> mapParticleOntoVoxel(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      const tarch::la::Vector<DIMENSIONS, double>& particleX
    );

    /** Returns the lower voxel of the pair the linear interpolation reads along each axis. */
    tarch::la::Vector<DIMENSIONS, int> mapBiasedParticleOntoVoxel(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      const tarch::la::Vector<DIMENSIONS, double>& particleX
    );

    double projectValueOntoParticle_piecewiseConstant(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& particleX,
      int                                          unknown
    );

    double projectValueOntoParticle_piecewiseLinear(
      const peano4::datamanagement::CellMarker&    marker,
      int                                          voxelsPerAxis,
      int                                          unknownsPerVoxel,
      const double* NOALIAS                        Q,
      const tarch::la::Vector<DIMENSIONS, double>& particleX,
      int                                          unknown
    );
  } // namespace internal
} // namespace exahype2::fv::particles::interpolation

#include "exahype2/fv/particles/ParticleInterpolation.cpph"
