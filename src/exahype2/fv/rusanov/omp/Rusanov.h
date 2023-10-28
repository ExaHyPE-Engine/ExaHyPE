// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/timing/Measurement.h"

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
       * If you run this routine without GPU offloading, we place all
       * temporary data on the heap. If GPUs are to be used, then we create
       * pointers to the heap, but we map the data explicitly to GPU memory.
       * As we use explicit allocs and frees, the map clauses basically
       * replicate the heap behaviour on the accelerator.
       *
       * I'd like to assign the host pointers null, as the actual temporary data
       * is to be create on the GPU. Such a 'create a pointer which is a
       * nullptr' ensures that the code still compiles on the host even though
       * it is semantically broken as the temporary data is not allocated.
       *
       * However, we have to create the right buffer size on the host,
       * as the NVIDIA tools otherwise complain. On the long term, this should
       * not be necessary.
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
       *
       * If you copy the pointers to pointers from the host to the GPU, you have to
       * be careful. It makes no sense to copy the pointers within the pointer array
       * to the GPU, as these are host pointers. So first deploy the actual data to
       * the GPU per patch. Then we grab the GPU pointer address to which the memory
       * region has been mapped to. We store this data in mappedPointersToQIn. We
       * transfer mappedPointersToQIn to the GPU and then reconstruct there the
       * pointer-to-pointer map that we need within the kernels.
       *
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
        static void timeStepWithRusanov_patchwise_map_static_calls(
          int                     targetDevice,
          ::exahype2::CellData&   patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep,
          tarch::timing::Measurement&  measurement
        )
        #if defined(UseManualInlining)
        __attribute__((always_inline))
        #endif
        ;

        /**
         * @see timeStepWithRusanov_patchwise_map_static_calls()
         */
        template <
          typename Solver,
          int      numberOfVolumesPerAxisInPatch,
          int      haloSize,
          int      unknowns,
          int      auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_batched_map_static_calls(
          int                     targetDevice,
          ::exahype2::CellData&   patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep,
          tarch::timing::Measurement&  measurement
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
        static void timeStepWithRusanov_patchwise_managed_static_calls(
          int                     targetDevice,
          ::exahype2::CellData&   patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep,
          tarch::timing::Measurement&  measurement
        )
        #if defined(UseManualInlining)
        __attribute__((always_inline))
        #endif
        ;

        /**
         * @see timeStepWithRusanov_patchwise_map_static_calls()
         */
        template <
          typename Solver,
          int      numberOfVolumesPerAxisInPatch,
          int      haloSize,
          int      unknowns,
          int      auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_batched_managed_static_calls(
          int                     targetDevice,
          ::exahype2::CellData&   patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep,
          tarch::timing::Measurement&  measurement
        )
        #if defined(UseManualInlining)
        __attribute__((always_inline))
        #endif
        ;

        /**
         * Dummy version delegating to one well-defined proper kernel. By
         * offering such a routine, we can generate generic compute code
         * in Python without knowing which kernel variants are on offer for
         * a particular GPU backend.
         */
        template <
          typename Solver,
          int      numberOfVolumesPerAxisInPatch,
          int      haloSize,
          int      unknowns,
          int      auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_generic_static_calls(
          int                     targetDevice,
          ::exahype2::CellData&   patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep,
          tarch::timing::Measurement&  measurement
        ) {
            timeStepWithRusanov_patchwise_map_static_calls<
              Solver,
              numberOfVolumesPerAxisInPatch,
              haloSize,
              unknowns,
              auxiliaryVariables,
              TempDataEnumerator
            >(
              targetDevice,
              patchData,
              evaluateFlux,
              evaluateNonconservativeProduct,
              evaluateSource,
              evaluateMaximumEigenvalueAfterTimeStep,
              measurement
            );
        }
      }
    }
  }
}

#include "Rusanov_patchwise_map_static_calls.cpph"
#include "Rusanov_batched_map_static_calls.cpph"
#include "Rusanov_patchwise_managed_static_calls.cpph"
#include "Rusanov_batched_managed_static_calls.cpph"
