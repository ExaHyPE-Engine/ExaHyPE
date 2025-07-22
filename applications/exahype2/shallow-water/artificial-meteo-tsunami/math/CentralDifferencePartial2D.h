#pragma once

#include <cmath>

namespace math::differentiation::numeric_3D::central_difference {
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
  inline auto partialX(auto f, const auto x, const auto y, const auto t) {
    const auto h = std::sqrt(std::numeric_limits<decltype(x)>::epsilon()) * std::max(1.0, std::abs(x));
    return (f(x + h, y, t) - f(x - h, y, t)) / (2.0 * h);
  };

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
  inline auto partialY(auto f, const auto x, const auto y, const auto t) {
    const auto h = std::sqrt(std::numeric_limits<decltype(y)>::epsilon()) * std::max(1.0, std::abs(y));
    return (f(x, y + h, t) - f(x, y - h, t)) / (2.0 * h);
  };

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
  inline auto partialT(auto f, const auto x, const auto y, const auto t) {
    const auto h = std::sqrt(std::numeric_limits<decltype(t)>::epsilon()) * std::max(1.0, std::abs(t));
    return (f(x, y, t + h) - f(x, y, t - h)) / (2.0 * h);
  };
} // namespace math::differentiation::numeric_3D::central_difference
