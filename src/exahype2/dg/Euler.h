// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_DG_EULER_H_
#define _EXAHYPE2_DG_EULER_H_


#include <vector>
#include <algorithm>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"

#include "DGUtils.h"

namespace exahype2 {
  namespace dg {
    /*
     * Computes the explicit Euler on one cell
     *
     */
    void explicitEulerOnCell(
      const double* __restrict__          					cellQin,
      const int                           					order,
      const int                           					unknowns,
      const int                           					auxiliaryVariables,
			const tarch::la::Vector<Dimensions,double>&		x,
			const double																	dx,
			const double																	t,
			const double																	dt,
      std::function< void(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      ) >   flux,
      std::function< void(
        const double * __restrict__                  Q,
        const double * __restrict__                  dQdx,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      ) >   nonconservativeProduct,
      std::function< void(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double * __restrict__                        S
      ) >   source,
      std::function< void(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        const tarch::la::Vector<Dimensions,double>&  h,
        const tarch::la::Vector<Dimensions,int>&     index,
        double                                       t,
        double * __restrict__                        S
      ) >   pointSource,
			const double* __restrict__										 quadNodes,
			const double* __restrict__										 quadWeights,
			const double* __restrict__                     DerivativeOperator,
      const double* __restrict__                     MassMatrix,
      const double* __restrict__                     StiffnessMatrix,
      const double* __restrict__                     InvertedAndLumpedMassMatrix,
      double* __restrict__                           cellQout
    );
  }
}

#endif
