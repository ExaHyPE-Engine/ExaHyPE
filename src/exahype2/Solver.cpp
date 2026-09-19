// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Solver.h"

#include "tarch/accelerator/Atomic.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

/**
 * Records that mesh refinement ran, so the next startTimeStep() reacts to a changed grid and
 * resets the flag.
 */
void exahype2::Solver::startDynamicMeshRefinementStep() { _meshRefinementRanSinceLastTimeStep = true; }

void exahype2::Solver::finishDynamicMeshRefinementStep() {}

double exahype2::Solver::getMinTimeStamp() const {
  return _minTimeStampThisTimeStep != std::numeric_limits<double>::infinity() ? _minTimeStampThisTimeStep
                                                                              : _minTimeStamp;
}

double exahype2::Solver::getMaxTimeStamp() const {
  // Only the minimum starts at infinity, so it says whether a cell has reported this time step.
  return _minTimeStampThisTimeStep != std::numeric_limits<double>::infinity() ? _maxTimeStampThisTimeStep
                                                                              : _maxTimeStamp;
}

double exahype2::Solver::getMinTimeStepSize() const {
  return _minTimeStepSizeThisTimeStep != std::numeric_limits<double>::infinity() ? _minTimeStepSizeThisTimeStep
                                                                                 : _minTimeStepSize;
}

double exahype2::Solver::getMaxTimeStepSize() const {
  // Only the minimum starts at infinity, so it says whether a cell has reported this time step.
  return _minTimeStepSizeThisTimeStep != std::numeric_limits<double>::infinity() ? _maxTimeStepSizeThisTimeStep
                                                                                 : _maxTimeStepSize;
}

double exahype2::Solver::getMinCellSize() const { return _minMeshWidth; }

double exahype2::Solver::getMaxCellSize() const { return _maxMeshWidth; }

void exahype2::Solver::mergeLocalUpdates(LocalUpdates& localUpdates) {
  if (localUpdates.empty) {
    return;
  }

  // An invalid observation overwrites both extrema, and a later atomic minimum or maximum
  // keeps at least one of them, so the rank reduction reads the invalid observation.
  if (localUpdates.timeStepSizeStatistics.hasInvalidObservation()) {
    tarch::accelerator::storeAtomic(_minTimeStepSizeThisTimeStep, localUpdates.timeStepSizeStatistics.minimum);
    tarch::accelerator::storeAtomic(_maxTimeStepSizeThisTimeStep, localUpdates.timeStepSizeStatistics.maximum);
  } else {
    tarch::accelerator::updateAtomicMin(_minTimeStepSizeThisTimeStep, localUpdates.timeStepSizeStatistics.minimum);
    tarch::accelerator::updateAtomicMax(_maxTimeStepSizeThisTimeStep, localUpdates.timeStepSizeStatistics.maximum);
  }
  tarch::accelerator::updateAtomicMin(_minTimeStampThisTimeStep, localUpdates.minTimeStamp);
  tarch::accelerator::updateAtomicMax(_maxTimeStampThisTimeStep, localUpdates.maxTimeStamp);
  tarch::accelerator::updateAtomicMin(_minMeshWidthThisTimeStep, localUpdates.minMeshWidth);
  tarch::accelerator::updateAtomicMax(_maxMeshWidthThisTimeStep, localUpdates.maxMeshWidth);
  if (localUpdates.hasHangingFaces) {
    _hasHangingFacesThisTimeStep = true;
  }

  localUpdates = LocalUpdates{};
}

void exahype2::Solver::resetTimeStepStatistics() {
  _minTimeStampThisTimeStep    = std::numeric_limits<double>::infinity();
  _maxTimeStampThisTimeStep    = 0.0;
  _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::infinity();
  _maxTimeStepSizeThisTimeStep = 0.0;
  _minMeshWidthThisTimeStep    = std::numeric_limits<double>::infinity();
  _maxMeshWidthThisTimeStep    = 0.0;
  _hasHangingFacesThisTimeStep = false;
}

/**
 * Replaces the extrema of this time step by the extrema over all ranks. A rank with an invalid
 * time step observation makes every rank hold that observation.
 */
void exahype2::Solver::reduceTimeStepStatisticsAcrossRanks() {
  // The upper half holds the negated maxima, so one minimum reduction returns both extrema,
  // and the last entry carries the hanging-face flag.
  const double localExtrema[7] = {
    _minTimeStampThisTimeStep,
    _minMeshWidthThisTimeStep,
    _minTimeStepSizeThisTimeStep,
    -_maxTimeStampThisTimeStep,
    -_maxMeshWidthThisTimeStep,
    -_maxTimeStepSizeThisTimeStep,
    _hasHangingFacesThisTimeStep ? -1.0 : 0.0
  };
  double         globalExtrema[7] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
  const MPI_Comm communicator     = tarch::mpi::Rank::getInstance().getCommunicator();

  const auto localTimeStepStatistics =
    exahype2::NonNegativeStatistics{_minTimeStepSizeThisTimeStep, _maxTimeStepSizeThisTimeStep};
  const std::uint64_t localInvalidTimeStepObservation[2]{
    localTimeStepStatistics.hasInvalidObservation() ? 0U : 1U,
    localTimeStepStatistics.invalidObservationBitsOrIdentity()
  };
  std::uint64_t globalInvalidTimeStepObservation[2]{};

  tarch::mpi::allreduce(localExtrema, globalExtrema, 7, MPI_DOUBLE, MPI_MIN, communicator);
  tarch::mpi::allreduce(
    localInvalidTimeStepObservation,
    globalInvalidTimeStepObservation,
    2,
    MPI_UINT64_T,
    MPI_MIN,
    communicator
  );

  _minTimeStampThisTimeStep    = globalExtrema[0];
  _minMeshWidthThisTimeStep    = globalExtrema[1];
  _minTimeStepSizeThisTimeStep = globalExtrema[2];
  _maxTimeStampThisTimeStep    = -globalExtrema[3];
  _maxMeshWidthThisTimeStep    = -globalExtrema[4];
  _maxTimeStepSizeThisTimeStep = -globalExtrema[5];
  _hasHangingFacesThisTimeStep = globalExtrema[6] < 0.0;
  if (globalInvalidTimeStepObservation[0] == 0U) {
    const auto invalidTimeStepStatistics =
      exahype2::NonNegativeStatistics::fromInvalidObservationBits(globalInvalidTimeStepObservation[1]);
    _minTimeStepSizeThisTimeStep = invalidTimeStepStatistics.minimum;
    _maxTimeStepSizeThisTimeStep = invalidTimeStepStatistics.maximum;
  }

  assertion(
    _minTimeStampThisTimeStep != std::numeric_limits<double>::infinity(),
    _minTimeStampThisTimeStep,
    _maxTimeStampThisTimeStep
  );
  assertion(
    _minMeshWidthThisTimeStep != std::numeric_limits<double>::infinity(),
    _minMeshWidthThisTimeStep,
    _maxMeshWidthThisTimeStep
  );
  assertion(
    _minTimeStepSizeThisTimeStep != std::numeric_limits<double>::infinity(),
    _minTimeStepSizeThisTimeStep,
    _maxTimeStepSizeThisTimeStep
  );
}

void exahype2::Solver::commitTimeStepStatistics() {
  _minTimeStamp    = _minTimeStampThisTimeStep;
  _maxTimeStamp    = _maxTimeStampThisTimeStep;
  _minTimeStepSize = _minTimeStepSizeThisTimeStep;
  _maxTimeStepSize = _maxTimeStepSizeThisTimeStep;
  _minMeshWidth    = _minMeshWidthThisTimeStep;
  _maxMeshWidth    = _maxMeshWidthThisTimeStep;
}
