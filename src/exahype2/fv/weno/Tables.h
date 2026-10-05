// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <array>

#include "exahype2/fv/GaussLegendreBasis.h"

namespace exahype2::fv::weno {
  template <int Order>
  struct ReconstructionTables;
} // namespace exahype2::fv::weno

/**
 * Compile-time tables of the polynomial WENO reconstruction of Dumbser et al. of polynomial order
 * @p Order in the Gauss-Legendre basis @c Basis.
 *
 * Every candidate stencil holds Order + 1 consecutive volumes and determines a polynomial of full
 * order from their averages: An even order uses a left-sided, a central and a right-sided stencil,
 * an odd order two central ones instead of the one. A stencil's reconstruction matrix maps its
 * averages onto the nodal coefficients, and the oscillation indicator matrix sums the squared
 * derivatives of first to Order-th order.
 *
 * The matrices are members of an object rather than static arrays, so a kernel that declares a
 * local constexpr instance can read them in device code, too.
 */
template <int Order>
struct exahype2::fv::weno::ReconstructionTables final {
  static_assert(Order >= 2 and Order <= 5, "The WENO tables cover the polynomial orders two to five.");

  using Basis = exahype2::fv::GaussLegendreBasis<Order + 1>;

  static constexpr int NumberOfNodes    = Basis::NumberOfNodes;
  static constexpr int NumberOfStencils = Order % 2 == 0 ? 3 : 4;

  /**
   * Offset of the first volume of a stencil relative to the reconstructed volume.
   */
  static constexpr int firstVolumeOfStencil(const int stencil) {
    if (stencil == 0) {
      return -Order;
    }
    if (stencil == NumberOfStencils - 1) {
      return 0;
    }
    if constexpr (Order % 2 == 0) {
      return -Order / 2;
    } else {
      return stencil == 1 ? -(Order + 1) / 2 : -(Order - 1) / 2;
    }
  }

  static constexpr double linearWeight(const int stencil) {
    return (stencil == 0 or stencil == NumberOfStencils - 1) ? 1.0 : 1e5;
  }

  /**
   * Entry [stencil][node][volume] weighs the average of the stencil's volume in the nodal
   * coefficient of the node.
   */
  std::array<double, NumberOfStencils * NumberOfNodes * NumberOfNodes> reconstructionMatrices = [] {
    constexpr auto Nodes   = Basis::nodes();
    constexpr auto Weights = Basis::weights();
    std::array<double, NumberOfStencils * NumberOfNodes * NumberOfNodes> matrices{};
    for (int stencil = 0; stencil < NumberOfStencils; stencil++) {
      std::array<double, NumberOfNodes * NumberOfNodes> averages{};
      for (int volume = 0; volume < NumberOfNodes; volume++) {
        const double lower = firstVolumeOfStencil(stencil) + volume;
        for (int node = 0; node < NumberOfNodes; node++) {
          for (int point = 0; point < NumberOfNodes; point++) {
            averages[volume * NumberOfNodes + node] += Weights[point] * Basis::value(node, lower + Nodes[point]);
          }
        }
      }
      const auto inverse = exahype2::fv::invertMatrix<NumberOfNodes>(averages);
      for (int entry = 0; entry < NumberOfNodes * NumberOfNodes; entry++) {
        matrices[stencil * NumberOfNodes * NumberOfNodes + entry] = inverse[entry];
      }
    }
    return matrices;
  }();

  std::array<double, NumberOfNodes * NumberOfNodes> oscillationIndicatorMatrix = [] {
    std::array<std::array<double, NumberOfNodes>, NumberOfNodes> coefficients{};
    for (int node = 0; node < NumberOfNodes; node++) {
      coefficients[node] = Basis::monomialCoefficients(node);
    }
    std::array<double, NumberOfNodes * NumberOfNodes> matrix{};
    for (int derivative = 1; derivative <= Order; derivative++) {
      for (int node = 0; node < NumberOfNodes; node++) {
        for (int power = 0; power < NumberOfNodes; power++) {
          coefficients[node][power] = power + 1 < NumberOfNodes ? (power + 1) * coefficients[node][power + 1] : 0.0;
        }
      }
      for (int row = 0; row < NumberOfNodes; row++) {
        for (int column = 0; column < NumberOfNodes; column++) {
          for (int rowPower = 0; rowPower < NumberOfNodes; rowPower++) {
            for (int columnPower = 0; columnPower < NumberOfNodes; columnPower++) {
              matrix[row * NumberOfNodes + column] += coefficients[row][rowPower] * coefficients[column][columnPower]
                                                    / (rowPower + columnPower + 1);
            }
          }
        }
      }
    }
    return matrix;
  }();
};
