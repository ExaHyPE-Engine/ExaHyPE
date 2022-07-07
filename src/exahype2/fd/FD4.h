// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FD4_H_
#define _EXAHYPE2_FD4_H_


#include "Functors.h"
#include "LoopBody.h"

#include "exahype2/CellData.h"


namespace exahype2 {
  namespace fd {
    /**
     * Fourth-order Finite Differences
     */
    static void timeStepWithFD4_patchwise_heap_functors(
        ::exahype2::CellData&   patchData,
        int                     numberOfVolumesPerAxisInPatch,
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


#include "FD4_Helper.cpph"
#include "FD4_patchwise_functors.cpph"

#endif
