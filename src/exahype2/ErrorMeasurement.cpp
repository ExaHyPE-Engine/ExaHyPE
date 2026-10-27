// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorMeasurement.h"

#include "tarch/Assertions.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

/**
 * Sets the output file name on the global master only. Only the global master writes the file.
 */
void
exahype2::ErrorMeasurement::open(const std::string& fileName, const double timeDeltaBetweenSnapshots) {
  assertion(std::isfinite(timeDeltaBetweenSnapshots) and timeDeltaBetweenSnapshots >= 0.0, timeDeltaBetweenSnapshots);
  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    _database.setOutputFileName(fileName);
  }
  _database.setDataName("l1-error, l2-error, linf-error");
  _database.setTimeDeltaBetweenTwoSnapshots(timeDeltaBetweenSnapshots);
  _open = true;
}

void
exahype2::ErrorMeasurement::setSnapshotRule(const tarch::database::SnapshotRule rule) {
  _database.setSnapshotRule(rule);
}

/**
 * Returns whether record() adds or replaces a snapshot at timeStamp. Every rank returns the same
 * answer.
 */
bool
exahype2::ErrorMeasurement::isSnapshotDue(const double timeStamp) {
  return _open and _database.isSnapshotDue(timeStamp);
}

/**
 * Returns a device buffer with room for NumberOfNorms errors per cell.
 */
double*
exahype2::ErrorMeasurement::getErrorsPerCell(const int numberOfCells) {
  assertion(numberOfCells >= 0, numberOfCells);
  const auto numberOfErrors = static_cast<std::size_t>(numberOfCells) * NumberOfNorms;
  if (_errorsPerCell.getSize() < numberOfErrors) {
    _errorsPerCell.allocate(numberOfErrors);
  }
  return _errorsPerCell.getData();
}

/**
 * Merges the error sums of all ranks, records the L1, L2 and Linf norms at timeStamp and returns
 * them. Every rank calls this function collectively.
 */
std::array<double, exahype2::ErrorMeasurement::NumberOfNorms>
exahype2::ErrorMeasurement::record(const ErrorSums& localSums, const double timeStamp) {
  assertion(_open);
  static_assert(std::is_trivially_copyable_v<ErrorSums>);

  const auto&            rank          = tarch::mpi::Rank::getInstance();
  const int              numberOfRanks = rank.getNumberOfRanks();
  constexpr int          BytesPerRank  = sizeof(ErrorSums);
  std::vector<ErrorSums> sumsOfRanks(static_cast<std::size_t>(numberOfRanks));
  tarch::mpi::allgather(
    &localSums,
    BytesPerRank,
    MPI_BYTE,
    sumsOfRanks.data(),
    BytesPerRank,
    MPI_BYTE,
    rank.getCommunicator()
  );

  ErrorSums globalSums = sumsOfRanks.front();
  for (std::size_t sumsOfRank = 1; sumsOfRank < sumsOfRanks.size(); sumsOfRank++) {
    globalSums.merge(sumsOfRanks[sumsOfRank]);
  }

  const std::array<double, NumberOfNorms> norms{
    globalSums.summedError.round(),
    std::sqrt(globalSums.summedSquaredError.round()),
    globalSums.maxError
  };
  _database.addGlobalSnapshot(timeStamp, NumberOfNorms, norms.data());
  return norms;
}

/**
 * Returns sums without terms whose bounds are the largest errors over all ranks. Every rank calls
 * this function collectively and receives the same bounds.
 */
exahype2::ErrorSums
exahype2::ErrorMeasurement::getEmptySumsOfAllRanks(const LargestErrors& largestErrors) {
  const double localLargest[2]  = {largestErrors.error, largestErrors.squaredError};
  double       globalLargest[2] = {0.0, 0.0};
  tarch::mpi::allreduce(
    localLargest,
    globalLargest,
    2,
    MPI_DOUBLE,
    MPI_MAX,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  ErrorSums sums;
  sums.summedError.clear(globalLargest[0]);
  sums.summedSquaredError.clear(globalLargest[1]);
  return sums;
}

void
exahype2::ErrorMeasurement::release() {
  _errorsPerCell.release();
  _partialSums.release();
}

tarch::AllocatedBytes
exahype2::ErrorMeasurement::getAllocatedBytes() const {
  return _errorsPerCell.getAllocatedBytes() + _partialSums.getAllocatedBytes();
}
