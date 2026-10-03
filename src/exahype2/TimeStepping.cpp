// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeStepping.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

#include <mpi.h>

namespace {
  constexpr double NumericalZero{1e-10};

  /**
   * A rank without a fault contributes this value to the minimum reduction of the fault bits.
   */
  constexpr std::uint64_t NoFaultBits = std::numeric_limits<std::uint64_t>::max();

  [[nodiscard]] bool isPositiveEigenvalue(const double eigenvalue) {
    return exahype2::MaxEigenvalueReduction::isPhysicalValue(eigenvalue) and eigenvalue > 0.0;
  }

  void raiseMaximum(
    double&           target,
    const double      eigenvalue,
    const char* const expression,
    const char* const parameterName
  ) {
    if (not exahype2::MaxEigenvalueReduction::isPhysicalValue(eigenvalue)) {
      tarch::triggerNonCriticalAssertion(
        __FILE__,
        __LINE__,
        expression,
        std::string(parameterName) + " bits=" + std::to_string(std::bit_cast<std::uint64_t>(eigenvalue))
      );
    }
    target = exahype2::MaxEigenvalueReduction::combine(target, eigenvalue);
  }

  void reportNonPositiveEigenvalue(
    const double      eigenvalue,
    const char* const expression,
    const char* const parameterName
  ) {
    if (not isPositiveEigenvalue(eigenvalue)) {
      tarch::triggerNonCriticalAssertion(
        __FILE__,
        __LINE__,
        expression,
        std::string(parameterName) + " bits=" + std::to_string(std::bit_cast<std::uint64_t>(eigenvalue))
      );
    }
  }
} // namespace

exahype2::TimeStepping::TimeStepping(const Rule rule, const double factor, const Stability stability) :
  _rule(rule),
  _factor(factor),
  _stability(stability) {
  assertion(std::isfinite(factor) and factor > 0.0, factor);
}

void
exahype2::TimeStepping::setLimits(const exahype2::Limits& limits) {
  assertion(limits.terminalTime >= 0.0, limits.terminalTime);
  assertion(limits.minAdmissiblePatchSize >= 0.0, limits.minAdmissiblePatchSize);
  assertion(limits.maxAdmissiblePatchSize > 0.0, limits.maxAdmissiblePatchSize);
  assertion(limits.maxAdmissibleMeshWidth > 0.0, limits.maxAdmissibleMeshWidth);
  _limits = limits;
}

double
exahype2::TimeStepping::startTimeStep() {
  if (_rule == Rule::Adaptive) {
    _timeStepSize = _admissibleTimeStepSizeOfNextStep;
  }
  _maxEigenvalueOfThisStep       = 0.0;
  _maxScaledEigenvalueOfThisStep = 0.0;
  return _timeStepSize;
}

void
exahype2::TimeStepping::raiseMaxEigenvalue(const double eigenvalue) {
  raiseMaximum(
    _maxEigenvalueOfThisStep,
    eigenvalue,
    "The maximum eigenvalue is finite and non-negative.",
    "Maximum eigenvalue"
  );
}

void
exahype2::TimeStepping::raiseMaxScaledEigenvalue(const double eigenvalue) {
  raiseMaximum(
    _maxScaledEigenvalueOfThisStep,
    eigenvalue,
    "The scaled maximum eigenvalue is finite and non-negative.",
    "Scaled maximum eigenvalue"
  );
}

/**
 * Replaces both maxima of this step by their maxima over all ranks. A negative or non-finite
 * maximum of one rank overwrites the maximum on every rank, so every rank reads the same fault.
 */
void
exahype2::TimeStepping::reduceMaxEigenvaluesAcrossRanks() {
  const bool eigenvalueIsPhysical = exahype2::MaxEigenvalueReduction::isPhysicalValue(_maxEigenvalueOfThisStep);
  const bool scaledEigenvalueIsPhysical =
    exahype2::MaxEigenvalueReduction::isPhysicalValue(_maxScaledEigenvalueOfThisStep);
  const double        localMaxEigenvalues[2]  = {_maxEigenvalueOfThisStep, _maxScaledEigenvalueOfThisStep};
  double              globalMaxEigenvalues[2] = {0.0, 0.0};
  const std::uint64_t localFaults[4]          = {
    eigenvalueIsPhysical ? 1U : 0U,
    eigenvalueIsPhysical ? NoFaultBits : std::bit_cast<std::uint64_t>(_maxEigenvalueOfThisStep),
    scaledEigenvalueIsPhysical ? 1U : 0U,
    scaledEigenvalueIsPhysical ? NoFaultBits : std::bit_cast<std::uint64_t>(_maxScaledEigenvalueOfThisStep)
  };
  std::uint64_t globalFaults[4]{};
  MPI_Comm      communicator = tarch::mpi::Rank::getInstance().getCommunicator();
  tarch::mpi::allreduce(localMaxEigenvalues, globalMaxEigenvalues, 2, MPI_DOUBLE, MPI_MAX, communicator);
  tarch::mpi::allreduce(localFaults, globalFaults, 4, MPI_UINT64_T, MPI_MIN, communicator);
  _maxEigenvalueOfThisStep       = globalMaxEigenvalues[0];
  _maxScaledEigenvalueOfThisStep = globalMaxEigenvalues[1];
  if (globalFaults[0] == 0U) {
    _maxEigenvalueOfThisStep = std::bit_cast<double>(globalFaults[1]);
  }
  if (globalFaults[2] == 0U) {
    _maxScaledEigenvalueOfThisStep = std::bit_cast<double>(globalFaults[3]);
  }
  reportNonPositiveEigenvalue(
    _maxEigenvalueOfThisStep,
    "The reduced maximum eigenvalue is finite and positive.",
    "Maximum eigenvalue"
  );
  _maxEigenvalue = _maxEigenvalueOfThisStep;
  if (_stability == Stability::ScaledEigenvalue) {
    reportNonPositiveEigenvalue(
      _maxScaledEigenvalueOfThisStep,
      "The reduced scaled maximum eigenvalue is finite and positive.",
      "Scaled maximum eigenvalue"
    );
  }
}

bool
exahype2::TimeStepping::hasPositiveStabilityEigenvalue() const {
  return isPositiveEigenvalue(
    _stability == Stability::ScaledEigenvalue ? _maxScaledEigenvalueOfThisStep : _maxEigenvalueOfThisStep
  );
}

void
exahype2::TimeStepping::advanceTimeStamp(const double timeStepSize) {
  assertion(std::isfinite(timeStepSize) and timeStepSize >= 0.0, timeStepSize, _timeStamp);
  _timeStamp += timeStepSize;
  _timeStepSizeOfLastStep = timeStepSize;
}

void
exahype2::TimeStepping::reduceMeshWidthsAcrossRanks(const double localMinimum, const double localMaximum) {
  assertion(localMinimum > 0.0, localMinimum);
  assertion(localMaximum >= 0.0, localMaximum);
  const double localExtrema[2]  = {localMinimum, -localMaximum};
  double       globalExtrema[2] = {0.0, 0.0};
  MPI_Comm     communicator     = tarch::mpi::Rank::getInstance().getCommunicator();
  tarch::mpi::allreduce(localExtrema, globalExtrema, 2, MPI_DOUBLE, MPI_MIN, communicator);
  _minMeshWidth = globalExtrema[0];
  _maxMeshWidth = -globalExtrema[1];
  assertion(std::isfinite(_minMeshWidth) and _minMeshWidth > 0.0, _minMeshWidth, _maxMeshWidth);
  assertion(_maxMeshWidth >= _minMeshWidth, _minMeshWidth, _maxMeshWidth);
}

/**
 * Stores timeStepSizeFromCFL as the size of the next step, clipped at the terminal time. A
 * negative or non-finite size is stored unchanged.
 */
void
exahype2::TimeStepping::setAdmissibleTimeStepSize(const double timeStepSizeFromCFL) {
  assertion(_rule == Rule::Adaptive);
  const double timeStepSizeToTerminalTime = _limits.terminalTime - _timeStamp;
  assertion(std::isfinite(timeStepSizeToTerminalTime), timeStepSizeToTerminalTime, _timeStamp);
  if (not exahype2::MaxEigenvalueReduction::isPhysicalValue(timeStepSizeFromCFL)) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The proposed time step size is finite and non-negative.",
      "Time step size bits=" + std::to_string(std::bit_cast<std::uint64_t>(timeStepSizeFromCFL))
    );
    _admissibleTimeStepSizeOfNextStep = timeStepSizeFromCFL;
    return;
  }
  _admissibleTimeStepSizeOfNextStep = std::min(timeStepSizeFromCFL, timeStepSizeToTerminalTime);
  // A step size within rounding of the remaining time grows to the remaining time, so no step of a
  // few machine epsilons follows.
  const double terminalTimeTolerance = NumericalZero * std::max(1.0, std::abs(_limits.terminalTime));
  if (timeStepSizeToTerminalTime - _admissibleTimeStepSizeOfNextStep <= terminalTimeTolerance) {
    _admissibleTimeStepSizeOfNextStep = timeStepSizeToTerminalTime;
  }
  if (tarch::la::smallerEquals(timeStepSizeFromCFL, 0.0, NumericalZero)) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The CFL time step size exceeds the numerical zero tolerance.",
      "CFL time step size=" + std::to_string(timeStepSizeFromCFL)
    );
  }
}

void
exahype2::TimeStepping::setFixedTimeStepSize(const double timeStepSize) {
  assertion(_rule == Rule::Fixed);
  _timeStepSize = timeStepSize;
  if (not std::isfinite(timeStepSize) or timeStepSize <= 0.0) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The fixed time step size is finite and positive.",
      "Time step size=" + std::to_string(timeStepSize)
    );
  }
}
