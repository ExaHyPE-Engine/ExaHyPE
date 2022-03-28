// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_RUSANOV_H_
#define _EXAHYPE2_FV_RUSANOV_RUSANOV_H_



#include "Functors.h"
#include "LoopBody.h"

#include "exahype2/CellData.h"


namespace exahype2 {
  namespace fv {
    namespace rusanov {
      /**
       * Apply the Rusanov Riemann solver over a set of patches.
       *
       * Here's the explanation of the implementation details marked via _subscript:
       *
       * _overlap1: This solver assumes that the input data has a halo layer of one.
       *
       * _sequential: This solver runs through the individual patches one by one.
       *   We neither exploit any parallelism, not do we merge the patch traversals.
       *   All the operations on patch 0 are completed, then we trigger all steps on
       *   patch 1, and so forth.
       *
       * _batched: We run through the individual steps of the finite volume update,
       *   and in each step we complete the work on all the patches handed in.
       *
       * There are two variants of the kernels: One that accepts a functor, and one
       * that works with the solver object and involves the static PDE term variants.
       * Both are annotated with Otter pragmas, so you can find out how much parallelism
       * there is.
       *
       *
       * ## Implementation details (signatures)
       *
       * The function is marked as static and put into a header file. I still want to
       * separate the declaration from the definition, so the actual implementation is
       * in a file with the extension cpph. This indicates that it should (logically)
       * be in a cpp file, but to allow the compiler to inline aggressively, it has to
       * be in the header.
       *
       * As I put the routine in a header and as multiple code parts might include it,
       * we often end up with multiple definition errors in Clang. Therefore, I mark the
       * routine as static.
       *
       * Please study the comments on the namespace internal (hosted in LoopBody.h) for
       * further details why certain code parts are written the way they are written.
       * The punchline is that the routines here host the loops, but the actual loop
       * bodies then are separated into routines of their own. We inline them aggressively,
       * so that doesn't come at additional cost. This way, we can re-use the same loop
       * body within different loop implementations, orderings, ...
       *
       *
       * ## Vectorisation
       *
       * I only vectorise the static variants of the routines, and I do so if and only
       * if OpenMP is enabled.
       *
       * Most of the time, I discuss within the individual loop bodies how to vectorise
       * and what things have to be taken into account. That is, the knowledge around
       * optimisation is documented with the corresponding loop bodies. There is one
       * exception:
       *
       * If I evaluate the fluxes, I have to take into account that diagonal volumes
       * don't hold valid data. After all, we only use split or face-normal fluxes, but
       * do not support transversal waves. So we should mask out those diagonal volumes
       * and should not compute anything on them. Indeed, some applications use
       * assertions to ensure that their user functions are not invoked on uninitialised
       * data and then fail. Unfortunately, such a mask stops the vectoriser from doing
       * its job, as the control flow simply becomes too complicated. So I enable the
       * masking if and only if I am not in release mode.
       *
       *
       * ## Memory management
       *
       * See ::exahype2::CellData() for details
       */
      static void timeStepWithRusanov_overlap1_patchwise(
        ::exahype2::CellData&              patchData,
        int                     numberOfVolumesPerAxisInPatch,
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


      static void timeStepWithRusanov_overlap1_batched(
        ::exahype2::CellData&              patchData,
        int                     numberOfVolumesPerAxisInPatch,
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


      template <
        typename Solver,
        typename TempDataEnumerator = exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator
      >
      static void timeStepWithRusanov_overlap1_patchwise(
        ::exahype2::CellData&              patchData,
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


      /**
       *
       *
       * ## Loop collapsing
       *
       * I originally thought I could collapse over the all the loops, as there
       * are no race conditions, but this seems not to work. I have no clue why.
       */
      template <
        typename Solver
      >
      static void timeStepWithRusanov_overlap1_patchwise_in_situ(
        ::exahype2::CellData&              patchData,
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


      template <
        typename Solver
      >
      static void timeStepWithRusanov_overlap1_batched_in_situ(
        ::exahype2::CellData&              patchData,
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


      template <
        typename Solver,
        typename TempDataEnumerator = exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator
      >
      static void timeStepWithRusanov_overlap1_patchwise_split_terms(
        ::exahype2::CellData&              patchData,
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


      /**
       * ## Vectorisation
       *
       * I tried to collapse the inner loop over x and the iteration over the patches.
       * This did not work. It led to a seg fault on Hamilton's AMD EPYC processors.
       */
      template <
        typename Solver,
        typename TempDataEnumerator = exahype2::fv::enumerator::AoSoALexicographicVolumeEnumerator
      >
      static void timeStepWithRusanov_overlap1_batched(
        ::exahype2::CellData&              patchData,
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


#include "Rusanov_Helper.cpph"
#include "Rusanov_overlap1_patchwise_functors.cpph"
#include "Rusanov_overlap1_patchwise_static_calls.cpph"
#include "Rusanov_overlap1_patchwise_in_situ_static_calls.cpph"
#include "Rusanov_overlap1_patchwise_split_terms_static_calls.cpph"
#include "Rusanov_overlap1_batched_functors.cpph"
#include "Rusanov_overlap1_batched_static_calls.cpph"
#include "Rusanov_overlap1_batched_in_situ_static_calls.cpph"


#if defined(GPUOffloadingOMP)
#include "exahype2/fv/rusanov/omp/Rusanov.h"
#endif

#endif
