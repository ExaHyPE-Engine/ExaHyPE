// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2 {
  struct NonNegativeReduction;
} // namespace exahype2

struct exahype2::NonNegativeReduction final {
  template <typename Value>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD Value minimum(Value currentValue, Value candidateValue);

  template <typename Value>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD Value maximum(Value currentValue, Value candidateValue);

  template <typename Value>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD Value sum(Value currentValue, Value candidateValue);

private:
  template <typename Value>
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicalValue(Value value);
};
TARGET_DECLARE_END

#include "exahype2/NonNegativeReduction.cpph"
