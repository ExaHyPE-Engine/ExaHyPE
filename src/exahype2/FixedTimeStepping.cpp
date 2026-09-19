// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "FixedTimeStepping.h"

#include "tarch/NonCriticalAssertions.h"

/**
 * Stores the time step size of the next time step. A value that is not finite and positive
 * raises a non-critical assertion.
 */
void exahype2::FixedTimeStepping::setFixedTimeStepSize(const double timeStepSize) {
  _timeStepSize = timeStepSize;
  if (not(std::isfinite(timeStepSize) and timeStepSize > 0.0)) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The fixed time step size is finite and positive.",
      "Time step size=" + std::to_string(timeStepSize)
    );
  }
}
