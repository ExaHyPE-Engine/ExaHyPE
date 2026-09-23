// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "AdaptiveTimeStepping.h"

#include "exahype2/NonNegativeReduction.h"
#include "exahype2/NonNegativeStatistics.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

namespace {
  constexpr double NumericalZero{1e-10};

  [[nodiscard]] bool isPositiveEigenvalue(const double eigenvalue) {
    const auto statistics = exahype2::NonNegativeStatistics{eigenvalue, eigenvalue};
    return statistics.hasPhysicalValues() and eigenvalue > 0.0;
  }

  /**
   * Raises target to eigenvalue. A value that is not finite and non-negative raises a
   * non-critical assertion and replaces a target that holds a physical value. Every traversal
   * task may report concurrently.
   */
  void raiseMaxEigenvalue(
    double&           target,
    const double      eigenvalue,
    const char* const expression,
    const char* const parameterName
  ) {
    const auto observation = exahype2::NonNegativeStatistics{eigenvalue, eigenvalue};
    if (not observation.hasPhysicalValues()) {
      tarch::triggerNonCriticalAssertion(
        __FILE__,
        __LINE__,
        expression,
        std::string(parameterName) + " bits=" + std::to_string(observation.invalidObservationBitsOrIdentity())
      );
    }
    const std::atomic_ref<double> maximum(target);
    auto                          current = maximum.load(std::memory_order_relaxed);
    while (true) {
      const auto updated = exahype2::NonNegativeReduction::maximum(current, eigenvalue);
      if (std::bit_cast<std::uint64_t>(updated) == std::bit_cast<std::uint64_t>(current)) {
        return;
      }
      if (maximum.compare_exchange_weak(current, updated, std::memory_order_relaxed)) {
        return;
      }
    }
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

exahype2::AdaptiveTimeStepping::AdaptiveTimeStepping(const Stability stability) :
  _stability{stability} {}

void exahype2::AdaptiveTimeStepping::setMaxEigenvalue(const double eigenvalue) {
  raiseMaxEigenvalue(
    _maxEigenvalueThisTimeStep,
    eigenvalue,
    "The maximum eigenvalue is finite and non-negative.",
    "Maximum eigenvalue"
  );
}

void exahype2::AdaptiveTimeStepping::setMaxScaledEigenvalue(const double eigenvalue) {
  raiseMaxEigenvalue(
    _maxScaledEigenvalueThisTimeStep,
    eigenvalue,
    "The scaled maximum eigenvalue is finite and non-negative.",
    "Scaled maximum eigenvalue"
  );
}

void exahype2::AdaptiveTimeStepping::startAdaptiveTimeStep() {
  _admissibleTimeStepSize          = _admissibleTimeStepSizeThisTimeStep;
  _maxEigenvalueThisTimeStep       = 0.0;
  _maxScaledEigenvalueThisTimeStep = 0.0;
}

/**
 * Replaces the eigenvalue maxima of this time step by the maxima over all ranks and commits the
 * physical maximum. One rank with an invalid observation makes every rank hold that observation.
 * A maximum that is not finite and positive raises a non-critical assertion, the scaled maximum
 * only when the CFL condition divides by it.
 */
void exahype2::AdaptiveTimeStepping::reduceMaxEigenvaluesAcrossRanks() {
  const auto eigenvalueStatistics =
    exahype2::NonNegativeStatistics{_maxEigenvalueThisTimeStep, _maxEigenvalueThisTimeStep};
  const auto scaledEigenvalueStatistics =
    exahype2::NonNegativeStatistics{_maxScaledEigenvalueThisTimeStep, _maxScaledEigenvalueThisTimeStep};

  const double        localMaxEigenvalues[2]      = {_maxEigenvalueThisTimeStep, _maxScaledEigenvalueThisTimeStep};
  double              globalMaxEigenvalues[2]     = {0.0, 0.0};
  const std::uint64_t localInvalidObservations[4] = {
    eigenvalueStatistics.hasInvalidObservation() ? 0U : 1U,
    eigenvalueStatistics.invalidObservationBitsOrIdentity(),
    scaledEigenvalueStatistics.hasInvalidObservation() ? 0U : 1U,
    scaledEigenvalueStatistics.invalidObservationBitsOrIdentity()
  };
  std::uint64_t  globalInvalidObservations[4]{};
  const MPI_Comm communicator = tarch::mpi::Rank::getInstance().getCommunicator();

  tarch::mpi::allreduce(localMaxEigenvalues, globalMaxEigenvalues, 2, MPI_DOUBLE, MPI_MAX, communicator);
  tarch::mpi::allreduce(localInvalidObservations, globalInvalidObservations, 4, MPI_UINT64_T, MPI_MIN, communicator);

  _maxEigenvalueThisTimeStep       = globalMaxEigenvalues[0];
  _maxScaledEigenvalueThisTimeStep = globalMaxEigenvalues[1];
  if (globalInvalidObservations[0] == 0U) {
    _maxEigenvalueThisTimeStep =
      exahype2::NonNegativeStatistics::fromInvalidObservationBits(globalInvalidObservations[1]).maximum;
  }
  if (globalInvalidObservations[2] == 0U) {
    _maxScaledEigenvalueThisTimeStep =
      exahype2::NonNegativeStatistics::fromInvalidObservationBits(globalInvalidObservations[3]).maximum;
  }

  reportNonPositiveEigenvalue(
    _maxEigenvalueThisTimeStep,
    "The reduced maximum eigenvalue is finite and positive.",
    "Maximum eigenvalue"
  );
  _maxEigenvalue = _maxEigenvalueThisTimeStep;
  if (_stability == Stability::ScaledEigenvalue) {
    reportNonPositiveEigenvalue(
      _maxScaledEigenvalueThisTimeStep,
      "The reduced scaled maximum eigenvalue is finite and positive.",
      "Scaled maximum eigenvalue"
    );
  }
}

bool exahype2::AdaptiveTimeStepping::hasPositiveStabilityEigenvalue() const {
  return isPositiveEigenvalue(
    _stability == Stability::ScaledEigenvalue ? _maxScaledEigenvalueThisTimeStep : _maxEigenvalueThisTimeStep
  );
}

/**
 * Stores the time step size the next time step may take: timeStepSizeFromCFL, or the distance
 * from the minimum time stamp of this time step to terminalTime when that is smaller. A proposal
 * that is not finite and non-negative, or a CFL time step size at the numerical zero, raises a
 * non-critical assertion.
 */
void exahype2::AdaptiveTimeStepping::proposeAdmissibleTimeStepSize(
  const double timeStepSizeFromCFL,
  const double terminalTime
) {
  const auto timeStepSizeToTerminalTime = terminalTime - _minTimeStampThisTimeStep;
  auto       proposedTimeStepStatistics = exahype2::NonNegativeStatistics::accumulate({}, timeStepSizeFromCFL);
  proposedTimeStepStatistics =
    exahype2::NonNegativeStatistics::accumulate(proposedTimeStepStatistics, timeStepSizeToTerminalTime);
  if (not proposedTimeStepStatistics.hasPhysicalValues()) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The proposed time step size is finite and non-negative.",
      "Time step size bits=" + std::to_string(proposedTimeStepStatistics.invalidObservationBitsOrIdentity())
    );
  }
  _admissibleTimeStepSizeThisTimeStep = proposedTimeStepStatistics.minimum;

  if (tarch::la::smallerEquals(timeStepSizeFromCFL, 0.0, NumericalZero)) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The CFL time step size exceeds the numerical zero tolerance.",
      "CFL time step size=" + std::to_string(timeStepSizeFromCFL)
    );
  }
}
