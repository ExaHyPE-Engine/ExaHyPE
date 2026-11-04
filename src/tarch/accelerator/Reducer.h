// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/accelerator/accelerator.h"

namespace tarch::accelerator {
  /**
   * Reducers for tarch::accelerator::parallelReduce(). Every backend merges its result into the
   * target variable that the reducer references. identity() seeds an accumulator and combine()
   * merges two accumulators in the order a backend chooses.
   */
  template <typename T>
  struct Max final {
    using ValueType = T;
    T& target;

    static GPU_CALLABLE_METHOD constexpr T identity() { return std::numeric_limits<T>::lowest(); }
    static GPU_CALLABLE_METHOD constexpr T combine(const T left, const T right) { return right > left ? right : left; }
  };

  template <typename T>
  struct Min final {
    using ValueType = T;
    T& target;

    static GPU_CALLABLE_METHOD constexpr T identity() { return std::numeric_limits<T>::max(); }
    static GPU_CALLABLE_METHOD constexpr T combine(const T left, const T right) { return right < left ? right : left; }
  };

  template <typename T>
  struct Sum final {
    using ValueType = T;
    T& target;

    static GPU_CALLABLE_METHOD constexpr T identity() { return T{0}; }
    static GPU_CALLABLE_METHOD constexpr T combine(const T left, const T right) { return left + right; }
  };

  /**
   * Reducer for a caller-defined operation such as a componentwise maximum. Op provides ValueType
   * and the static GPU_CALLABLE_METHOD functions identity() and combine(). Op::combine() is
   * associative and commutative and ValueType is trivially copyable.
   */
  template <typename Op>
  struct Custom final {
    using ValueType = typename Op::ValueType;
    ValueType& target;

    static GPU_CALLABLE_METHOD ValueType identity() { return Op::identity(); }
    static GPU_CALLABLE_METHOD ValueType combine(const ValueType left, const ValueType right) {
      return Op::combine(left, right);
    }
  };
} // namespace tarch::accelerator
