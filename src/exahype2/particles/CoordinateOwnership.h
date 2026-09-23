// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/dataview/CellMarker.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles {
  INLINE_METHOD bool ownsCoordinate(
    const peano4::dataview::CellMarker&          marker,
    const tarch::la::Vector<DIMENSIONS, double>& coordinate,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset
  );
} // namespace exahype2::particles

#include "exahype2/particles/CoordinateOwnership.cpph"
