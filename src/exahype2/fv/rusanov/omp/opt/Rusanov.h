// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_OMP_OPT_RUSANOV_H_
#define _EXAHYPE2_FV_RUSANOV_OMP_OPT_RUSANOV_H_

#include "exahype2/CellData.h"

namespace exahype2 {
  namespace fv {
    namespace rusanov {
      namespace omp::opt {
        static constexpr int MaxNumberOfPossiblePatchEntries = 1024*2*2*2*2; // "|P|_max", magic number for now, such that the kernel benchmarks work

        static int currentOffloadedPatches = 0; // Counter "A", use atomics to increment/decrement

        bool offloadIntoPreallocatedMemory(int numberOfPatchesToOffload);
        int leavePreallocatedMemoryFromOffloading(int numberOfPatchesLeaving);

        template <
          typename Solver,
          int      numberOfVolumesPerAxisInPatch,
          int      haloSize,
          int      unknowns,
          int      auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_patchwise_heap_static_calls(
          int                     targetDevice,
          exahype2::CellData&     patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep
        )
        #if defined(UseManualInlining)
        __attribute__((always_inline))
        #endif
        ;

        template <
          typename Solver,
          int      numberOfVolumesPerAxisInPatch,
          int      haloSize,
          int      unknowns,
          int      auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_batched_heap_static_calls(
          int                     targetDevice,
          exahype2::CellData&     patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep
        )
        #if defined(UseManualInlining)
        __attribute__((always_inline))
        #endif
        ;
      }
    }
  }
}

#include "Rusanov_patchwise_heap_static_calls.cpph"
#include "Rusanov_batched_heap_static_calls.cpph"

#endif
