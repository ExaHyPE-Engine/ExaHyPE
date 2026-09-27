// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Limits.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2 {
  struct MaxEigenvalueReduction;
} // namespace exahype2

/**
 * Reduces the wave speeds of a time step to their maximum for the CFL condition of
 * exahype2::TimeStepping. combine() returns a wave speed that is negative or not finite as soon as
 * one operand is one, so a fault of a PDE term reaches the reduction across the ranks and the
 * assertion of the time step rule; identity() is zero.
 */
struct exahype2::MaxEigenvalueReduction final {
  using ValueType = double;

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double identity();
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD double combine(double left, double right);
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicalValue(double value);
};
TARGET_DECLARE_END

namespace exahype2 {
  class TimeStepping;
} // namespace exahype2

/**
 * The time stamp, the time step size and the bounds of one solver. An adaptive rule takes the
 * time step size the CFL condition of the solver family admits for the wave speeds and the mesh
 * widths of the last step, reduced across the ranks and clipped at the terminal time; a fixed
 * rule takes the size the solver family sets from the mesh widths. The wave speed maxima of a
 * step are raised by the kernels through exahype2::MaxEigenvalueReduction, reduced across the
 * ranks and read by the CFL condition.
 */
class exahype2::TimeStepping final {
public:
  enum class Rule { Adaptive, Fixed };

  /** Names the eigenvalue maximum the CFL condition of the solver family divides by. */
  enum class Stability { PhysicalEigenvalue, ScaledEigenvalue };

  TimeStepping()                               = delete;
  TimeStepping(const TimeStepping&)            = default;
  TimeStepping(TimeStepping&&)                 = default;
  TimeStepping& operator=(const TimeStepping&) = default;
  TimeStepping& operator=(TimeStepping&&)      = default;
  ~TimeStepping()                              = default;

  TimeStepping(Rule rule, double factor, Stability stability);

  void setLimits(const exahype2::Limits& limits);
  [[nodiscard]] INLINE_METHOD const exahype2::Limits& getLimits() const;
  [[nodiscard]] INLINE_METHOD Rule getRule() const;
  [[nodiscard]] INLINE_METHOD double getFactor() const;

  [[nodiscard]] INLINE_METHOD double getTimeStamp() const;
  [[nodiscard]] INLINE_METHOD double getTimeStepSize() const;
  [[nodiscard]] INLINE_METHOD double getTimeStepSizeOfLastStep() const;
  [[nodiscard]] INLINE_METHOD double getTimeStepSizeOfNextStep() const;
  [[nodiscard]] INLINE_METHOD double getMinMeshWidth() const;
  [[nodiscard]] INLINE_METHOD double getMaxMeshWidth() const;
  [[nodiscard]] INLINE_METHOD double getMaxEigenvalue() const;
  [[nodiscard]] INLINE_METHOD double getMaxEigenvalueOfThisStep() const;
  [[nodiscard]] INLINE_METHOD double getMaxScaledEigenvalueOfThisStep() const;

  double startTimeStep();
  void raiseMaxEigenvalue(double eigenvalue);
  void raiseMaxScaledEigenvalue(double eigenvalue);
  void reduceMaxEigenvaluesAcrossRanks();
  [[nodiscard]] bool hasPositiveStabilityEigenvalue() const;
  void advanceTimeStamp(double timeStepSize);
  void reduceMeshWidthsAcrossRanks(double localMinimum, double localMaximum);
  void setAdmissibleTimeStepSize(double timeStepSizeFromCFL);
  void setFixedTimeStepSize(double timeStepSize);

private:
  Rule             _rule{Rule::Adaptive};
  double           _factor{0.0};
  Stability        _stability{Stability::PhysicalEigenvalue};
  exahype2::Limits _limits{};

  double _timeStamp{0.0};
  double _timeStepSize{0.0};
  double _timeStepSizeOfLastStep{0.0};
  double _admissibleTimeStepSizeOfNextStep{0.0};

  double _minMeshWidth{std::numeric_limits<double>::infinity()};
  double _maxMeshWidth{0.0};

  double _maxEigenvalue{0.0};
  double _maxEigenvalueOfThisStep{0.0};
  double _maxScaledEigenvalueOfThisStep{0.0};
};

#include "exahype2/TimeStepping.cpph"
