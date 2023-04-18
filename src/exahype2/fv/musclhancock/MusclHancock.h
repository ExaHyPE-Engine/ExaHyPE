// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "Functors.h"
#include "LoopBody.h"

#include "exahype2/CellData.h"


namespace exahype2 {
  namespace fv {
    namespace musclhancock {
      static void timeStepWithMusclHancock_patchwise_heap_functors(
        ::exahype2::CellData&   patchData,
        int                     numberOfVolumesPerAxisInPatch,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables,
        bool                    evaluateFlux,
        bool                    evaluateNonconservativeProduct,
        bool                    evaluateSource,
        bool                    evaluateMaximumEigenvalueAfterTimeStep,
        Flux                    flux,
        NonconservativeProduct  nonconservativeProduct,
        Source                  source,
        MaxEigenvalue           maxEigenvalue
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;
    }
  }
}


#include "MusclHancock_Helper.cpph"
#include "MusclHancock_patchwise_functors.cpph"


