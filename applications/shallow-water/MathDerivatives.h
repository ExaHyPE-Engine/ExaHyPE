// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace numerics::_3D {
  /**
   * @brief Approximates the partial derivative ∂f/∂x of a function f(x, y, t) using central differences.
   *
   * This lambda computes the numerical derivative of a scalar function with respect to the x-direction
   * via a second-order central difference scheme. The step size is adaptively chosen based on machine
   * precision to balance truncation and rounding errors.
   *
   * @param f The function to differentiate; must accept (x, y, t) as input
   * @param x The x-coordinate at which the derivative is evaluated
   * @param y The y-coordinate (passed to f but not differentiated)
   * @param t The time value (passed to f but not differentiated)
   * @return Approximation of ∂f/∂x at (x, y, t)
   */
  static inline auto partialX(auto f, const auto x, const auto y, const auto t);

  /**
   * @brief Approximates the partial derivative ∂f/∂y of a function f(x, y, t) using central differences.
   *
   * This lambda computes the numerical derivative of a scalar function with respect to the y-direction
   * via a second-order central difference scheme. The step size is adaptively chosen based on machine
   * precision to balance truncation and rounding errors.
   *
   * @param f The function to differentiate; must accept (x, y, t) as input
   * @param x The x-coordinate (passed to f but not differentiated)
   * @param y The y-coordinate at which the derivative is evaluated
   * @param t The time value (passed to f but not differentiated)
   * @return Approximation of ∂f/∂y at (x, y, t)
   */
  static inline auto partialY(auto f, const auto x, const auto y, const auto t);

  /**
   * @brief Approximates the partial derivative ∂f/∂t of a function f(x, y, t) using central differences.
   *
   * This lambda computes the numerical derivative of a scalar function with respect to the t-direction
   * via a second-order central difference scheme. The step size is adaptively chosen based on machine
   * precision to balance truncation and rounding errors.
   *
   * @param f The function to differentiate; must accept (x, y, t) as input
   * @param x The x-coordinate (passed to f but not differentiated)
   * @param y The y-coordinate (passed to f but not differentiated)
   * @param t The time value at which the derivative is evaluated
   * @return Approximation of ∂f/∂t at (x, y, t)
   */
  static inline auto partialT(auto f, const auto x, const auto y, const auto t);
} // namespace numerics::_3D

#include "MathDerivatives.cpph"
