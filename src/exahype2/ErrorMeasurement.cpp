// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorMeasurement.h"

#include "tarch/accelerator/Atomic.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

/**
 * Names the output file and the interval between two recorded snapshots; only the global
 * master writes the series.
 */
void exahype2::ErrorMeasurement::open(const std::string& fileName, const double timeDeltaBetweenSnapshots) {
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    _database.setOutputFileName(fileName);
    _database.setDataName("l1-error, l2-error, linf-error");
    _database.setTimeDeltaBetweenTwoSnapshots(timeDeltaBetweenSnapshots);
  }
  std::fill_n(_errors, NumberOfNorms, 0.0);
}

/**
 * Adds one cell's L1 error, squared L2 error and Linf error; every traversal task may report
 * concurrently.
 */
void exahype2::ErrorMeasurement::add(const double* const error) {
  assertion(error != nullptr);
  for (int norm = 0; norm < NumberOfNorms; norm++) {
    assertion(std::isfinite(error[norm]) and error[norm] >= 0.0, norm, error[norm]);
  }
  tarch::accelerator::addAtomic(_errors[0], error[0]);
  tarch::accelerator::addAtomic(_errors[1], error[1]);
  tarch::accelerator::updateAtomicMax(_errors[2], error[2]);
}

/**
 * Reduces the accumulated errors over all ranks, records the three norms at timeStamp and
 * clears the accumulators. Returns the norms.
 */
std::array<double, exahype2::ErrorMeasurement::NumberOfNorms> exahype2::ErrorMeasurement::reduceAndRecord(
  const double timeStamp
) {
  const double   localSummedErrors[2]  = {_errors[0], _errors[1]};
  double         globalSummedErrors[2] = {0.0, 0.0};
  const double   localMaxError[1]      = {_errors[2]};
  double         globalMaxError[1]     = {0.0};
  const MPI_Comm communicator          = tarch::mpi::Rank::getInstance().getCommunicator();
  tarch::mpi::allreduce(localSummedErrors, globalSummedErrors, 2, MPI_DOUBLE, MPI_SUM, communicator);
  tarch::mpi::allreduce(localMaxError, globalMaxError, 1, MPI_DOUBLE, MPI_MAX, communicator);

  const std::array<double, NumberOfNorms> norms{
    globalSummedErrors[0],
    std::sqrt(globalSummedErrors[1]),
    globalMaxError[0]
  };
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    _database.addGlobalSnapshot(timeStamp, NumberOfNorms, norms.data());
  }
  std::fill_n(_errors, NumberOfNorms, 0.0);
  return norms;
}
