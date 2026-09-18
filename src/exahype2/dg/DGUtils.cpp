// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DGUtils.h"

/** Copies one half of a face projection from sourceQ into destinationQ. */
void exahype2::dg::copyOneSideOfFaceProjection(
  const int                   unknownsPlusAuxiliaryVariables,
  const int                   order,
  const int                   numberOfProjectedQuantities,
  const int                   normal,
  const bool                  isRightFaceHalf,
  const double* NOALIAS const sourceQ,
  double* NOALIAS const       destinationQ
) {
  assertion(order >= 0, order);
  assertion(normal >= 0 and normal < DIMENSIONS, normal);

  ::exahype2::fv::copyHalfOfHalo(
    unknownsPlusAuxiliaryVariables,
    order + 1,
    numberOfProjectedQuantities,
    normal,
    isRightFaceHalf,
    sourceQ,
    destinationQ
  );
}

/** Returns nodesPerAxis raised to DIMENSIONS. */
int exahype2::dg::getNodesPerCell(const int nodesPerAxis) {
  assertion(nodesPerAxis > 0, nodesPerAxis);

  int nodesPerCell = nodesPerAxis;
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    nodesPerCell *= nodesPerAxis;
  }
  return nodesPerCell;
}

/** Returns the lexicographic node stride for every coordinate axis. */
tarch::la::Vector<DIMENSIONS, int> exahype2::dg::getStrides(const int nodesPerAxis) {
  assertion(nodesPerAxis > 0, nodesPerAxis);

  tarch::la::Vector<DIMENSIONS, int> strides(1);
  for (int axis = 1; axis < DIMENSIONS; axis++) {
    strides[axis] = strides[axis - 1] * nodesPerAxis;
  }
  return strides;
}

/** Converts a lexicographically serialised node into its Cartesian index. */
tarch::la::Vector<DIMENSIONS, int> exahype2::dg::getIndex(
  const int                                node,
  const tarch::la::Vector<DIMENSIONS, int> strides
) {
  assertion(node >= 0, node);

  tarch::la::Vector<DIMENSIONS, int> index(0);
  int                                consumedNodeIndex = 0;
  for (int axis = DIMENSIONS - 1; axis >= 0; axis--) {
    index[axis] = (node - consumedNodeIndex) / strides[axis];
    consumedNodeIndex += strides[axis] * index[axis];
  }
  return index;
}

/**
 * Evaluates the nodal DG gradient in every coordinate direction.
 *
 * gradQ contains DIMENSIONS consecutive blocks of strideQ values. invDx scales the
 * derivative operator along every axis, so the caller passes a cell of equal width
 * in every direction.
 */
void exahype2::dg::computeGradient(
  const double* NOALIAS const QCell,
  const double* NOALIAS const derivativeOperator,
  const double                invDx,
  const int                   nodesPerAxis,
  const int                   strideQ,
  const int                   scalarIndex,
  double* NOALIAS const       gradQ
) {
  assertion(invDx > 0.0, invDx);
  assertion(nodesPerAxis > 0, nodesPerAxis);
  assertion(strideQ > 0, strideQ);
  assertion(scalarIndex >= 0, scalarIndex);

  const tarch::la::Vector<DIMENSIONS, int> strides = exahype2::dg::getStrides(nodesPerAxis);
  const tarch::la::Vector<DIMENSIONS, int> index   = exahype2::dg::getIndex(scalarIndex, strides);

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

/** Returns a diagnostic string for a cell in lexicographic node order. */
std::string exahype2::dg::plotCell(
  const double* NOALIAS const Q,
  const int                   order,
  const int                   unknowns,
  const int                   auxiliaryVariables
) {
  assertion(order >= 0, order);

  return ::exahype2::fv::plotPatch(Q, order + 1, unknowns, auxiliaryVariables, DIMENSIONS == 2);
}
