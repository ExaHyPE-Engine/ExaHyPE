// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "exahype2/fd/Functors.h"
#include "exahype2/CellData.h"


#include "../KernelVariants.h"


namespace exahype2 {
  namespace fd {
    namespace fd4 {
      namespace omp {
        template <
          typename Solver,
          int                     numberOfGridCellsPerPatchPerAxis,
          int                     haloSize,
          int                     unknowns,
          int                     auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStep_batched_heap_multicore_static_calls(
          ::exahype2::CellData&   patchData,
          double                  KOSigma,
          bool                    evaluateFlux,
          bool                    evaluateDifferentialSource, //for ncp
          bool                    evaluateAlgebraicSource, //real source
          bool                    copyOldTimeStepAndScaleWithTimeStepSize,
          DifferentialSourceTermVariant variant
        )
        #if defined(UseManualInlining)
        __attribute__((always_inline))
        #endif
        ;
      }
    }
  }
}


#include "FD4_batched_static_calls.cpph"
