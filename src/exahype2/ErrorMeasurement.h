// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/database/GlobalTimeSeriesDatabase.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  class ErrorMeasurement;
} // namespace exahype2

/**
 * Accumulates the L1, L2 and Linf errors of a solver against its analytical solution over one
 * time step and writes the reduced norms as a time series.
 */
class exahype2::ErrorMeasurement final {
public:
  static constexpr int NumberOfNorms = 3;

  ErrorMeasurement()                                   = default;
  ErrorMeasurement(const ErrorMeasurement&)            = delete;
  ErrorMeasurement(ErrorMeasurement&&)                 = delete;
  ErrorMeasurement& operator=(const ErrorMeasurement&) = delete;
  ErrorMeasurement& operator=(ErrorMeasurement&&)      = delete;
  ~ErrorMeasurement()                                  = default;

  void open(const std::string& fileName, double timeDeltaBetweenSnapshots);

  void add(const double* error);

  std::array<double, NumberOfNorms> reduceAndRecord(double timeStamp);

private:
  tarch::database::GlobalTimeSeriesDatabase _database{};
  double                                    _errors[NumberOfNorms]{0.0, 0.0, 0.0};
};

TARGET_DECLARE
namespace exahype2 {
  template <int NumberOfMeasuredUnknowns, typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulateError(
    const int (&measuredUnknowns)[NumberOfMeasuredUnknowns],
    const Precision* value,
    const double*    exactSolution,
    double           weight,
    double (&errors)[ErrorMeasurement::NumberOfNorms]
  );

  template <
    typename SolverType,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    int NumberOfMeasuredUnknowns,
    typename Precision>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulateSolutionError(
    const int (&measuredUnknowns)[NumberOfMeasuredUnknowns],
    const Precision*                             value,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       timeStamp,
    double                                       timeStepSize,
    double                                       weight,
    double (&errors)[ErrorMeasurement::NumberOfNorms]
  );
} // namespace exahype2
TARGET_DECLARE_END

namespace exahype2 {
  template <
    typename SolverType,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    int NumberOfMeasuredUnknowns,
    typename Precision>
  INLINE_METHOD void accumulateAndRecordSolutionError(
    const int (&measuredUnknowns)[NumberOfMeasuredUnknowns],
    const Precision*                             value,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       timeStamp,
    double                                       timeStepSize,
    double                                       weight,
    ErrorMeasurement&                            measurement
  );
} // namespace exahype2

#include "exahype2/ErrorMeasurement.cpph"
