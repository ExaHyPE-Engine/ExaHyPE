// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/particles/ParticleSet.h"
#include "exahype2/particles/TrajectoryDatabase.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::particles {
  template <int NumberOfData>
  [[nodiscard]] INLINE_METHOD bool isSnapshotDue(
    ParticleSet<NumberOfData>& particles,
    double                     timeStamp,
    double                     initialRecordTime,
    double                     lastRecordTime,
    TrajectoryDatabase&        database
  );

  template <int NumberOfData>
  [[nodiscard]] INLINE_METHOD bool isSnapshotDueOnSomeRank(
    ParticleSet<NumberOfData>& particles,
    double                     timeStamp,
    double                     initialRecordTime,
    double                     lastRecordTime,
    TrajectoryDatabase&        database
  );

  template <int NumberOfData>
  INLINE_METHOD void recordSnapshots(
    ParticleSet<NumberOfData>& particles,
    double                     timeStamp,
    double                     initialRecordTime,
    double                     lastRecordTime,
    TrajectoryDatabase&        database
  );
} // namespace exahype2::particles

#include "exahype2/particles/TrajectoryRecording.cpph"
