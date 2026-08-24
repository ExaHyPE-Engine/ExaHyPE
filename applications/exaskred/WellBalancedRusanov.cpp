// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "WellBalancedRusanov.h"

namespace {
  struct Shortcuts {
    static constexpr int h  = 0;
    static constexpr int hu = 1;
    static constexpr int hv = hu + 1;
    static constexpr int b  = 3;
  };

  template <typename T>
  void instantiateWellBalancedRusanov() {
    constexpr int NumberOfUnknowns           = 4;
    constexpr int NumberOfAuxiliaryVariables = 0;
    constexpr int Order                      = 1;
    constexpr int NumberOfFaceNodes          = []() constexpr {
      int result = 1;
      for (int dimension = 1; dimension < DIMENSIONS; dimension++) {
        result *= Order + 1;
      }
      return result;
    }();
    T leftState[NumberOfFaceNodes * NumberOfUnknowns]{};
    T rightState[NumberOfFaceNodes * NumberOfUnknowns]{};
    T leftFlux[NumberOfFaceNodes * NumberOfUnknowns]{};
    T rightFlux[NumberOfFaceNodes * NumberOfUnknowns]{};
    for (int node = 0; node < NumberOfFaceNodes; node++) {
      leftState[node * NumberOfUnknowns + Shortcuts::h]  = static_cast<T>(1.0);
      leftState[node * NumberOfUnknowns + Shortcuts::b]  = static_cast<T>(-1.0);
      rightState[node * NumberOfUnknowns + Shortcuts::h] = static_cast<T>(1.0);
      rightState[node * NumberOfUnknowns + Shortcuts::b] = static_cast<T>(-1.0);
    }
    const tarch::la::Vector<DIMENSIONS, double> x{};
    const tarch::la::Vector<DIMENSIONS, double> h(1.0);
    const auto                                  maxEigenvalue = [](
                                 const T* const NOALIAS                                        Q,
                                 [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& facePosition,
                                 [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& faceSize,
                                 [[maybe_unused]] const double                                 time,
                                 [[maybe_unused]] const double                                 timeStepSize,
                                 [[maybe_unused]] const int                                    direction
                               ) { return static_cast<double>(Q[Shortcuts::h]); };
    const auto interfaceGravity =
      []([[maybe_unused]] const T* const NOALIAS QLeft, [[maybe_unused]] const T* const NOALIAS QRight) {
        return 9.81;
      };
    applications::exahype2::exaskred::riemann::
      wellBalancedRusanov<Shortcuts, Shortcuts::b, NumberOfUnknowns, NumberOfAuxiliaryVariables, Order>(
        leftState,
        rightState,
        x,
        h,
        0.0,
        0.0,
        0,
        maxEigenvalue,
        interfaceGravity,
        leftFlux,
        rightFlux
      );
  }

  template void instantiateWellBalancedRusanov<float>();
  template void instantiateWellBalancedRusanov<double>();
  template void instantiateWellBalancedRusanov<long double>();
} // namespace
