// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_SYCL_RUSANOV_H_
#define _EXAHYPE2_FV_RUSANOV_SYCL_RUSANOV_H_


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
       * If you run this routine without GPU offloading, i.e. if you use it
       * non-GPU cousin, we place all temporary data on the heap. If you use
       * the SYCL variant, we explicitly alloc the temp memory on the device.
       *
       * Once the kernel has finished, we have to free all temporary data. We
       * use sycl::free() to delete our scratchpads.
       *
       * With SYCL, we can copy the enumerators directly to the GPU, as they
       * provide standard constructors.
       *
       * The copying of the actual patch data is more complicated, as we
       * have pointers over pointers here. We use the GPUCellData wrapper
       * in the sycl namespace to realise these deep copies.
       *
       *
       * ## SYCL details
       *
       * SYCL required std::size_t for all of its ranges. We store the ranges
       * implicitly in the patch description as integer. When you try to pass
       * in the integers into the SYCL routines, C++ likely will complain with
       *
       *      error: non-constant-expression cannot be narrowed from type 'int' to 'size_t' (aka 'unsigned long') in initializer list [-Wc++11-narrowing]
       *
       * If I explicitly convert the variable into a size_t before, then
       * everything is fine. We can do this here, as we know what we are doing.
       * This is also the reason why I have remodelled the template signature
       * which now uses std::size_t rather than int (if you compare the
       * template arguments to the non-GPU version).
       *
       * By default, SYCL queues execute kernel functions in an out-of-order
       * fashion based on dependency information. Therefore, we have to
       * wait once the kernels have terminated.
       *
       * I try to stick with ranges where possible and to avoid nd_range.
       * Whenever I do not need things like grain sizes or a substructuring
       * of iteration spaces, I avoid it. I keep stuff as simple as possible.
       *
       * ## Syntax
       *
       * As we work in a subnamespace sycl of the Rusanov solvers, any
       * reference to a namespace sycl will make C++ search for something
       * within our rusanov::sycl namespace. Therefore, we have to qualify
       * the namespace absolutely with ::sycl if we want to use something
       * from the standard.
       *
       *
       *
       */
      namespace sycl {
        template <
          typename Solver,
          std::size_t  numberOfVolumesPerAxisInPatch,
          std::size_t  haloSize,
          std::size_t  unknowns,
          std::size_t  auxiliaryVariables,
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
          std::size_t  numberOfVolumesPerAxisInPatch,
          std::size_t  haloSize,
          std::size_t  unknowns,
          std::size_t  auxiliaryVariables,
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

        namespace helper {
          #if defined(GPUOffloadingOMP)
          /**
           * On the GPU, we don't need any of exahype2::CellData's fancy routines or
           * dynamic data management. We simply have to be able to create a struct
           * on the accelerator, which we then befill with the required pointers
           * which we copy over from the host.
           *
           * Therefore, this class is a mere copy of exahype2::CellData without any
           * of its routines. The only different to the host variant (from a data
           * field point of view) is that numberOfCells is not const.
           */
          #pragma omp declare target
          struct OpenMPGPUCellData {
            double**   QIn;
            tarch::la::Vector<Dimensions,double>*   patchCentre;
            tarch::la::Vector<Dimensions,double>*   patchSize;
            double*    t;
            double*    dt;
            int        numberOfCells;
            double**   QOut;
            double*    maxEigenvalue;
          };
          #pragma omp end declare target
          #endif
        }
      }
    }
  }
}


#include "Rusanov_patchwise_heap_static_calls.cpph"
#include "Rusanov_batched_heap_static_calls.cpph"

#endif
