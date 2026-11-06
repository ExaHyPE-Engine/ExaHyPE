// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeStepping.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/mpi/ReductionRecord.h"

namespace {
  constexpr double NumericalZero{1e-10};

  /**
   * A rank without a fault adds this value to the minimum reduction of the fault bits.
   */
  constexpr std::uint64_t NoFaultBits = std::numeric_limits<std::uint64_t>::max();

  constexpr int SignificantDigitsOfMessages{6};

  [[nodiscard]] bool isPositiveEigenvalue(const double eigenvalue) {
    return exahype2::MaxEigenvalueReduction::isPhysicalValue(eigenvalue) and eigenvalue > 0.0;
  }

  [[nodiscard]] std::string formatValue(const double value) {
    std::ostringstream text;
    text << std::setprecision(SignificantDigitsOfMessages) << value;
    return text.str();
  }

  void raiseMaximum(double& target, const double eigenvalue, const double timeStamp, const char* const quantity) {
    if (not exahype2::MaxEigenvalueReduction::isPhysicalValue(eigenvalue)) {
      tarch::triggerNonCriticalAssertion(
        std::string("The ") + quantity + " has to be finite and non-negative in every cell.",
        std::string("The ") + quantity + " is " + formatValue(eigenvalue)
          + " in a cell at t = " + formatValue(timeStamp) + "."
      );
    }
    target = exahype2::MaxEigenvalueReduction::combine(target, eigenvalue);
  }

  void reportNonPositiveEigenvalue(const double eigenvalue, const double timeStamp, const char* const quantity) {
    if (not isPositiveEigenvalue(eigenvalue)) {
      tarch::triggerNonCriticalAssertion(
        std::string("The ") + quantity + " has to be finite in every cell and positive in at least one cell.",
        std::string("The ") + quantity + " over all cells is " + formatValue(eigenvalue)
          + " at t = " + formatValue(timeStamp) + "."
      );
    }
  }
} // namespace

exahype2::TimeStepping::TimeStepping(const Rule rule, const double factor, const Stability stability) :
  _rule(rule),
  _factor(factor),
  _stability(stability) {
  ASSERTION(std::isfinite(factor) and factor > 0.0, factor);
}

void
exahype2::TimeStepping::setLimits(const exahype2::Limits& limits) {
  ASSERTION(limits.terminalTime >= 0.0, limits.terminalTime);
  ASSERTION(limits.minAdmissiblePatchSize >= 0.0, limits.minAdmissiblePatchSize);
  ASSERTION(limits.maxAdmissiblePatchSize > 0.0, limits.maxAdmissiblePatchSize);
  ASSERTION(limits.maxAdmissibleMeshWidth > 0.0, limits.maxAdmissibleMeshWidth);
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
  raiseMaximum(_maxEigenvalueOfThisStep, eigenvalue, _timeStamp, "maximum eigenvalue");
  _maxEigenvaluesRaised = true;
}

void
exahype2::TimeStepping::raiseMaxScaledEigenvalue(const double eigenvalue) {
  raiseMaximum(_maxScaledEigenvalueOfThisStep, eigenvalue, _timeStamp, "maximum eigenvalue per cell width");
  _maxEigenvaluesRaised = true;
}

/**
 * Stores the mesh width extrema of the cells of this rank. readValuesFromGlobalReduction() reads
 * their extrema over all ranks.
 */
void
exahype2::TimeStepping::setLocalMeshWidths(const double localMinimum, const double localMaximum) {
  ASSERTION(localMinimum > 0.0, localMinimum);
  ASSERTION(localMaximum >= 0.0, localMaximum);
  _localMinMeshWidth  = localMinimum;
  _localMaxMeshWidth  = localMaximum;
  _localMeshWidthsSet = true;
}

/**
 * Adds the mesh widths and the eigenvalue maxima of this rank to record. A negative or non-finite
 * maximum also adds its bit pattern to a minimum. Every rank then reads the same fault.
 */
void
exahype2::TimeStepping::addLocalValuesToGlobalReduction(tarch::mpi::ReductionRecord& record) {
  const bool eigenvalueIsPhysical = exahype2::MaxEigenvalueReduction::isPhysicalValue(_maxEigenvalueOfThisStep);
  const bool scaledEigenvalueIsPhysical =
    exahype2::MaxEigenvalueReduction::isPhysicalValue(_maxScaledEigenvalueOfThisStep);
  const std::array<std::uint64_t, 2> flags{_maxEigenvaluesRaised ? 1U : 0U, _localMeshWidthsSet ? 1U : 0U};
  const std::array<std::uint64_t, 4> faults{
    eigenvalueIsPhysical ? 1U : 0U,
    eigenvalueIsPhysical ? NoFaultBits : std::bit_cast<std::uint64_t>(_maxEigenvalueOfThisStep),
    scaledEigenvalueIsPhysical ? 1U : 0U,
    scaledEigenvalueIsPhysical ? NoFaultBits : std::bit_cast<std::uint64_t>(_maxScaledEigenvalueOfThisStep)
  };
  _firstFlagSlot           = record.addMaxima(flags);
  _minMeshWidthSlot        = record.addMinimumOfDouble(_localMinMeshWidth);
  _maxMeshWidthSlot        = record.addMaximumOfDouble(_localMaxMeshWidth);
  _maxEigenvalueSlot       = record.addMaximumOfDouble(_maxEigenvalueOfThisStep);
  _maxScaledEigenvalueSlot = record.addMaximumOfDouble(_maxScaledEigenvalueOfThisStep);
  _firstFaultSlot          = record.addMinima(faults);
}

/**
 * Reads the mesh widths and the eigenvalue maxima of all ranks from record after its allreduce.
 * Each set of values changes only if some rank set it since its previous
 * addLocalValuesToGlobalReduction(). A fault of one rank overwrites the maximum on every rank.
 */
void
exahype2::TimeStepping::readValuesFromGlobalReduction(const tarch::mpi::ReductionRecord& record) {
  _hasNewMaxEigenvalues = record.getMaximum(_firstFlagSlot) != 0U;
  _hasNewMeshWidths     = record.getMaximum(_firstFlagSlot + 1) != 0U;
  _maxEigenvaluesRaised = false;
  _localMeshWidthsSet   = false;

  if (_hasNewMeshWidths) {
    _minMeshWidth = record.getMinimumOfDouble(_minMeshWidthSlot);
    _maxMeshWidth = record.getMaximumOfDouble(_maxMeshWidthSlot);
    ASSERTION(std::isfinite(_minMeshWidth) and _minMeshWidth > 0.0, _minMeshWidth, _maxMeshWidth);
    ASSERTION(_maxMeshWidth >= _minMeshWidth, _minMeshWidth, _maxMeshWidth);
  }
  if (not _hasNewMaxEigenvalues) {
    return;
  }

  _maxEigenvalueOfThisStep       = record.getMaximumOfDouble(_maxEigenvalueSlot);
  _maxScaledEigenvalueOfThisStep = record.getMaximumOfDouble(_maxScaledEigenvalueSlot);
  if (record.getMinimum(_firstFaultSlot) == 0U) {
    _maxEigenvalueOfThisStep = std::bit_cast<double>(record.getMinimum(_firstFaultSlot + 1));
  }
  if (record.getMinimum(_firstFaultSlot + 2) == 0U) {
    _maxScaledEigenvalueOfThisStep = std::bit_cast<double>(record.getMinimum(_firstFaultSlot + 3));
  }
  reportNonPositiveEigenvalue(_maxEigenvalueOfThisStep, _timeStamp, "maximum eigenvalue");
  _maxEigenvalue = _maxEigenvalueOfThisStep;
  if (_stability == Stability::ScaledEigenvalue) {
    reportNonPositiveEigenvalue(_maxScaledEigenvalueOfThisStep, _timeStamp, "maximum eigenvalue per cell width");
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
  ASSERTION(std::isfinite(timeStepSize) and timeStepSize >= 0.0, timeStepSize, _timeStamp);
  _timeStamp += timeStepSize;
  _timeStepSizeOfLastStep = timeStepSize;
}

/**
 * Stores timeStepSizeFromCFL as the size of the next step. The terminal time clips this size. A
 * size that is not finite or not above NumericalZero enters the next step unclipped.
 */
void
exahype2::TimeStepping::setAdmissibleTimeStepSize(const double timeStepSizeFromCFL) {
  ASSERTION(_rule == Rule::Adaptive);
  _admissibleTimeStepSizeOfNextStep = timeStepSizeFromCFL;
  if (not std::isfinite(timeStepSizeFromCFL) or timeStepSizeFromCFL <= NumericalZero) {
    tarch::triggerNonCriticalAssertion(
      "The simulation is unstable at t = " + formatValue(_timeStamp) + ": The maximum eigenvalue is "
        + formatValue(_maxEigenvalue) + " and the time step size is " + formatValue(timeStepSizeFromCFL) + " (minimum "
        + formatValue(NumericalZero) + ").",
      "Check the PDE terms, the initial condition and the boundary conditions."
    );
    return;
  }
  const double timeStepSizeToTerminalTime = _limits.terminalTime - _timeStamp;
  ASSERTION(std::isfinite(timeStepSizeToTerminalTime), timeStepSizeToTerminalTime, _timeStamp);
  // A step size within rounding of the remaining time grows to the remaining time. This step ends
  // the run at the terminal time.
  const double terminalTimeTolerance = NumericalZero * std::max(1.0, std::abs(_limits.terminalTime));
  if (timeStepSizeToTerminalTime - timeStepSizeFromCFL <= terminalTimeTolerance) {
    _admissibleTimeStepSizeOfNextStep = timeStepSizeToTerminalTime;
  }
}

void
exahype2::TimeStepping::setFixedTimeStepSize(const double timeStepSize) {
  ASSERTION(_rule == Rule::Fixed);
  _timeStepSize = timeStepSize;
  if (not std::isfinite(timeStepSize) or timeStepSize <= 0.0) {
    tarch::triggerNonCriticalAssertion(
      "The fixed time step size has to be finite and positive.",
      "The fixed time step size is " + formatValue(timeStepSize) + " at t = " + formatValue(_timeStamp) + "."
    );
  }
}
