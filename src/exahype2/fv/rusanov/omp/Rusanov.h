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
       *
       * ## Patch data transfer
       *
       * All the data within the CellData object is trivially copyable besides the
       * actual patch sizes and positions. Trivially copyable means that they are
       * plain double arrays. That is, we can map them onto the device. The actual
       * input and output data is stored as pointer to pointers. Here, we have to
       * loop over the entries and copy each individual pointer over separatedly.
       *
       * The positions and sizes of the patches are in principle trivially copyable
       * as well, as they simply host arrays of known sizes. However, it is quite
       * cumbersome to tell OpenMP that this stuff can just be copied. It also might
       * become annoying the compiler decided to add padding bytes. So we copy those
       * guys manually into one large buffer of double values, and then we reconstruct
       * the objects on the device. In this particular context, the resulting overhead
       * is acceptable, as this meta data is not extraordinary large.
       */
      namespace omp {
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
          ::exahype2::CellData&   patchData,
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
          ::exahype2::CellData&   patchData,
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
