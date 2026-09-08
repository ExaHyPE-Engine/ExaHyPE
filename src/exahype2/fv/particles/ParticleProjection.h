// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/fv/particles/ParticleInterpolation.h"
#include "peano4/datamanagement/CellMarker.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::particles::projection {
  void depositMass_PiecewiseConstant(
    const peano4::datamanagement::CellMarker&    marker,
    int                                          voxelsPerAxis,
    int                                          unknownsPerVoxel,
    double* NOALIAS                              Q,
    const tarch::la::Vector<DIMENSIONS, double>& particleX,
    double                                       mass,
    int                                          unknownIndex
  );
} // namespace exahype2::fv::particles::projection
