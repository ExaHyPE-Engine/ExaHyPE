// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Basis.h"
#include "peano4/CellConnectivity.h"
#include "tarch/Assertions.h"

namespace exahype2::dg::unittests {
  struct TransferSetup;
  struct TransferOperators;

  double getLinearTransferProfile(const double* coordinates, int axes, int value);
  double getTransferConstantOf(int value);
  void setTransferCoordinatesOf(double* coordinates, int axes, int node, int block);

  template <int Nodes>
  INLINE_METHOD void fillLinearTransferProfile(double* values, int axes, int block);

  template <int Nodes>
  INLINE_METHOD void fillTransferConstant(double* values);
} // namespace exahype2::dg::unittests

/**
 * The Gauss-Legendre basis of the transfer tests.
 */
struct exahype2::dg::unittests::TransferSetup final {
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
struct exahype2::dg::unittests::TransferOperators final {
  exahype2::dg::AMRMatrices1D<TransferSetup::NodesPerAxis> prolongation{
    exahype2::dg::transposePerChild<TransferSetup::NodesPerAxis>(
      exahype2::dg::getInterpolationMatrix1D<TransferSetup::NodesPerAxis>(TransferSetup::QuadratureNodes)
    )
  };
  exahype2::dg::AMRMatrices1D<TransferSetup::NodesPerAxis> restriction{
    exahype2::dg::transposePerChild<TransferSetup::NodesPerAxis>(
      exahype2::dg::getWeightedRestrictionMatrix1D<TransferSetup::NodesPerAxis>(
        TransferSetup::QuadratureNodes,
        TransferSetup::QuadratureWeights
      )
    )
  };
};

#include "unittests/exahype2/dg/TransferTestData.cpph"
