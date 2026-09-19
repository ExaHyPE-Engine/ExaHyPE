// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Solver.h"
#include "tarch/CompilerSpecificSettings.h"

namespace exahype2 {
  class AdaptiveTimeStepping;
} // namespace exahype2

/**
 * Reduces wave-speed maxima across traversal tasks and ranks.
 * The solver family's CFL condition bounds the next time step.
 * The next time step ends no later than minTerminalTime().
 */
class exahype2::AdaptiveTimeStepping : public exahype2::Solver {
public:
  /**
   * The eigenvalue maximum the CFL condition of the solver family divides by.
   */
  enum class Stability { PhysicalEigenvalue, ScaledEigenvalue };

  AdaptiveTimeStepping()                                       = delete;
  AdaptiveTimeStepping(const AdaptiveTimeStepping&)            = delete;
  AdaptiveTimeStepping(AdaptiveTimeStepping&&)                 = delete;
  AdaptiveTimeStepping& operator=(const AdaptiveTimeStepping&) = delete;
  AdaptiveTimeStepping& operator=(AdaptiveTimeStepping&&)      = delete;
  ~AdaptiveTimeStepping() override                             = default;

  void setMaxEigenvalue(double eigenvalue);
  void setMaxScaledEigenvalue(double eigenvalue);

  [[nodiscard]] INLINE_METHOD double getMaxEigenvalue() const;
  [[nodiscard]] INLINE_METHOD double getTimeStepSize() const;

protected:
  explicit AdaptiveTimeStepping(Stability stability);

  [[nodiscard]] virtual double minTerminalTime() const = 0;

  void               startAdaptiveTimeStep();
  void               reduceMaxEigenvaluesAcrossRanks();
  [[nodiscard]] bool hasPositiveStabilityEigenvalue() const;
  void               proposeAdmissibleTimeStepSize(double timeStepSizeFromCFL, double terminalTime);

  double _maxEigenvalue{0.0};
  double _maxEigenvalueThisTimeStep{0.0};
  double _maxScaledEigenvalueThisTimeStep{0.0};
  double _admissibleTimeStepSize{0.0};
  double _admissibleTimeStepSizeThisTimeStep{0.0};

private:
  Stability _stability{Stability::PhysicalEigenvalue};
};

#include "exahype2/AdaptiveTimeStepping.cpph"
