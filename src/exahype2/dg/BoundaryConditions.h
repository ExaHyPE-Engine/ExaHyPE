// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace exahype2::dg {
  void applyBoundaryConditions(
    std::function<void(
      const double* NOALIAS                        Qinside,
      double* NOALIAS                              Qoutside,
      const tarch::la::Vector<DIMENSIONS, double>& x,
      double                                       t,
      double                                       dt,
      int                                          normal
    )>                                           boundaryCondition,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       t,
    double                                       dt,
    int                                          order,
    int                                          unknowns,
    int                                          auxiliaryVariables,
    int                                          faceNumber,
    const double* NOALIAS                        quadraturePoints,
    double* NOALIAS                              Q
  );
} // namespace exahype2::dg
