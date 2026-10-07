// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeStepping.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/mpi/ReductionRecord.h"

namespace {
  constexpr double NumericalZero{1e-10};

  /**
   * A rank without a fault contributes this value to the minimum reduction of the fault bits.
   */
  constexpr std::uint64_t NoFaultBits = std::numeric_limits<std::uint64_t>::max();

  [[nodiscard]] bool isPositiveEigenvalue(const double eigenvalue) {
    return exahype2::MaxEigenvalueReduction::isPhysicalValue(eigenvalue) and eigenvalue > 0.0;
  }

  [[nodiscard]] std::string describeValue(const char* const name, const double value) {
    std::ostringstream description;
    description << name << " " << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
    return description.str();
  }

  void raiseMaximum(
    double&           target,
    const double      eigenvalue,
    const char* const expression,
    const char* const parameterName
  ) {
    if (not exahype2::MaxEigenvalueReduction::isPhysicalValue(eigenvalue)) {
      tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, expression, describeValue(parameterName, eigenvalue));
    }
    target = exahype2::MaxEigenvalueReduction::combine(target, eigenvalue);
  }

  void reportNonPositiveEigenvalue(
    const double      eigenvalue,
    const char* const expression,
    const char* const parameterName
  ) {
    if (not isPositiveEigenvalue(eigenvalue)) {
      tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, expression, describeValue(parameterName, eigenvalue));
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
 * Commits the proposed step size and clears the eigenvalue maxima of the step. An adaptive step
 * clips the proposal to the time that remains from the current time stamp. A step after a step
 * without a reduction reuses the proposal of an earlier time stamp.
 */
double
exahype2::TimeStepping::startTimeStep() {
  if (_rule == Rule::Adaptive) {
    _timeStepSize = exahype2::MaxEigenvalueReduction::isPhysicalValue(_admissibleTimeStepSizeOfNextStep)
                    ? clipToTerminalTime(_admissibleTimeStepSizeOfNextStep)
                    : _admissibleTimeStepSizeOfNextStep;
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
  _maxEigenvaluesRaised = true;
}

void
exahype2::TimeStepping::raiseMaxScaledEigenvalue(const double eigenvalue) {
  raiseMaximum(
    _maxScaledEigenvalueOfThisStep,
    eigenvalue,
    "The scaled maximum eigenvalue is finite and non-negative.",
    "Scaled maximum eigenvalue"
  );
  _maxEigenvaluesRaised = true;
}

/**
 * Stores the mesh width extrema of the cells of this rank. adoptFrom() reads their extrema over all
 * ranks.
 */
void
exahype2::TimeStepping::setLocalMeshWidths(const double localMinimum, const double localMaximum) {
  assertion(localMinimum > 0.0, localMinimum);
  assertion(localMaximum >= 0.0, localMaximum);
  _localMinMeshWidth  = localMinimum;
  _localMaxMeshWidth  = localMaximum;
  _localMeshWidthsSet = true;
}

/**
 * Adds the size of the time step since the last adoptFrom(), the mesh widths and the eigenvalue
 * maxima of this rank to record. A negative or non-finite maximum also adds its bit pattern to a
 * minimum. Every rank then reads the same fault.
 */
void
exahype2::TimeStepping::contributeTo(tarch::mpi::ReductionRecord& record) {
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
  _timeStepSizeSlot        = record.addMaximumOfDouble(_timeStampAdvanced ? _timeStepSizeOfLastStep : 0.0);
  _timeStampSlot           = record.addMaximumOfDouble(_timeStamp);
  _firstFlagSlot           = record.addMaxima(flags);
  _minMeshWidthSlot        = record.addMinimumOfDouble(_localMinMeshWidth);
  _maxMeshWidthSlot        = record.addMaximumOfDouble(_localMaxMeshWidth);
  _maxEigenvalueSlot       = record.addMaximumOfDouble(_maxEigenvalueOfThisStep);
  _maxScaledEigenvalueSlot = record.addMaximumOfDouble(_maxScaledEigenvalueOfThisStep);
  _firstFaultSlot          = record.addMinima(faults);
}

/**
 * Reads the time step size, the mesh widths and the eigenvalue maxima of all ranks from record
 * after its allreduce. Each set of values changes only if some rank set it since its previous
 * contribution. A fault of one rank overwrites the maximum on every rank.
 */
void
exahype2::TimeStepping::adoptFrom(const tarch::mpi::ReductionRecord& record) {
  _timeStepSizeOfReducedStep = record.getMaximumOfDouble(_timeStepSizeSlot);
  _timeStampOfReducedStep    = record.getMaximumOfDouble(_timeStampSlot);
  _hasNewMaxEigenvalues      = record.getMaximum(_firstFlagSlot) != 0U;
  _hasNewMeshWidths          = record.getMaximum(_firstFlagSlot + 1) != 0U;
  _maxEigenvaluesRaised      = false;
  _localMeshWidthsSet        = false;
  _timeStampAdvanced         = false;
  assertion(
    std::isfinite(_timeStepSizeOfReducedStep) and _timeStepSizeOfReducedStep >= 0.0,
    _timeStepSizeOfReducedStep
  );

  if (_hasNewMeshWidths) {
    _minMeshWidth = record.getMinimumOfDouble(_minMeshWidthSlot);
    _maxMeshWidth = record.getMaximumOfDouble(_maxMeshWidthSlot);
    assertion(std::isfinite(_minMeshWidth) and _minMeshWidth > 0.0, _minMeshWidth, _maxMeshWidth);
    assertion(_maxMeshWidth >= _minMeshWidth, _minMeshWidth, _maxMeshWidth);
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
  reportNonPositiveEigenvalue(
    _maxEigenvalueOfThisStep,
    "The maximum eigenvalue is finite in every cell and positive in at least one cell.",
    "Maximum eigenvalue"
  );
  _maxEigenvalue = _maxEigenvalueOfThisStep;
  if (_stability == Stability::ScaledEigenvalue) {
    reportNonPositiveEigenvalue(
      _maxScaledEigenvalueOfThisStep,
      "The scaled maximum eigenvalue is finite in every cell and positive in at least one cell.",
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
  _timeStampAdvanced      = true;
}

/**
 * Stores timeStepSizeFromCFL as the size of the next step. The terminal time clips this size. A
 * negative or non-finite size enters the next step unchanged.
 */
void
exahype2::TimeStepping::setAdmissibleTimeStepSize(const double timeStepSizeFromCFL) {
  assertion(_rule == Rule::Adaptive);
  if (not exahype2::MaxEigenvalueReduction::isPhysicalValue(timeStepSizeFromCFL)) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The proposed time step size is finite and non-negative.",
      describeValue("Time step size", timeStepSizeFromCFL)
    );
    _admissibleTimeStepSizeOfNextStep = timeStepSizeFromCFL;
    return;
  }
  _admissibleTimeStepSizeOfNextStep = clipToTerminalTime(timeStepSizeFromCFL);
  if (tarch::la::smallerEquals(timeStepSizeFromCFL, 0.0, NumericalZero)) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The CFL time step size exceeds the numerical zero tolerance.",
      describeValue("CFL time step size", timeStepSizeFromCFL)
    );
  }
}

/**
 * Returns timeStepSize clipped to the time that remains from the current time stamp to the
 * terminal time. A step size within rounding of the remaining time grows to the remaining time. No
 * step of a few machine epsilons follows.
 */
double
exahype2::TimeStepping::clipToTerminalTime(const double timeStepSize) const {
  const double timeStepSizeToTerminalTime = _limits.terminalTime - _timeStamp;
  assertion(std::isfinite(timeStepSizeToTerminalTime), timeStepSizeToTerminalTime, _timeStamp);
  assertion(exahype2::MaxEigenvalueReduction::isPhysicalValue(timeStepSize), timeStepSize);
  const double terminalTimeTolerance = NumericalZero * std::max(1.0, std::abs(_limits.terminalTime));
  const double clipped               = std::min(timeStepSize, timeStepSizeToTerminalTime);
  return timeStepSizeToTerminalTime - clipped <= terminalTimeTolerance ? timeStepSizeToTerminalTime : clipped;
}

/**
 * Raises a non-critical assertion if the time step of the last adopted reduction exceeds the CFL
 * limit of its own wave speeds. timeStepSizeFromCFL derives from these wave speeds.
 */
void
exahype2::TimeStepping::validateTimeStepSizeOfReducedStep(const double timeStepSizeFromCFL) const {
  assertion(_rule == Rule::Adaptive);
  if (exahype2::MaxEigenvalueReduction::isPhysicalValue(timeStepSizeFromCFL) and exceedsCFLLimit(timeStepSizeFromCFL)) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The time step size stays within the CFL limit of the wave speeds of its step.",
      describeValue("Time step size", _timeStepSizeOfReducedStep) + ", "
        + describeValue("CFL time step size", timeStepSizeFromCFL) + ", " + describeValue("CFL factor", _factor)
    );
  }
}

/**
 * Returns whether the time step of the last adopted reduction exceeds the CFL limit of its own wave
 * speeds. The limit removes a CFL factor below one from timeStepSizeFromCFL. The tolerance is
 * relative to the limit. A step that ends at the terminal time may also exceed the limit by the
 * growth of setAdmissibleTimeStepSize().
 */
bool
exahype2::TimeStepping::exceedsCFLLimit(const double timeStepSizeFromCFL) const {
  assertion(exahype2::MaxEigenvalueReduction::isPhysicalValue(timeStepSizeFromCFL), timeStepSizeFromCFL);
  const double limit                 = timeStepSizeFromCFL / std::min(_factor, 1.0);
  const double terminalTimeTolerance = NumericalZero * std::max(1.0, std::abs(_limits.terminalTime));
  const bool   endsAtTerminalTime    = _limits.terminalTime - _timeStampOfReducedStep <= terminalTimeTolerance;
  const double growth                = endsAtTerminalTime ? terminalTimeTolerance : 0.0;
  return _timeStepSizeOfReducedStep > limit * (1.0 + NumericalZero) + growth;
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
      describeValue("Time step size", timeStepSize)
    );
  }
}
