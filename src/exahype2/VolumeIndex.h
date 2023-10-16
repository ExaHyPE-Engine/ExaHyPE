#pragma once

namespace exahype2 {
  // Base case for recursion
  template <int N, typename... Args>
  struct VolumeIndex {
    using VectorType = tarch::la::Vector<N, int>;

    static VectorType compute(Args... args) {
      VectorType result;
      fillVector<N>(result, args...);
      return result;
    }

  private:
    template <int I, typename... RestArgs>
    static void fillVector(VectorType& vector, int arg, RestArgs... args) {
      vector(I) = arg;
      fillVector<I + 1>(vector, args...);
    }

    template <int I>
    static void fillVector(VectorType&) {}
  };

  // Partial specialization to handle the base case (N=0)
  template <typename... Args>
  struct VolumeIndex<0, Args...> {
    using VectorType = tarch::la::Vector<0, int>;

    static VectorType compute(Args...) {
      return VectorType();
    }
  };
}

namespace {
  /**
   * A lot of the loop routines expect a voxel (or face) index which is
   * a vector over integers. Syntactically, I can just hand over
   *
   * <pre>
     volumeIndex2d(x,y)
     </pre>
   *
   * Peano's vector class then provides the right constructor to convert
   * this into a vector object. However, this constructor has to rely on
   * a while loop that iterates over the arguments. A while loop in return
   * cannot vectorise or be inlined/unrolled. So I need this helper routine
   * which is way less elegant, yet knows how often the underlying while
   * loop is to be unrolled.
   */
  tarch::la::Vector<Dimensions, int> volumeIndex2d( int x, int y ) {
    tarch::la::Vector<Dimensions, int> result;
    result(0) = x;
    result(1) = y;
    return result;
  }

  tarch::la::Vector<Dimensions, int> volumeIndex3d( int x, int y, int z ) {
    tarch::la::Vector<Dimensions, int> result;
    result(0) = x;
    result(1) = y;
    result(2) = z;
    return result;
  }
}
