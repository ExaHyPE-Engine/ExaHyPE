// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2 {
  // Constructs a tarch::la::Vector<N, int> from variadic args
  template <int N, typename... Args>
  struct VolumeIndex {
    static_assert(N > 0, "VolumeIndex<N,...> must have N > 0");

    using VectorType = tarch::la::Vector<N, int>;

    static INLINE_METHOD GPU_CALLABLE_METHOD VectorType generate(Args... args) {
      VectorType result;
      fillVector<0>(result, args...); // Start recursion at index 0
      return result;
    }

  private:
    // Recursive fill function
    template <int I, class FirstArg, class... RestArgs>
    static INLINE_METHOD GPU_CALLABLE_METHOD void fillVector(VectorType& vector, FirstArg arg, RestArgs... rest) {
      vector(I) = arg;
      fillVector<I + 1>(vector, rest...);
    }

    // Termination case
    template <int I>
    static INLINE_METHOD GPU_CALLABLE_METHOD void fillVector(VectorType&) {}
  };

  /**
   * A lot of the loop routines expect a voxel (or face) index which is
   * a vector over integers. Syntactically, we can just hand over
   *
   * <pre>
     volumeIndex(x, y, z)
     </pre>
   *
   * Peano's vector class then provides the right constructor to convert
   * this into a vector object. However, this constructor has to rely on
   * a while loop that iterates over the arguments. A while loop in return
   * cannot vectorise or be inlined/unrolled. So we need this helper routine
   * which is way less elegant, yet knows how often the underlying while
   * loop is to be unrolled. The unrolling happens via template magic, i.e.
   * the compiler takes care of that.
   */
  // Factory function: Deduces N from argument count
  template <class... Args>
  static INLINE_METHOD GPU_CALLABLE_METHOD auto volumeIndex(Args... args) {
    constexpr int N = sizeof...(args);
    static_assert(N > 0, "volumeIndex() must be called with at least one argument");
    return VolumeIndex<N, Args...>::generate(args...);
  }
} // namespace exahype2
