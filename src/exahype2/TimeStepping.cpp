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

  /** A rank without a fault contributes this value to the minimum reduction of the fault bits. */
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

/**
 * Commits the time step size the previous step admitted as the size of this step, clears the
 * wave speed maxima of this step and returns the size. A fixed rule keeps its size.
 */
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
 * Replaces both maxima of this step by their maxima over every rank. A rank whose maximum is
 * negative or not finite sends its bit pattern, and every rank takes that pattern as its
 * maximum, so every rank reads the same fault.
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
 * Stores the time step size the next time step takes: timeStepSizeFromCFL, or the distance
 * from the time stamp to the terminal time when that is smaller. A CFL time step size that is
 * negative or not finite is stored as it is and raises a non-critical assertion, and so does one
 * at the numerical zero.
 */
void
exahype2::TimeStepping::setAdmissibleTimeStepSize(const double timeStepSizeFromCFL) {
  assertion(_rule == Rule::Adaptive);
  const double timeStepSizeToTerminalTime = _limits.terminalTime - _timeStamp;
  assertion(std::isfinite(timeStepSizeToTerminalTime), timeStepSizeToTerminalTime, _timeStamp);
  if (exahype2::MaxEigenvalueReduction::isPhysicalValue(timeStepSizeFromCFL)) {
    _admissibleTimeStepSizeOfNextStep = std::min(timeStepSizeFromCFL, timeStepSizeToTerminalTime);
  } else {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The proposed time step size is finite and non-negative.",
      "Time step size bits=" + std::to_string(std::bit_cast<std::uint64_t>(timeStepSizeFromCFL))
    );
    _admissibleTimeStepSizeOfNextStep = timeStepSizeFromCFL;
  }
  // A step that leaves less than the rounding of the terminal time behind stretches onto it, so
  // the run does not close with a step of a few times the machine epsilon.
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

/** Stores the size every following step of a fixed rule takes. */
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
