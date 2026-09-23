// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Solver.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2 {
  class FixedTimeStepping;
} // namespace exahype2

/**
 * Holds the time step size every cell of a solver takes. The closing sweep of a time step
 * stores the size the solver family derives from the mesh widths of this time step.
 */
class exahype2::FixedTimeStepping : public exahype2::Solver {
public:
  FixedTimeStepping()                                    = default;
  FixedTimeStepping(const FixedTimeStepping&)            = delete;
  FixedTimeStepping(FixedTimeStepping&&)                 = delete;
  FixedTimeStepping& operator=(const FixedTimeStepping&) = delete;
  FixedTimeStepping& operator=(FixedTimeStepping&&)      = delete;
  ~FixedTimeStepping() override                          = default;

  [[nodiscard]] INLINE_METHOD double getTimeStepSize() const;

protected:
  void setFixedTimeStepSize(double timeStepSize);

private:
  double _timeStepSize{0.0};
};

#include "exahype2/FixedTimeStepping.cpph"
