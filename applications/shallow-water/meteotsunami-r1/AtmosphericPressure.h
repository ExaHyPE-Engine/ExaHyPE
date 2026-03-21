// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace applications::exahype2::ShallowWater::R1 {
  /**
   * @brief Computes the pressure field p(x, y, t) following the Jörn test-case logic.
   *
   * The function models a one-dimensional atmospheric pressure disturbance.
   *
   * @param x Streamwise spatial coordinate [m]
   * @param y Cross-stream spatial coordinate (unused) [m]
   * @param t Physical time in seconds. In the original MATLAB setup, this corresponds
   *          to (t_index - 1) * 60.
   *
   * @tparam Pa_ Amplitude of the pressure disturbance [Pa]
   * @tparam U_  Propagation velocity of the disturbance [m/s]
   * @tparam T_  Period of the disturbance [s]
   * @tparam P0_  Period of the disturbance [s]
   *
   * @return Pressure value p(x, y, t) [Pa]
   */
  template <auto Pa_, auto U_, auto T_, auto P0_>
  static inline auto atmosphericPressure(const auto x, [[maybe_unused]] const auto y, const auto t);
} // namespace applications::exahype2::ShallowWater::R1

#include "AtmosphericPressure.cpph"
