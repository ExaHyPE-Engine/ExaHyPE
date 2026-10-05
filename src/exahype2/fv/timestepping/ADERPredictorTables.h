// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <array>

#include "exahype2/dg/ader/PredictorScratch.h"
#include "exahype2/fv/GaussLegendreBasis.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

namespace exahype2::fv::timestepping {
  template <typename Basis, typename Precision>
  struct ADERPredictorTables;
} // namespace exahype2::fv::timestepping

TARGET_DECLARE
/**
 * The operator tables of the space-time predictor of the ADER-DG solvers over the nodal @c Basis
 * of a reconstruction, laid out as the generated tables of an ADER-DG solver. The predictor
 * integrates in space and time on the nodes of the basis, so the basis has to be collocated with
 * its quadrature like the Gauss-Legendre one.
 *
 * The tables are members of an object rather than static arrays, so a kernel that declares a
 * local constexpr instance can hand view() to the predictor in device code, too.
 */
template <typename Basis, typename Precision>
struct exahype2::fv::timestepping::ADERPredictorTables final {
private:
  static constexpr int NumberOfNodes = Basis::NumberOfNodes;
  static constexpr int NodesPerCell  = Basis::NodesPerCell;

  template <std::size_t Size>
  static constexpr std::array<Precision, Size> convert(const std::array<double, Size>& values) {
    std::array<Precision, Size> result{};
    for (std::size_t entry = 0; entry < Size; entry++) {
      result[entry] = static_cast<Precision>(values[entry]);
    }
    return result;
  }

  static constexpr std::array<double, NodesPerCell> cellWeights() {
    constexpr auto                   Weights = Basis::weights();
    std::array<double, NodesPerCell> result{};
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      double weight    = 1.0;
      int    remainder = cellNode;
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        weight *= Weights[remainder % NumberOfNodes];
        remainder /= NumberOfNodes;
      }
      result[cellNode] = weight;
    }
    return result;
  }

  static constexpr std::array<double, NumberOfNodes * NumberOfNodes> stiffness() {
    constexpr auto                                    Nodes   = Basis::nodes();
    constexpr auto                                    Weights = Basis::weights();
    std::array<double, NumberOfNodes * NumberOfNodes> result{};
    for (int row = 0; row < NumberOfNodes; row++) {
      for (int column = 0; column < NumberOfNodes; column++) {
        result[row * NumberOfNodes + column] = Weights[column] * Basis::derivative(row, Nodes[column]);
      }
    }
    return result;
  }

public:
  std::array<Precision, NumberOfNodes> quadraturePoints  = convert(Basis::nodes());
  std::array<Precision, NumberOfNodes> quadratureWeights = convert(Basis::weights());
  std::array<Precision, NumberOfNodes> inverseQuadratureWeights = [] {
    constexpr auto                       Weights = Basis::weights();
    std::array<Precision, NumberOfNodes> result{};
    for (int node = 0; node < NumberOfNodes; node++) {
      result[node] = static_cast<Precision>(1.0 / Weights[node]);
    }
    return result;
  }();
  std::array<Precision, NodesPerCell> cellQuadratureWeights = convert(cellWeights());
  std::array<Precision, NodesPerCell> inverseCellQuadratureWeights = [] {
    constexpr auto                      Weights = cellWeights();
    std::array<Precision, NodesPerCell> result{};
    for (int cellNode = 0; cellNode < NodesPerCell; cellNode++) {
      result[cellNode] = static_cast<Precision>(1.0 / Weights[cellNode]);
    }
    return result;
  }();
  std::array<Precision, NumberOfNodes * NumberOfNodes> stiffnessOperator = convert(stiffness());
  std::array<Precision, NumberOfNodes * NumberOfNodes> stiffnessOperatorTransposed = [] {
    constexpr auto                                       Stiffness = stiffness();
    std::array<Precision, NumberOfNodes * NumberOfNodes> result{};
    for (int row = 0; row < NumberOfNodes; row++) {
      for (int column = 0; column < NumberOfNodes; column++) {
        result[column * NumberOfNodes + row] = static_cast<Precision>(Stiffness[row * NumberOfNodes + column]);
      }
    }
    return result;
  }();
  std::array<Precision, NumberOfNodes * NumberOfNodes> derivativeOperator = [] {
    constexpr auto                                       Nodes = Basis::nodes();
    std::array<Precision, NumberOfNodes * NumberOfNodes> result{};
    for (int row = 0; row < NumberOfNodes; row++) {
      for (int column = 0; column < NumberOfNodes; column++) {
        result[row * NumberOfNodes + column] = static_cast<Precision>(Basis::derivative(column, Nodes[row]));
      }
    }
    return result;
  }();
  std::array<Precision, NumberOfNodes * NumberOfNodes> inversePicardOperator = [] {
    constexpr auto                                    Stiffness = stiffness();
    std::array<double, NumberOfNodes * NumberOfNodes> picardOperator{};
    for (int row = 0; row < NumberOfNodes; row++) {
      for (int column = 0; column < NumberOfNodes; column++) {
        picardOperator[row * NumberOfNodes + column] = Basis::value(row, 1.0) * Basis::value(column, 1.0)
                                                     - Stiffness[row * NumberOfNodes + column];
      }
    }
    return convert(exahype2::fv::invertMatrix<NumberOfNodes>(picardOperator));
  }();
  std::array<Precision, NumberOfNodes> leftFaceCoefficients = [] {
    std::array<Precision, NumberOfNodes> result{};
    for (int node = 0; node < NumberOfNodes; node++) {
      result[node] = static_cast<Precision>(Basis::value(node, 0.0));
    }
    return result;
  }();
  std::array<Precision, NumberOfNodes> rightFaceCoefficients = [] {
    std::array<Precision, NumberOfNodes> result{};
    for (int node = 0; node < NumberOfNodes; node++) {
      result[node] = static_cast<Precision>(Basis::value(node, 1.0));
    }
    return result;
  }();

  INLINE_METHOD GPU_CALLABLE_METHOD exahype2::dg::ader::BasisTables<Precision> view() const {
    return {
      .quadraturePoints             = quadraturePoints.data(),
      .quadratureWeights            = quadratureWeights.data(),
      .inverseQuadratureWeights     = inverseQuadratureWeights.data(),
      .cellQuadratureWeights        = cellQuadratureWeights.data(),
      .inverseCellQuadratureWeights = inverseCellQuadratureWeights.data(),
      .stiffnessOperator            = stiffnessOperator.data(),
      .stiffnessOperatorTransposed  = stiffnessOperatorTransposed.data(),
      .derivativeOperator           = derivativeOperator.data(),
      .inversePicardOperator        = inversePicardOperator.data(),
      .leftFaceCoefficients         = leftFaceCoefficients.data(),
      .rightFaceCoefficients        = rightFaceCoefficients.data()
    };
  }
};
TARGET_DECLARE_END
