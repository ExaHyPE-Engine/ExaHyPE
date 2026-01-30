// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#pragma once

#include "../../CellAccess.h"
#include "../PatchUtils.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/Globals.h"
#include "peano4/utils/Loop.h"
#include "tarch/logging/Log.h"

#include <map>
#include <string>

namespace exahype2 {
  namespace fv {
    namespace particles {
      namespace interpolation {

        // ==========================================================================================
        // Global variables (set via setGlobalInterpolationParameters)
        // ==========================================================================================

        /**
         * Number of voxels per cell axis used for interpolation.
         */
        extern int _voxelsPerAxis;

        /**
         * Number of unknowns per voxel (e.g., velocity, pressure).
         */
        extern int _unknownsPerVoxel;

        /**
         * Interpolation method: 0 = none, 1 = constant, 2 = linear.
         */
        extern int _interpolationMethod;

        // ==========================================================================================
        // Public API: Projection and Interpolation
        // ==========================================================================================

        /**
         * Project multiple quantities from a cell to a particle using piecewise constant interpolation.
         */
        template <int... Indices, typename Particle>
        void projectValues_piecewiseConstant(
          const peano4::datamanagement::CellMarker& marker,
          int                                       voxelsPerAxis,
          int                                       unknownsPerVoxel,
          const double* __restrict__ Q,
          Particle&                             particle,
          tarch::la::Vector<DIMENSIONS, double> pos
        );

        /**
         * Project multiple quantities from a cell to a particle using piecewise linear interpolation.
         */
        template <int... Indices, typename Particle>
        void projectValues_piecewiseLinear(
          const peano4::datamanagement::CellMarker& marker,
          int                                       voxelsPerAxis,
          int                                       unknownsPerVoxel,
          const double* __restrict__ Q,
          Particle&                             particle,
          tarch::la::Vector<DIMENSIONS, double> pos
        );

        /**
         * Interpolate values at a particle's midpoint position. Automatically selects neighbor patch if needed.
         */
        template <int... Indices, typename Particle>
        void handleMidpoints(
          const peano4::datamanagement::CellMarker& marker,
          const double* __restrict__ Q,
          Particle&                                    particle,
          const tarch::la::Vector<DIMENSIONS, double>& midpointPosition
        );

        /**
         * Apply configured interpolation scheme to populate particle state from a cell.
         */
        template <int... Indices, typename Particle>
        void applyInterpolation(
          Particle&                                 particle,
          tarch::la::Vector<DIMENSIONS, double>     pos,
          const peano4::datamanagement::CellMarker& marker,
          const double* __restrict__ Q
        );

        /**
         * Set global interpolation configuration.
         *
         * @param voxels  Voxels per axis
         * @param unknowns Unknowns per voxel
         * @param method  Interpolation method (0 = none, 1 = constant, 2 = linear)
         */
        void setGlobalInterpolationParameters(int voxels, int unknowns, int method);

        // ==========================================================================================
        // Internal Utilities (not meant to be called directly in user code)
        // ==========================================================================================

        namespace internal {

          /**
           * Map particle to a voxel using nearest-neighbor assignment.
           */
          tarch::la::Vector<DIMENSIONS, int> mapParticleOntoVoxel(
            const peano4::datamanagement::CellMarker&    marker,
            int                                          voxelsPerAxis,
            const tarch::la::Vector<DIMENSIONS, double>& particleX
          );

          /**
           * Map particle to a biased voxel (used in linear interpolation).
           */
          tarch::la::Vector<DIMENSIONS, int> mapBiasedParticleOntoVoxel(
            const peano4::datamanagement::CellMarker&    marker,
            int                                          voxelsPerAxis,
            const tarch::la::Vector<DIMENSIONS, double>& particleX
          );

          /**
           * Project a single quantity from a voxel using piecewise constant interpolation.
           */
          double projectValueOntoParticle_piecewiseConstant(
            const peano4::datamanagement::CellMarker& marker,
            int                                       voxelsPerAxis,
            int                                       unknownsPerVoxel,
            const double* __restrict__ Q,
            const tarch::la::Vector<DIMENSIONS, double>& particleX,
            int                                          unknown
          );

          /**
           * Project a single quantity from a voxel using piecewise linear interpolation.
           */
          double projectValueOntoParticle_piecewiseLinear(
            const peano4::datamanagement::CellMarker& marker,
            int                                       voxelsPerAxis,
            int                                       unknownsPerVoxel,
            const double* __restrict__ Q,
            const tarch::la::Vector<DIMENSIONS, double>& particleX,
            int                                          unknown
          );

        } // namespace internal

      } // namespace interpolation
    } // namespace particles
  } // namespace fv
} // namespace exahype2

#include "ParticleInterpolation.cpph"
