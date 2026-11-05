// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Constants.h"

namespace exahype2::fv {
  template <int NumberOfNodesPerAxis>
  struct GaussLegendreBasis;

  template <int Size>
  constexpr std::array<double, Size * Size> invertMatrix(const std::array<double, Size * Size>& matrix);
} // namespace exahype2::fv

/**
 * The Lagrange basis through the Gauss-Legendre points of the unit interval, in which a
 * reconstruction represents the polynomial of a volume and a time stepping evolves it.
 *
 * Every member is a constexpr function rather than a static array, because device code cannot
 * read a static data member. A kernel evaluates what it needs into a local constexpr object.
 */
template <int NumberOfNodesPerAxis>
struct exahype2::fv::GaussLegendreBasis final {
  static_assert(NumberOfNodesPerAxis >= 3 and NumberOfNodesPerAxis <= 6, "The Gauss-Legendre rules cover three to six nodes.");

  static constexpr int NumberOfNodes = NumberOfNodesPerAxis;
  static constexpr int NodesPerCell  = peano4::power(NumberOfNodes, DIMENSIONS);

  static constexpr std::array<double, NumberOfNodes> nodes() {
    if constexpr (NumberOfNodes == 3) {
      return {0.11270166537925831148, 0.5, 0.88729833462074168852};
    } else if constexpr (NumberOfNodes == 4) {
      return {0.069431844202973712388, 0.33000947820757186760, 0.66999052179242813240, 0.93056815579702628761};
    } else if constexpr (NumberOfNodes == 5) {
      return {0.046910077030668003601, 0.23076534494715845448, 0.5, 0.76923465505284154552, 0.95308992296933199640};
    } else {
      return {
        0.033765242898423986094,
        0.16939530676686774317,
        0.38069040695840154568,
        0.61930959304159845432,
        0.83060469323313225683,
        0.96623475710157601391
      };
    }
  }

  static constexpr std::array<double, NumberOfNodes> weights() {
    if constexpr (NumberOfNodes == 3) {
      return {0.27777777777777777778, 0.44444444444444444444, 0.27777777777777777778};
    } else if constexpr (NumberOfNodes == 4) {
      return {0.17392742256872692869, 0.32607257743127307131, 0.32607257743127307131, 0.17392742256872692869};
    } else if constexpr (NumberOfNodes == 5) {
      return {
        0.11846344252809454376,
        0.23931433524968323402,
        0.28444444444444444444,
        0.23931433524968323402,
        0.11846344252809454376
      };
    } else {
      return {
        0.085662246189585172520,
        0.18038078652406930378,
        0.23395696728634552369,
        0.23395696728634552369,
        0.18038078652406930378,
        0.085662246189585172520
      };
    }
  }

  static constexpr double value(const int node, const double x) {
    constexpr auto Points = nodes();
    double         result = 1.0;
    for (int other = 0; other < NumberOfNodes; other++) {
      if (other != node) {
        result *= (x - Points[other]) / (Points[node] - Points[other]);
      }
    }
    return result;
  }

  static constexpr double derivative(const int node, const double x) {
    constexpr auto Points = nodes();
    double         result = 0.0;
    for (int omitted = 0; omitted < NumberOfNodes; omitted++) {
      if (omitted != node) {
        double term = 1.0 / (Points[node] - Points[omitted]);
        for (int other = 0; other < NumberOfNodes; other++) {
          if (other != node and other != omitted) {
            term *= (x - Points[other]) / (Points[node] - Points[other]);
          }
        }
        result += term;
      }
    }
    return result;
  }

  /**
   * Returns the monomial coefficients of one basis function, the constant one first.
   */
  static constexpr std::array<double, NumberOfNodes> monomialCoefficients(const int node) {
    constexpr auto                    Points = nodes();
    std::array<double, NumberOfNodes> coefficients{};
    coefficients[0] = 1.0;
    int degree      = 0;
    for (int other = 0; other < NumberOfNodes; other++) {
      if (other != node) {
        const double scale = 1.0 / (Points[node] - Points[other]);
        for (int power = degree + 1; power >= 0; power--) {
          const double shifted = power > 0 ? coefficients[power - 1] : 0.0;
          const double kept    = power <= degree ? coefficients[power] : 0.0;
          coefficients[power]  = scale * (shifted - Points[other] * kept);
        }
        degree++;
      }
    }
    return coefficients;
  }
};

template <int Size>
constexpr std::array<double, Size * Size> exahype2::fv::invertMatrix(const std::array<double, Size * Size>& matrix) {
  double work[Size][2 * Size]{};
  for (int row = 0; row < Size; row++) {
    for (int column = 0; column < Size; column++) {
      work[row][column] = matrix[row * Size + column];
    }
    work[row][Size + row] = 1.0;
  }
  for (int pivot = 0; pivot < Size; pivot++) {
    int largestRow = pivot;
    for (int row = pivot + 1; row < Size; row++) {
      const double candidate = work[row][pivot] < 0.0 ? -work[row][pivot] : work[row][pivot];
      const double largest   = work[largestRow][pivot] < 0.0 ? -work[largestRow][pivot] : work[largestRow][pivot];
      if (candidate > largest) {
        largestRow = row;
      }
    }
    for (int column = 0; column < 2 * Size; column++) {
      const double swapped     = work[pivot][column];
      work[pivot][column]      = work[largestRow][column];
      work[largestRow][column] = swapped;
    }
    const double scale = 1.0 / work[pivot][pivot];
    for (int column = 0; column < 2 * Size; column++) {
      work[pivot][column] *= scale;
    }
    for (int row = 0; row < Size; row++) {
      if (row != pivot) {
        const double factor = work[row][pivot];
        for (int column = 0; column < 2 * Size; column++) {
          work[row][column] -= factor * work[pivot][column];
        }
      }
    }
  }
  std::array<double, Size * Size> inverse{};
  for (int row = 0; row < Size; row++) {
    for (int column = 0; column < Size; column++) {
      inverse[row * Size + column] = work[row][Size + column];
    }
  }
  return inverse;
}
