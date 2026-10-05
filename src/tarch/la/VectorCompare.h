// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/la/Scalar.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace tarch::la {
  template <int N>
  struct VectorCompare;
} // namespace tarch::la

template <int N>
struct tarch::la::VectorCompare {
public:
  INLINE_METHOD GPU_CALLABLE_METHOD VectorCompare(double accuracy = NUMERICAL_ZERO_DIFFERENCE);

  INLINE_METHOD GPU_CALLABLE_METHOD bool operator()(
    const Vector<N, double>& left,
    const Vector<N, double>& right
  ) const;

private:
  const double _accuracy;
};
TARGET_DECLARE_END

#include "tarch/la/VectorCompare.cpph"
