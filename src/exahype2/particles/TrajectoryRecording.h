// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/particles/CoordinateOwnership.h"
#include "peano4/Constants.h"
#include "peano4/dataview/CellMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles {
  template <typename Particle, typename Vertices, typename Database>
  INLINE_METHOD void recordParticleSnapshots(
    const peano4::dataview::CellMarker&          marker,
    const Vertices&                              fineGridVertices,
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    double                                       timeStamp,
    double                                       initialRecordTime,
    double                                       lastRecordTime,
    Database&                                    database
  );
} // namespace exahype2::particles

#include "exahype2/particles/TrajectoryRecording.cpph"
