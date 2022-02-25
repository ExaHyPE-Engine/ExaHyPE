// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_RIEMANN_H_
#define _EXAHYPE2_DG_RIEMANN_H_


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"

#include "DGUtils.h"

namespace exahype2 {
  namespace dg {

  void getMaxEigenvaluePerFace(
      std::function< double(
          const double * __restrict__                 Q,
          const tarch::la::Vector<Dimensions,double>& x,
          double                                      t,
          int                                          normal
          ) >   maxEigenvalue,
      double* __restrict__                     QHull[Dimensions*2],
      const int                                     order,
      const int                                     unknowns,
      const int                                     auxiliaryVariables,
      const tarch::la::Vector<Dimensions,double>&   x,
      const double                                  t,
      double                                        eigenvalues[Dimensions*2]
      );

  void computeRusanovOnHull(
      std::function< void(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      ) >   flux,
      std::function< void(
        const double * __restrict__                  Qinside,
        double * __restrict__                         Qoutside,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal
      ) >   boundaryConditions,
      double*                                         QHullIn[Dimensions*2],
      double* __restrict__                           QHullOut,
      const int                                     order,
      const int                                     unknowns,
      const int                                     auxiliaryVariables,
      const tarch::la::Vector<Dimensions,double>&   x,
      const double                                  dx,
      const double                                  t,
      const double                                  dt,
      const double                                   maxEigenvalueAlongFace[Dimensions*2],
      const bool                                      atBoundary[Dimensions*2]
      );

  void applyRiemannToPatch(
      double* __restrict__         Q,
      const double* __restrict__   QHullIn,
      const int                   order,
      const int                   unknowns,
      const int                   auxiliaryVariables,
      const double                 dx,
      const double                 dt,
      const double* __restrict__  BasisFunctionValuesLeft,
      const double* __restrict__  BasisFunctionValuesRight,
      const double* __restrict__  quadratureWeights
      );

  }
}

#endif
