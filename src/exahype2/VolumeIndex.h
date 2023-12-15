// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace exahype2 {
  /**
   * A lot of the loop routines expect a voxel (or face) index which is
   * a vector over integers. Syntactically, I can just hand over
   *
   * <pre>
     volumeIndex(x, y, z)
     </pre>
   *
   * Peano's vector class then provides the right constructor to convert
   * this into a vector object. However, this constructor has to rely on
   * a while loop that iterates over the arguments. A while loop in return
   * cannot vectorise or be inlined/unrolled. So I need this helper routine
   * which is way less elegant, yet knows how often the underlying while
   * loop is to be unrolled. The unrolling happens via template magic, i.e.,
   * the compiler takes care of that.
   */

  // Base case for recursion
  template <int N, typename... Args>
  struct VolumeIndex {
    using VectorType = tarch::la::Vector<N, int>;

    static VectorType generate(Args... args) {
      VectorType result;
      fillVector<0>(result, args...); // Start with index 0
      return result;
    }

  private:
    template <int I, class... RestArgs>
    static void fillVector(VectorType& vector, int arg, RestArgs... args) {
      vector(I) = arg;
      fillVector<I + 1>(vector, args...);
    }

    // Termination condition when all arguments are processed
    template <int I>
    static void fillVector(VectorType&) {}
  };

  // Partial specialisation to handle the base case (N=0)
  template <class... Args>
  struct VolumeIndex<0, Args...> {
    using VectorType = tarch::la::Vector<0, int>;
    static VectorType generate(Args...) { return VectorType(); }
  };

  template <class... Args>
  auto volumeIndex(Args... args) {
    constexpr int N = sizeof...(args);
    return VolumeIndex<N, Args...>::generate(args...);
  }
} // namespace exahype2
