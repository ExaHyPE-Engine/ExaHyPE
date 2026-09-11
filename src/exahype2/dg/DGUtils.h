// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Functors.h"
#include "exahype2/fv/PatchUtils.h"
#include "peano4/Globals.h"
#include "peano4/Loop.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"
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
    int                                          polynomialOrder,
    const T* NOALIAS                             quadraturePoints
  );

  double getQuadratureWeight(
    const tarch::la::Vector<3, double>& cellSize,
    const tarch::la::Vector<3, int>&    index,
    const double* NOALIAS               quadratureWeights
  );

  int getNodesPerCell(int nodesPerAxis);

  tarch::la::Vector<DIMENSIONS, int> getStrides(int nodesPerAxis);

  tarch::la::Vector<DIMENSIONS, int> getIndex(int node, tarch::la::Vector<DIMENSIONS, int> strides);

  int cellIndexToHullIndex(
    const tarch::la::Vector<DIMENSIONS, int>& indexCell,
    const int                                 direction,
    const int                                 orientation,
    const int                                 nodesPerAxis
  );

  void computeGradient(
    const double* NOALIAS const QCell,
    const double* NOALIAS const derivativeOperator,
    const double                invDx,
    const int                   nodesPerAxis,
    const int                   strideQ,
    const int                   scalarIndex,
    double* NOALIAS             gradQ
  );

  void subtractCell(
    double* NOALIAS       QOut,
    const double* NOALIAS Qsubstract,
    const int             order,
    const int             unknowns,
    const int             auxiliaryVariables
  );

  std::string plotCell(const double* NOALIAS Q, const int order, const int unknowns, const int auxiliaryVariables);

  std::string plotFace(
    const double* NOALIAS Q,
    const int             order,
    const int             unknowns,
    const int             auxiliaryVariables,
    int                   normal,
    int                   numberOfQuantitiesProjectedOntoFace
  );

  template <typename QStoreType>
  QStoreType evaluatePolynomial(
    const peano4::datamanagement::CellMarker&    marker,
    int                                          order,
    const QStoreType* NOALIAS                    QuadratureNodes1D,
    int                                          unknownsPerDoF,
    const QStoreType* NOALIAS                    Q,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    int                                          unknown
  );

  template <typename QStoreType>
  QStoreType evaluatePolynomial(
    const peano4::datamanagement::FaceMarker&    marker,
    int                                          order,
    const QStoreType* NOALIAS                    QuadratureNodes1D,
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
    int                   isRightFaceHalf,
    const double* NOALIAS srcQ,
    double* NOALIAS       destQ
  );
} // namespace exahype2::dg

#include "exahype2/dg/DGUtils.cpph"
