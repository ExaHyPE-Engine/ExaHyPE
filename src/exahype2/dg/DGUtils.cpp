// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DGUtils.h"

/** Copies one side of a DG face projection using the finite-volume halo layout helper. */
void exahype2::dg::copyOneSideOfFaceProjection(
  int                   unknownsPlusAuxiliaryVariables,
  int                   order,
  int                   numberOfProjectedQuantities,
  int                   normal,
  int                   isRightFaceHalf,
  const double* NOALIAS srcQ,
  double* NOALIAS       destQ
) {
  ::exahype2::fv::copyHalfOfHalo(
    unknownsPlusAuxiliaryVariables,
    order + 1,
    numberOfProjectedQuantities,
    normal,
    isRightFaceHalf,
    srcQ,
    destQ
  );
}

/** Returns the tensor-product quadrature weight scaled by the cell size. */
double exahype2::dg::getQuadratureWeight(
  const tarch::la::Vector<3, double>& cellSize,
  const tarch::la::Vector<3, int>&    index,
  const double* NOALIAS               quadratureWeights
) {
  double quadratureWeight = 1.0;
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    quadratureWeight *= cellSize(axis) * quadratureWeights[index(axis)];
  }
  return quadratureWeight;
}

/** Returns nodesPerAxis raised to DIMENSIONS. */
int exahype2::dg::getNodesPerCell(int nodesPerAxis) {
  int nodesPerCell = nodesPerAxis;
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    nodesPerCell *= nodesPerAxis;
  }
  return nodesPerCell;
}

/** Returns the lexicographic node stride for every coordinate axis. */
tarch::la::Vector<DIMENSIONS, int> exahype2::dg::getStrides(int nodesPerAxis) {
  tarch::la::Vector<DIMENSIONS, int> strides(1);
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    strides[axis] = strides[axis - 1] * nodesPerAxis;
  }
  return strides;
}

/** Converts a lexicographically serialised node into its Cartesian index. */
tarch::la::Vector<DIMENSIONS, int> exahype2::dg::getIndex(int node, tarch::la::Vector<DIMENSIONS, int> strides) {
  tarch::la::Vector<DIMENSIONS, int> index(0);
  int                                consumedNodeIndex = 0;
  for (int axis = DIMENSIONS - 1; axis >= 0; axis--) {
    index[axis] = (node - consumedNodeIndex) / strides[axis];
    consumedNodeIndex += strides[axis] * index[axis];
  }
  return index;
}

/**
 * Converts a cell node index into the serialised index on one oriented face.
 * orientation is zero for the lower face and one for the upper face.
 */
int exahype2::dg::cellIndexToHullIndex(
  const tarch::la::Vector<DIMENSIONS, int>& indexCell,
  const int                                 direction,
  const int                                 orientation,
  const int                                 nodesPerAxis
) {
  int hullIndex  = 0;
  int faceStride = 1;

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    if (axis != direction) {
      hullIndex += indexCell[axis] * faceStride;
      faceStride *= nodesPerAxis;
    }
  }

  // The hull stores the two faces of every direction one after another, so the index skips
  // faceStride entries for each preceding face.
  return hullIndex + faceStride * (2 * direction + orientation);
}

/**
 * Evaluates the nodal DG gradient in every coordinate direction.
 *
 * gradQ contains DIMENSIONS consecutive blocks of strideQ values. invDx
 * assumes an isotropic cell and scales the one-dimensional derivative operator.
 */
void exahype2::dg::computeGradient(
  const double* NOALIAS const QCell,
  const double* NOALIAS const derivativeOperator,
  const double                invDx,
  const int                   nodesPerAxis,
  const int                   strideQ,
  const int                   scalarIndex,
  double* NOALIAS             gradQ
) {
  const tarch::la::Vector<DIMENSIONS, int> strides = getStrides(nodesPerAxis);
  const tarch::la::Vector<DIMENSIONS, int> index   = getIndex(scalarIndex, strides);

  for (int axis = 0; axis < DIMENSIONS; axis++) {
    for (int quantity = 0; quantity < strideQ; quantity++) {
      gradQ[axis * strideQ + quantity] = 0.0;
    }

    for (int node = 0; node < nodesPerAxis; node++) {
      const double coefficient = invDx * derivativeOperator[node + nodesPerAxis * index[axis]];
      for (int quantity = 0; quantity < strideQ; quantity++) {
        gradQ[axis * strideQ + quantity] +=
          coefficient * QCell[(scalarIndex + (node - index[axis]) * strides[axis]) * strideQ + quantity];
      }
    }
  }
}

/** Subtracts all unknowns and auxiliary variables of Qsubstract from QOut. */
void exahype2::dg::subtractCell(
  double* NOALIAS       QOut,
  const double* NOALIAS Qsubstract,
  const int             order,
  const int             unknowns,
  const int             auxiliaryVariables
) {
  const int nodesPerAxis = order + 1;
  const int nodesPerCell = getNodesPerCell(nodesPerAxis);
  const int strideQ      = unknowns + auxiliaryVariables;

  for (int node = 0; node < nodesPerCell; node++) {
    for (int quantity = 0; quantity < strideQ; quantity++) {
      QOut[node * strideQ + quantity] -= Qsubstract[node * strideQ + quantity];
    }
  }
}

/** Returns a diagnostic string for a cell in lexicographic node order. */
std::string exahype2::dg::plotCell(
  const double* NOALIAS Q,
  const int             order,
  const int             unknowns,
  const int             auxiliaryVariables
) {
  return ::exahype2::fv::plotPatch(Q, order + 1, unknowns, auxiliaryVariables, DIMENSIONS == 2);
}

/**
 * Returns a diagnostic string for both sides of a projected face.
 * Values remain in their serialised face order; normal determines the spatial layout.
 */
std::string exahype2::dg::plotFace(
  const double* NOALIAS Q,
  const int             order,
  const int             unknowns,
  const int             auxiliaryVariables,
  int                   normal,
  int                   numberOfQuantitiesProjectedOntoFace
) {
  return ::exahype2::fv::plotPatchOverlap(
    Q,
    unknowns,
    auxiliaryVariables,
    order + 1,
    numberOfQuantitiesProjectedOntoFace,
    normal,
    DIMENSIONS == 2
  );
}
