// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "peano4/Partition.h"

namespace exahype2::dg::tests {
  struct TransferSetup;
  struct TransferOperators;

  double getLinearTransferProfile(const double* coordinates, int axes, int value);
  double getTransferConstantOf(int value);
  void setTransferCoordinatesOf(double* coordinates, int axes, int node, int block);

  template <int Nodes>
  void fillLinearTransferProfile(double* values, int axes, int block);

  template <int Nodes>
  void fillTransferConstant(double* values);
} // namespace exahype2::dg::tests

/**
 * The basis of the transfer tests: Two Gauss-Legendre nodes per axis and two values per node.
 */
struct exahype2::dg::tests::TransferSetup final {
  static constexpr int SubdivisionFactor = peano4::SubdivisionFactor;
  static constexpr int ChildrenPerCell   = peano4::ChildrenPerCell;
  static constexpr int NodesPerAxis      = 2;
  static constexpr int ValuesPerNode     = 2;
  static constexpr int FaceNodes         = exahype2::dg::getNodesPerFace(NodesPerAxis);
  static constexpr int CellNodes         = exahype2::dg::getNodesPerCell(NodesPerAxis);

  static const double QuadratureNodes[NodesPerAxis];
  static const double QuadratureWeights[NodesPerAxis];
};

/**
 * The transfer operators of the basis in the layout of the generated tables.
 */
struct exahype2::dg::tests::TransferOperators final {
  exahype2::dg::AMRMatrices1D<TransferSetup::NodesPerAxis> prolongation;
  exahype2::dg::AMRMatrices1D<TransferSetup::NodesPerAxis> restriction;

  TransferOperators();
};

/**
 * Writes the linear profile at the nodes of one block of axes axes: The unit interval on every
 * axis for a negative block, the child interval of block on every axis otherwise.
 */
template <int Nodes>
void
exahype2::dg::tests::fillLinearTransferProfile(double* const values, const int axes, const int block) {
  for (int node = 0; node < Nodes; node++) {
    double coordinates[DIMENSIONS];
    setTransferCoordinatesOf(coordinates, axes, node, block);
    for (int value = 0; value < TransferSetup::ValuesPerNode; value++) {
      values[node * TransferSetup::ValuesPerNode + value] = getLinearTransferProfile(coordinates, axes, value);
    }
  }
}

/**
 * Writes the constant of every value into every node.
 */
template <int Nodes>
void
exahype2::dg::tests::fillTransferConstant(double* const values) {
  for (int node = 0; node < Nodes; node++) {
    for (int value = 0; value < TransferSetup::ValuesPerNode; value++) {
      values[node * TransferSetup::ValuesPerNode + value] = getTransferConstantOf(value);
    }
  }
}
