// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Buffer.h"
#include "tarch/accelerator/MemoryLocation.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/database/GlobalTimeSeriesDatabase.h"
#include "tarch/database/SnapshotDatabase.h"
#include "tarch/la/FixedPointSum.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  struct ErrorSums;
  struct LargestErrors;
  class ErrorMeasurement;
  struct FindLargestErrorsOfCells;
  struct SumErrorsOfCells;
  struct MergeErrorSums;
} // namespace exahype2

/**
 * Sums of the L1 and the squared L2 errors and the largest error of a set of cells. Every order
 * of the cells returns the same bits.
 */
struct exahype2::ErrorSums final {
  TARGET_DECLARE
  static constexpr int          NumberOfNorms        = 3;
  static constexpr std::int64_t MaximumNumberOfCells = std::int64_t{1} << 40;
  TARGET_DECLARE_END

  tarch::la::FixedPointSum<MaximumNumberOfCells> summedError{};
  tarch::la::FixedPointSum<MaximumNumberOfCells> summedSquaredError{};
  double                                         maxError{0.0};

  INLINE_METHOD GPU_CALLABLE_METHOD void add(const double* errors);
  INLINE_METHOD GPU_CALLABLE_METHOD void merge(const ErrorSums& other);
};

/**
 * Largest L1 error and largest squared L2 error of a set of cells. The struct is the operation of
 * a tarch::accelerator::Custom reducer.
 */
struct exahype2::LargestErrors final {
  using ValueType = LargestErrors;

  double error{0.0};
  double squaredError{0.0};

  static INLINE_METHOD GPU_CALLABLE_METHOD LargestErrors identity();
  static INLINE_METHOD GPU_CALLABLE_METHOD LargestErrors combine(LargestErrors left, LargestErrors right);
};

/**
 * Time series of the L1, L2 and Linf errors of a solver against its analytical solution. A solver
 * kernel writes NumberOfNorms errors per cell into getErrorsPerCell() and record() merges them
 * over all ranks on the integer grid that the largest errors of all ranks set. The norms do not
 * depend on the rank and thread layout.
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
  void setSnapshotRule(tarch::database::SnapshotRule rule);
  [[nodiscard]] bool isSnapshotDue(double timeStamp);

  [[nodiscard]] double* getErrorsPerCell(int numberOfCells);
  INLINE_METHOD std::array<double, NumberOfNorms> record(int numberOfCells, double timeStamp);
  INLINE_METHOD std::array<double, NumberOfNorms> record(const std::vector<double>& errorsPerCell, double timeStamp);

  void release();
  [[nodiscard]] tarch::AllocatedBytes getAllocatedBytes() const;

private:
  [[nodiscard]] static ErrorSums getEmptySumsOfAllRanks(const LargestErrors& largestErrors);
  std::array<double, NumberOfNorms> record(const ErrorSums& localSums, double timeStamp);

  static INLINE_METHOD LargestErrors getLargestErrorsOfCells(const double* errorsPerCell, int numberOfCells);
  static INLINE_METHOD ErrorSums sumErrorsOfCells(
    const double*                          errorsPerCell,
    int                                    numberOfCells,
    const ErrorSums&                       emptySums,
    tarch::accelerator::Buffer<ErrorSums>& partialSums
  );

  tarch::database::GlobalTimeSeriesDatabase _database{};
  bool                                      _open{false};
  tarch::accelerator::Buffer<double>        _errorsPerCell{tarch::MemoryLocation::Device};
  tarch::accelerator::Buffer<ErrorSums>     _partialSums{tarch::MemoryLocation::Device};
};

TARGET_DECLARE
struct exahype2::FindLargestErrorsOfCells final {
  const double* errorsPerCell{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int cell, LargestErrors& largestErrors) const;
};

struct exahype2::SumErrorsOfCells final {
  const double* errorsPerCell{nullptr};
  int           numberOfCells{0};
  int           numberOfPartialSums{0};
  ErrorSums     emptySums{};
  ErrorSums*    partialSums{nullptr};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int partialSum) const;
};

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
