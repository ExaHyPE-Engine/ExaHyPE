// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ErrorMeasurement.h"

#include "tarch/accelerator/ParallelFor.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

/**
 * Returns the error sums of numberOfCells cells in errorsPerCell. The partial sums merge pairwise
 * on the device, and only the total is copied to the host.
 */
exahype2::ErrorSums
exahype2::ErrorMeasurement::sumErrorsOfCells(
  const double* const                    errorsPerCell,
  const int                              numberOfCells,
  tarch::accelerator::Buffer<ErrorSums>& partialSums
) {
  assertion(numberOfCells >= 0, numberOfCells);
  assertion(errorsPerCell != nullptr or numberOfCells == 0);
  constexpr int MaximumNumberOfPartialSums = 1024;
  const int     numberOfPartialSums        = std::clamp(numberOfCells, 1, MaximumNumberOfPartialSums);
  if (partialSums.getSize() < static_cast<std::size_t>(numberOfPartialSums)) {
    partialSums.allocate(static_cast<std::size_t>(numberOfPartialSums));
  }
  tarch::accelerator::parallelFor(
    tarch::accelerator::Range(numberOfPartialSums),
    SumErrorsOfCells{errorsPerCell, numberOfCells, numberOfPartialSums, partialSums.getData()}
  );
  for (int stride = 1; stride < numberOfPartialSums; stride *= 2) {
    tarch::accelerator::parallelFor(
      tarch::accelerator::Range(numberOfPartialSums),
      MergeErrorSums{partialSums.getData(), stride, numberOfPartialSums}
    );
  }
  ErrorSums result;
  partialSums.copyToHost(&result, 1);
  return result;
}

/**
 * Records the norms of the first numberOfCells cells in getErrorsPerCell(). This
 * translation unit compiles with the GPU compiler when a device backend is selected.
 */
std::array<double, exahype2::ErrorMeasurement::NumberOfNorms>
exahype2::ErrorMeasurement::record(const int numberOfCells, const double timeStamp) {
  assertion(
    _errorsPerCell.getSize() >= static_cast<std::size_t>(numberOfCells) * NumberOfNorms,
    _errorsPerCell.getSize(),
    numberOfCells
  );
  return record(sumErrorsOfCells(_errorsPerCell.getData(), numberOfCells, _partialSums), timeStamp);
}

/**
 * Sets the output file name on the global master only, so only the global master writes the file.
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
