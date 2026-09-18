// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "BoundaryConditions.h"

#include "exahype2/Enumeration.h"
#include "exahype2/dg/DGUtils.h"
#include "peano4/Loop.h"

/**
 * Fills the exterior trace of one projected DG face.
 *
 * Q contains the interior and exterior overlap layers, and the callback receives each interior
 * trace value and writes the corresponding exterior value. faceNumber follows Peano's face
 * enumeration in [0, 2*DIMENSIONS), and the order+1 quadrature points are reference coordinates
 * on [0, 1].
 */
void exahype2::dg::applyBoundaryConditions(
  const std::function<void(
    const double* NOALIAS                        Qinside,
    double* NOALIAS                              Qoutside,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    double                                       t,
    double                                       dt,
    int                                          normal
  )>                                           boundaryCondition,
  const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
  const tarch::la::Vector<DIMENSIONS, double>& cellSize,
  const double                                 t,
  const double                                 dt,
  const int                                    order,
  const int                                    unknowns,
  const int                                    auxiliaryVariables,
  const int                                    faceNumber,
  const double* NOALIAS                        quadraturePoints,
  double* NOALIAS                              Q
) {
  const int                             normal     = faceNumber % DIMENSIONS;
  const int                             stride     = unknowns + auxiliaryVariables;
  tarch::la::Vector<DIMENSIONS, double> faceOffset = faceCentre - 0.5 * cellSize;
  faceOffset(normal) += 0.5 * cellSize(normal);

  peano4::dForEachExclusive(order + 1, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
    tarch::la::Vector<DIMENSIONS, double> x;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      x(axis) = faceOffset(axis);
      x(axis) += (axis == normal) ? 0.0 : quadraturePoints[dof(axis)] * cellSize(axis);
    }

    tarch::la::Vector<DIMENSIONS, int> insideDoF  = dof;
    tarch::la::Vector<DIMENSIONS, int> outsideDoF = dof;
    if (faceNumber < DIMENSIONS) {
      insideDoF(normal)  = 1;
      outsideDoF(normal) = 0;
    } else {
      insideDoF(normal)  = 0;
      outsideDoF(normal) = 1;
    }

    const int insideDoFSerialised  = exahype2::serialiseVoxelIndexInOverlap(insideDoF, order + 1, 1, normal) * stride;
    const int outsideDoFSerialised = exahype2::serialiseVoxelIndexInOverlap(outsideDoF, order + 1, 1, normal) * stride;

    boundaryCondition(Q + insideDoFSerialised, Q + outsideDoFSerialised, x, t, dt, normal);
  });
}
