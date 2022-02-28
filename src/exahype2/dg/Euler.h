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
     * We exploit the fact that we can update all the updates within a cell independently:
     * All the unknowns within a patch can (and do) affect every single unknown in the
     * output, but the output variables do not interact. After all, it is an explicit scheme.
     * Consequently, we can loop over all the unknowns in parallel (if we want).
     *
     * Algorithm:
     *
     * - Copy over in into out. We run an explicit Euler, so the value is updated
     *   according to a formula @f$ Q(t+\Delta t) = Q(t) + \Delta t \cdot F @f$ and
     *   the
     */
    void explicitEulerOnCell_Flux_EvaluateTermsOnce(
      const double* __restrict__                    cellQin,
      const int                                     order,
      const int                                     unknowns,
      const int                                     auxiliaryVariables,
      const tarch::la::Vector<Dimensions,double>&   x,
      const double                                  dx,
      const double                                  t,
      const double                                  dt,
      std::function< void(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      ) >   flux,
      const double* __restrict__                     quadNodes,
      const double* __restrict__                     quadWeights,
      const double* __restrict__                     MassMatrix,
      const double* __restrict__                     StiffnessMatrix,
      double* __restrict__                           cellQout
    );


    /*
     * Computes the explicit Euler on one cell
     *
     * Generic implementation which implements fluxes redundantly
     *
     * @todo Rename
     */
    void explicitEulerOnCell_FluxNCPSource_EvaluateTermsRedundantly(
        const double* __restrict__                                                  cellQin,
        const int                                                                   order,
        const int                                                                   unknowns,
        const int                                                                   auxiliaryVariables,
        const tarch::la::Vector<Dimensions,double>&    x,
        const double                                   dx,
        const double                                   t,
        const double                                   dt,
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
        const double* __restrict__                     quadNodes,
        const double* __restrict__                     quadWeights,
        const double* __restrict__                     MassMatrix,
        const double* __restrict__                     StiffnessMatrix,
        double* __restrict__                           cellQout
    );


    namespace internal {
      /**
       * Copy solution over for one node
       *
       * This method signature is kind of an overkill (we don't need all of the
       * variables), but I decided to keep the internal routines as close to
       * each other as possible.
       */
      void copySolution(
        const double* __restrict__                    cellQin,
        const int                                     order,
        const int                                     unknowns,
        const int                                     auxiliaryVariables,
        const tarch::la::Vector<Dimensions,int>&      node,
        double* __restrict__                          cellQout
      );
    }
  }
}

#endif
