// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TransferTestData.h"

const double exahype2::dg::unittests::TransferSetup::QuadratureNodes[NodesPerAxis] = {
  0.5 - 0.5 / std::sqrt(3.0),
  0.5 + 0.5 / std::sqrt(3.0)
};
const double exahype2::dg::unittests::TransferSetup::QuadratureWeights[NodesPerAxis] = {0.5, 0.5};

/**
 * Returns a profile that is linear in every coordinate with a slope that depends on the axis and the value.
 */
double
exahype2::dg::unittests::getLinearTransferProfile(const double* const coordinates, const int axes, const int value) {
  double result = 1.0 + value;
  for (int axis = 0; axis < axes; axis++) {
    result += (2.0 + axis + value) * coordinates[axis];
  }
  return result;
}

/**
 * Returns the constant of one value without an exact binary representation.
 */
double
exahype2::dg::unittests::getTransferConstantOf(const int value) {
  return 1.0 / 3.0 + 0.1 * value;
}

/**
 * Writes the coordinates of node in block. A negative block spans the unit interval on every axis.
 * Any other block spans the child interval block. The lowest axis runs fastest in node and in block.
 */
void
exahype2::dg::unittests::setTransferCoordinatesOf(
  double* const coordinates,
  const int     axes,
  const int     node,
  const int     block
) {
  int nodeRemainder  = node;
  int blockRemainder = block;
  for (int axis = 0; axis < axes; axis++) {
    const double nodeCoordinate = TransferSetup::QuadratureNodes[nodeRemainder % TransferSetup::NodesPerAxis];
    coordinates[axis]           = block < 0
      ? nodeCoordinate
      : (blockRemainder % TransferSetup::SubdivisionFactor + nodeCoordinate) / TransferSetup::SubdivisionFactor;
    nodeRemainder /= TransferSetup::NodesPerAxis;
    blockRemainder /= TransferSetup::SubdivisionFactor;
  }
}
