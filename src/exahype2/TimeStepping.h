// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Limits.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

namespace tarch::mpi {
  class ReductionRecord;
} // namespace tarch::mpi

TARGET_DECLARE
namespace exahype2 {
  struct MaxEigenvalueReduction;
} // namespace exahype2

/**
 * Maximum reduction of the wave speeds of a time step. combine() returns a negative or non-finite
 * operand unchanged. The reduction then passes a fault of a PDE term on to its result.
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
 * Time stamp, time step size and bounds of one solver. An adaptive rule takes the time step size
 * that the CFL condition admits for the wave speeds and mesh widths of the last step. A fixed rule
 * takes the size that the solver family computes from the mesh widths.
 * addLocalValuesToGlobalReduction() and readValuesFromGlobalReduction() reduce the mesh widths and
 * wave speeds over the ranks in the reduction of a step.
 */
class exahype2::TimeStepping final {
public:
  enum class Rule { Adaptive, Fixed };

  /**
   * Eigenvalue maximum that the CFL condition of the solver family divides by.
   */
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
  [[nodiscard]] INLINE_METHOD bool hasNewMaxEigenvalues() const;
  [[nodiscard]] INLINE_METHOD bool hasNewMeshWidths() const;

  double startTimeStep();
  void raiseMaxEigenvalue(double eigenvalue);
  void raiseMaxScaledEigenvalue(double eigenvalue);
  void setLocalMeshWidths(double localMinimum, double localMaximum);
  void addLocalValuesToGlobalReduction(tarch::mpi::ReductionRecord& record);
  void readValuesFromGlobalReduction(const tarch::mpi::ReductionRecord& record);
  [[nodiscard]] bool hasPositiveStabilityEigenvalue() const;
  void advanceTimeStamp(double timeStepSize);
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

  double _localMinMeshWidth{std::numeric_limits<double>::infinity()};
  double _localMaxMeshWidth{0.0};
  bool   _maxEigenvaluesRaised{false};
  bool   _localMeshWidthsSet{false};
  bool   _hasNewMaxEigenvalues{false};
  bool   _hasNewMeshWidths{false};

  int _firstFlagSlot{0};
  int _minMeshWidthSlot{0};
  int _maxMeshWidthSlot{0};
  int _maxEigenvalueSlot{0};
  int _maxScaledEigenvalueSlot{0};
  int _firstFaultSlot{0};
};

#include "exahype2/TimeStepping.cpph"
