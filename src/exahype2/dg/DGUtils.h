// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Functors.h"
#include "exahype2/fv/PatchUtils.h"
#include "peano4/Loop.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/dataview/FaceMarker.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/Assertions.h"
#include "tarch/la/ScalarOperations.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg {
  template <typename T>
  tarch::la::Vector<DIMENSIONS, double> getQuadraturePoint(
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const tarch::la::Vector<DIMENSIONS, int>&    index,
    const T* NOALIAS                             quadraturePoints
  );

  int getNodesPerCell(int nodesPerAxis);

  tarch::la::Vector<DIMENSIONS, int> getStrides(int nodesPerAxis);

  tarch::la::Vector<DIMENSIONS, int> getIndex(int node, tarch::la::Vector<DIMENSIONS, int> strides);

  void computeGradient(
    const double* NOALIAS QCell,
    const double* NOALIAS derivativeOperator,
    double                invDx,
    int                   nodesPerAxis,
    int                   strideQ,
    int                   scalarIndex,
    double* NOALIAS       gradQ
  );

  std::string plotCell(const double* NOALIAS Q, int order, int unknowns, int auxiliaryVariables);

  template <typename QStoreType>
  QStoreType evaluatePolynomial(
    const peano4::dataview::CellMarker&          marker,
    int                                          order,
    const QStoreType* NOALIAS                    quadratureNodes1D,
    int                                          unknownsPerDoF,
    const QStoreType* NOALIAS                    Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    int                                          unknown
  );

  template <typename QStoreType>
  QStoreType evaluatePolynomial(
    const peano4::dataview::FaceMarker&          marker,
    int                                          order,
    const QStoreType* NOALIAS                    quadratureNodes1D,
    int                                          unknownsPerDoF,
    const QStoreType* NOALIAS                    Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    int                                          unknown
  );

  void copyOneSideOfFaceProjection(
    int                   unknownsPlusAuxiliaryVariables,
    int                   order,
    int                   numberOfProjectedQuantities,
    int                   normal,
    bool                  isRightFaceHalf,
    const double* NOALIAS sourceQ,
    double* NOALIAS       destinationQ
  );
} // namespace exahype2::dg

#include "exahype2/dg/DGUtils.cpph"
