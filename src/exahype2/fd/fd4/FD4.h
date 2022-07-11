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
