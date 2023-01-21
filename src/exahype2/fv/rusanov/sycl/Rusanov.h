// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once




namespace exahype2 {
  namespace fv {
    namespace rusanov {
      /**
       * Read the docu page "Rusanov ExaHyPE's Rusanov kernel implementation"
       * (from Rusanov.h) before you continue. I also recommend that you study
       * timeStepWithRusanov_batched_heap_static_calls() first.
       *
       * The realisation is thus very similar to timeStepWithRusanov_batched_heap_static_calls()
       * with one decisive difference: We do not build up a task graph for
       * the individual logical steps, but we build up a task graph per
       * patch.
       *
       *
       * ## Comparison to OpenMP
       *
       * In the OpenMP world, I use nested parallelism to realise this
       * patch-wise processing. While I tried this out with SYCL (and wasn't
       * successful as I did hit some compiler issues), it turns out that the
       * SYCL approach is way more elegant with its DAG on the device.
       *
       *
       * ## Implementation details
       *
       * We hold a set of vectors over events which represent the key stages
       * of the algorithm per patch. We wait for these guys at the end of the
       * algorithm. An exception is the copySolutionEvents. We don't have to
       * wait for these, as the kernels behind updateWithEigenvalueEvent already
       * depend on them.
       *
       * So our realisation is, in principle, very simple: Each large logical
       * step (cmp. the batched variant) is formalised by a vector of events.
       * As any compute kernels depends only on events defined over its batch,
       * the kernels are, in principle, totally independent of each other and we
       * thus may speak of a patch-wise processing.
       *
       * Within each logical step, we have additional substeps. The update due
       * to the flux for example first requires us to compute the fluxes in x,
       * y and z direction. Here, I have to be very careful: Again, I model
       * the steps as events. Even though they are internal events, I also have
       * to store them in a global vector, as they have to remain persistent,
       * aka in-memory, until the final update kernel has terminated. I originally
       * modelled them as local variables, but then they are destroyed at the
       * end of the scope block, and we get a memory error.
       *
       * An alternative to the ``store them in a vector'' approach would be to
       * use smart pointers. But I don't like smart pointers.
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
        static void timeStepWithRusanov_taskgraph_static_calls(
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


        /**
         * Implementation of the Rusanov which builds up a task graph which consists of the
         * logical steps of a Rusanov solver. So we have a step to copy the data into the target,
         * then one to compute the eigenvalues along the x-direction, then one along the y-direction,
         * then one to update with the eigenvalue damping (which depends on all previous steps), and
         * so forth.
         *
         * Each individual step runs over all patches handed in. So the
         * individual steps are relatively heavy, while the DAG exclusively
         * represents logical steps of the underlying numerical scheme. It is a
         * (trivial) sequence of steps. I call the approach batched, as each
         * individual logical step in the task graph runs over the batch of patches.
         *
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
         */
        template <
          typename Solver,
          std::size_t  numberOfVolumesPerAxisInPatch,
          std::size_t  haloSize,
          std::size_t  unknowns,
          std::size_t  auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_batched_static_calls(
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


        /**
         * 1:1 translation of the numerical scheme.
         *
         * We grab the SYCL queue that we want to use (Peano puts one SYCL
         * queue on each available device and enumerates them logically
         * starting from 0). Next, we create enumerators which are simply
         * classes that translate multidimensional indices into an integer.
         * As we use enumerators which are template arguments, we can
         * quickly exchange SoA with AoS for example. Once the enumerators
         * are in place, we allocate the memory for all temporary data on
         * the device.
         *
         * This implementation is called patch-wise. This means that we have
         * a big outer loop which runs over all the patches. If we had a
         * sequential accelerator, this would handle one patch after the
         * other, i.e. process the patches patch-wisely. As these loop steps
         * are independent of each other, we can use a parallel for.
         *
         * Per patch, we then run through the numerical steps and issue
         * (nested) parallel fors.
         *
         * ## Flaws
         *
         * - We would like to use higher-dimensional range loops.
         * - This version seems to crash from time to time. I assume it is
         *   due to the fact that the parallel fors have different range
         *   dimensions.
         *
         * @see timeStepWithRusanov_batched_heap_static_calls() for further
         *   SYCL-relevant details.
         */
        template <
          typename Solver,
          std::size_t  numberOfVolumesPerAxisInPatch,
          std::size_t  haloSize,
          std::size_t  unknowns,
          std::size_t  auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_patchwise_static_calls(
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


        /**
         * Dummy version delegating to one well-defined proper kernel. By
         * offering such a routine, we can generate generic compute code
         * in Python without knowing which kernel variants are on offer for
         * a particular GPU backend.
         */
        template <
          typename Solver,
          std::size_t  numberOfVolumesPerAxisInPatch,
          std::size_t  haloSize,
          std::size_t  unknowns,
          std::size_t  auxiliaryVariables,
          typename TempDataEnumerator
        >
        static void timeStepWithRusanov_generic_static_calls(
          int                     targetDevice,
          ::exahype2::CellData&   patchData,
          bool                    evaluateFlux,
          bool                    evaluateNonconservativeProduct,
          bool                    evaluateSource,
          bool                    evaluateMaximumEigenvalueAfterTimeStep
        ) {
          timeStepWithRusanov_taskgraph_static_calls<
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
            evaluateMaximumEigenvalueAfterTimeStep
          );
        }
      }
    }
  }
}


#include "Rusanov_taskgraph_static_calls.cpph"
#include "Rusanov_batched_static_calls.cpph"
#include "Rusanov_patchwise_static_calls.cpph"

