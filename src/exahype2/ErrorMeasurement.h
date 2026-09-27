// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/database/GlobalTimeSeriesDatabase.h"
#include "tarch/la/ExactAccumulator.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  struct ErrorSums;
  class ErrorMeasurement;
  struct SumErrorsOfCells;
  struct MergeErrorSums;

  namespace tests {
    class ErrorMeasurementTest;
  } // namespace tests
} // namespace exahype2

/** Holds the exact sums of the L1 and the squared L2 errors and the largest error of a set of cells. */
struct exahype2::ErrorSums final {
  TARGET_DECLARE
  static constexpr int NumberOfNorms = 3;
  TARGET_DECLARE_END

  tarch::la::ExactAccumulator summedError{};
  tarch::la::ExactAccumulator summedSquaredError{};
  double                      maxError{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void add(const double* errors);
  INLINE_METHOD GPU_CALLABLE_METHOD void merge(const ErrorSums& other);
};

/**
 * Merges the exact L1, L2 and Linf error sums of a solver against its analytical solution over
 * all ranks, so every rank and thread layout records the same norms, and writes them as a time
 * series. The errors of every cell live in one device buffer of the measurement: A kernel of
 * the solver writes NumberOfNorms values per cell into getErrorsPerCell() and record() sums
 * them in the memory the kernels read.
 */
class exahype2::ErrorMeasurement final {
public:
  TARGET_DECLARE
  static constexpr int NumberOfNorms = ErrorSums::NumberOfNorms;
  TARGET_DECLARE_END

  ErrorMeasurement()                                         = default;
  ErrorMeasurement(const ErrorMeasurement& other)            = delete;
  ErrorMeasurement(ErrorMeasurement&& other)                 = delete;
  ErrorMeasurement& operator=(const ErrorMeasurement& other) = delete;
  ErrorMeasurement& operator=(ErrorMeasurement&& other)      = delete;
  ~ErrorMeasurement()                                        = default;

  void open(const std::string& fileName, double timeDeltaBetweenSnapshots);

  [[nodiscard]] bool isSnapshotDue(double timeStamp);

  [[nodiscard]] double* getErrorsPerCell(int numberOfCells);
  INLINE_METHOD std::array<double, NumberOfNorms> record(int numberOfCells, double timeStamp);
  std::array<double, NumberOfNorms> record(const ErrorSums& localSums, double timeStamp);

  void release();
  [[nodiscard]] tarch::AllocatedBytes getAllocatedBytes() const;

private:
  friend class exahype2::tests::ErrorMeasurementTest;

  static INLINE_METHOD ErrorSums sumErrorsOfCells(
    const double*                          errorsPerCell,
    int                                    numberOfCells,
    tarch::accelerator::Buffer<ErrorSums>& partialSums
  );

  tarch::database::GlobalTimeSeriesDatabase _database{};
  bool                                      _open{false};
  tarch::accelerator::Buffer<double>        _errorsPerCell{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<ErrorSums>     _partialSums{tarch::MemoryLocation::Device};
};

TARGET_DECLARE
/** Adds the errors of every numberOfPartialSums-th cell, starting at the cell of its index, into that partial sum. */
struct exahype2::SumErrorsOfCells final {
  const double* errorsPerCell{nullptr};
  int           numberOfCells{0};
  int           numberOfPartialSums{0};
  ErrorSums*    partialSums{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int partialSum) const;
};

/** Merges the partial sum stride entries further into the partial sum of an index divisible by twice stride. */
struct exahype2::MergeErrorSums final {
  ErrorSums* partialSums{nullptr};
  int        stride{0};
  int        numberOfPartialSums{0};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int partialSum) const;
};

namespace exahype2 {
  template <std::size_t NumberOfMeasuredUnknowns, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulateError(
    const std::array<int, NumberOfMeasuredUnknowns>& measuredUnknowns,
    const Precision*                                 value,
    const double*                                    exactSolution,
    double                                           weight,
    double (&errors)[ErrorMeasurement::NumberOfNorms]
  );

  template <
    typename SolverType,
    int         NumberOfUnknowns,
    int         NumberOfAuxiliaryVariables,
    std::size_t NumberOfMeasuredUnknowns,
    typename Precision
  >
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulateSolutionError(
    const std::array<int, NumberOfMeasuredUnknowns>& measuredUnknowns,
    const Precision*                                 value,
    const tarch::la::Vector<DIMENSIONS, double>&     x,
    double                                           timeStamp,
    double                                           timeStepSize,
    double                                           weight,
    double (&errors)[ErrorMeasurement::NumberOfNorms]
  );
} // namespace exahype2
TARGET_DECLARE_END

#include "exahype2/ErrorMeasurement.cpph"
