#pragma once

#include "ParticleInterpolation.h"

#include "peano4/datamanagement/CellMarker.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  namespace fv {
    namespace particles {
      namespace projection {
        void depositMass_PiecewiseConstant(
          const peano4::datamanagement::CellMarker& marker,
          int                                       voxelsPerAxis,
          int                                       unknownsPerVoxel,
          double* __restrict__ Q,
          const tarch::la::Vector<DIMENSIONS, double>& particleX,
          double                                       mass,
          int                                          unknownIndex
        );
      } // namespace projection
    } // namespace particles
  } // namespace fv
} // namespace exahype2
