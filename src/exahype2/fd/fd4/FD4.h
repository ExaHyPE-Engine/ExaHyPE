// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FD_FD4_H_
#define _EXAHYPE2_FD_FD4_H_


#include "exahype2/fd/Functors.h"
#include "exahype2/fd/LoopBody.h"
#include "exahype2/fd/fd4/LoopBody.h"

#include "exahype2/CellData.h"


namespace exahype2 {
  namespace fd {
    namespace fd4 {
      /**
       * Fourth-order Finite Differences
       *
       * This routine relies on the fact that we have a halo size of 3.
       *
       *
       * @param copyOldTimeStepAndScaleWithTimeStepSize The routine can be used
       *   in two ways: Either for a classic explicit Euler or in a Runge-Kutta
       *   context. For the latter, we evaluate the outcome of the right-hand
       *   side of an ODE formulation @f$ \partial _t Q(t) = F(t) @f$, i.e. we
       *   store the F. In an Euler context, we evaluate F, too, but then
       *   immediately scale it with the time step size and add the original
       *   time step's data. We copy stuff over.
       *
       * @param KOSigma Penalty term for the sixth order derivative. Scales
       *   the Kreiss Oliger dissipation. If you set it to 0.0, the term is not
       *   evaluated at all.
       */
      static void timeStep_patchwise_heap_functors(
        ::exahype2::CellData&   patchData,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables,
        double                  KOSigma,
        bool                    evaluateFlux,
        bool                    evaluateDifferentialSource, //for ncp
        bool                    evaluateAlgebraicSource, //real source
        bool                    evaluateMaximumEigenvalueAfterTimeStep,
        bool                    copyOldTimeStepAndScaleWithTimeStepSize,
        Flux                    flux,
        NonconservativeProduct  DifferentialSource,
        Source                  AlgebraicSource,
        MaxEigenvalue           maxEigenvalue
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;
    }
  }
}


#include "exahype2/fd/FD_Helper.cpph"
#include "FD4_patchwise_functors.cpph"

#endif
