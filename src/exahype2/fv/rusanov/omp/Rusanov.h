// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_OMP_RUSANOV_H_
#define _EXAHYPE2_FV_RUSANOV_OMP_RUSANOV_H_


namespace exahype2 {
  namespace fv {
    namespace rusanov {
      /**
       * The name sequential is a little bit irritating in a GPU context.
       * What it means is that the code runs logically through the patches
       * one by one and issues all kernel on one patch before it continues
       * with the next one. Obviously, the patches are all independent of
       * each other. Therefore, we can deploy this sequence of patch
       * updates to a distributed for over the team.
       *
       * ## Memory management
       *
       * If you run this routine without GPU offloading, we place all temporary data on the heap.
       * If GPUs are to be used, then we create pointer for the temporary data, but we assign
       * them all null, as the actual temporary data is to be create on the GPU. This
       * 'create a pointer which is a nullptr' ensures that the code still compiles on the host
       * even though it is semantically broken as the temporary data is not acquired. However, we
       * want to run on the GPU, so we use the pragma omp enter data instruction to allocate the
       * data directly on the device.
       */
      namespace omp {
        template <
          typename Solver,
          typename TempDataEnumerator = exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator
        >
        static void timeStepWithRusanov_overlap1_patchwise_heap_static_calls(
          int                     targetDevice,
          ::exahype2::CellData&   patchData,
          int                     numberOfVolumesPerAxisInPatch,
          int                     unknowns,
          int                     auxiliaryVariables,
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


#include "Rusanov_overlap1_patchwise_heap_static_calls.cpph"

#endif
