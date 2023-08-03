#pragma once

#include "Functors.h"
#include "LoopBody.h"
#include "Riemann_Helper.cpph"

#include "exahype2/CellData.h"
namespace exahype2 {
  namespace fv {
    namespace riemann {
        static void timeStepWithRiemann_patchwise_heap_functors(
        ::exahype2::CellData&   patchData,
        int                     numberOfVolumesPerAxisInPatch,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables,
        bool                    evaluateFlux,
        bool                    evaluateRiemann,
        bool                    evaluateMaximumEigenvalueAfterTimeStep,
        Flux                    flux,
        Eigenvalues             eigenvalues,
        SolveRiemannProblem     solveRiemannProblem
      )
      #if defined(UseManualInlining)
      __attribute__((always_inline))
      #endif
      ;
    }
  }
}

#include "Riemann_patchwise_functors.cpph"
