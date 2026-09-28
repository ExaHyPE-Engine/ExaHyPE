// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorMeasurement.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

/**
 * Sets the interval between two recorded snapshots on every rank and names the output file on
 * the global master, the one rank that writes the series.
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

/**
 * Returns whether record() adds or replaces a snapshot at timeStamp. Every rank records the same
 * norms at the same time stamps, so every rank returns the same answer.
 */
bool
exahype2::ErrorMeasurement::isSnapshotDue(const double timeStamp) {
  return _open and _database.isSnapshotDue(timeStamp);
}

/**
 * Returns the buffer that holds NumberOfNorms errors for each of the numberOfCells cells.
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
 * Returns the L1, L2 and Linf norms at timeStamp of the error sums of all ranks and records them
 * on every rank. Collective over all ranks; the sums remain exact until every rank's sums are
 * merged.
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

  ErrorSums globalSums;
  for (const ErrorSums& sumsOfRank : sumsOfRanks) {
    globalSums.merge(sumsOfRank);
  }

  const std::array<double, NumberOfNorms> norms{
    globalSums.summedError.round(),
    std::sqrt(globalSums.summedSquaredError.round()),
    globalSums.maxError
  };
  _database.addGlobalSnapshot(timeStamp, NumberOfNorms, norms.data());
  return norms;
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
