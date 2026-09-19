// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/DGUtils.h"
#include "exahype2/particles/CoordinateOwnership.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::particles {
  template <std::size_t NumberOfCoordinates, std::size_t ComponentsPerCoordinate, typename SolverType>
  INLINE_METHOD void invertCoordinates(
    SolverType& solver,
    const double (&rawCoordinates)[NumberOfCoordinates][ComponentsPerCoordinate],
    std::vector<tarch::la::Vector<DIMENSIONS, double>>& coordinates
  );

  INLINE_METHOD bool isFaultFace(const peano4::dataview::FaceMarker& marker, int faultNormal, double faultPosition);

  template <int Order, int UnknownsPerDoF, int UnknownsToStore, typename StoragePrecision, typename Database>
  INLINE_METHOD void recordCurvilinearCellSolution(
    const peano4::dataview::CellMarker&                       marker,
    const StoragePrecision*                                   quadraturePoints,
    const StoragePrecision*                                   solution,
    const std::vector<tarch::la::Vector<DIMENSIONS, double>>& coordinates,
    const tarch::la::Vector<DIMENSIONS, double>&              domainOffset,
    double                                                    timeStamp,
    Database&                                                 database
  );

  template <
    int Order,
    int UnknownsPerDoF,
    int UnknownsToStore,
    int FirstDisplacement,
    int FirstVelocity,
    typename StoragePrecision,
    typename Database>
  INLINE_METHOD void recordCurvilinearFaultSolution(
    const peano4::dataview::FaceMarker&                       marker,
    const StoragePrecision*                                   quadraturePoints,
    const StoragePrecision*                                   faceEstimates,
    int                                                       basisElementsPerFace,
    const std::vector<tarch::la::Vector<DIMENSIONS, double>>& faultCoordinates,
    int                                                       firstSnapshotId,
    double                                                    timeStamp,
    Database&                                                 database
  );
} // namespace exahype2::particles

#include "exahype2/particles/CurviTracer.cpph"
