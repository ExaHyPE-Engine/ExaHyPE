#pragma once

#include "Functors.h"
#include "exahype2/enumerators/AoSLexicographicEnumerator.h"
#include "exahype2/enumerators/AoSoALexicographicEnumerator.h"
#include "exahype2/enumerators/SoALexicographicEnumerator.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/multicore/multicore.h"

namespace exahype2 {
  namespace fv {
    namespace riemann {
      constexpr int PickAllEntriesFromOutputVector = -1;

      namespace internal {
#if defined(SharedOMP) \
and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
#pragma omp declare simd
#endif
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void copySolution_LoopBody(
          const double* __restrict__ QIn,
          const QInEnumeratorType&                  QInEnumerator,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions, int>& volumeIndex,
          int                                       unknown,
          double* __restrict__ QOut,
          const QOutEnumeratorType& QOutEnumerator
        ) InlineMethod;
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

        /**
         * Specialisation if input data is already in AoS format. This holds for
         * all of our kernels unless you explicitly reorder the input.
         */
        static void computeEigenvalues_LoopBody(
          const double* __restrict__ QIn,
          exahype2::enumerators::AoSLexicographicEnumerator QInEnumerator,
          exahype2::fv::riemann::Eigenvalues               eigenvalues,
          const tarch::la::Vector<Dimensions, double>&     patchCentre,
          const tarch::la::Vector<Dimensions, double>&     patchSize,
          int                                              patchIndex,
          const tarch::la::Vector<Dimensions, int>&        volumeIndex,
          double                                           t,
          double                                           dt,
          int                                              normal,
          double* __restrict__ QEigenvalues,
          exahype2::enumerators::AoSLexicographicEnumerator QEigenvaluesEnumerator
        ) InlineMethod;

        /**
         * Specialisation anticipating that the ordering of the output
         * data will be AoS in almost all the cases. So it makes sense
         * to exploit this knowledge performance-wisely.
         */
        static double reduceMaxEigenvalue_LoopBody(
          const double* __restrict__ QOut,
          exahype2::enumerators::AoSLexicographicEnumerator QOutEnumerator,
          const double* __restrict__ tempMaxEigenvalueX,
          const double* __restrict__ tempMaxEigenvalueY,
          const double* __restrict__ tempMaxEigenvalueZ,
          exahype2::enumerators::AoSLexicographicEnumerator maxEigenvalueEnumerator,
          const tarch::la::Vector<Dimensions, double>&     patchCentre,
          const tarch::la::Vector<Dimensions, double>&     patchSize,
          int                                              patchIndex,
          const tarch::la::Vector<Dimensions, int>&        volumeIndex,
          double                                           t,
          double                                           dt,
          int                                              unknowns
        ) InlineMethod;

        /**
         * This one does not have to be declared as simd. For whatever
         * reason ...
         */
        static void computeFlux_LoopBody(
          const double* __restrict__ QIn,
          exahype2::enumerators::AoSLexicographicEnumerator QInEnumerator,
          exahype2::fv::riemann::Flux                      flux,
          const tarch::la::Vector<Dimensions, double>&     patchCentre,
          const tarch::la::Vector<Dimensions, double>&     patchSize,
          int                                              patchIndex,
          const tarch::la::Vector<Dimensions, int>&        volumeIndex,
          double                                           t,
          double                                           dt,
          int                                              normal,
          double* __restrict__ QFlux,
          exahype2::enumerators::AoSLexicographicEnumerator QFluxEnumerator
        ) InlineMethod;

#if defined(SharedOMP) \
  and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
#pragma omp declare simd
#endif
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
        template <typename QRiemannEnumeratorType, typename QOutEnumeratorType>
        static void updateSolutionWithRiemannSolution_LoopBody(
          const double* __restrict__ tempLeftUpdatesX,
          const double* __restrict__ tempRightUpdatesX,
          const double* __restrict__ tempBelowUpdatesY,
          const double* __restrict__ tempAboveUpdatesY,
          const double* __restrict__ tempBackwardsUpdatesZ,
          const double* __restrict__ tempForwardsUpdatesZ,
          const QRiemannEnumeratorType&                riemannEnumerator,
          const tarch::la::Vector<Dimensions, double>& patchCentre,
          const tarch::la::Vector<Dimensions, double>& patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions, int>&    volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* __restrict__ QOut,
          const QOutEnumeratorType& QOutEnumerator
        ) InlineMethod;
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

        static void computeRiemannSolution_LoopBody(
          const double* __restrict__ QIn,
          exahype2::enumerators::AoSLexicographicEnumerator QInEnumerator,
          exahype2::fv::riemann::SolveRiemannProblem       solveRiemannProblem,
          const double* __restrict__ QFlux,
          exahype2::enumerators::AoSLexicographicEnumerator QFluxEnumerator,
          const double* __restrict__ QEigenvalues,
          exahype2::enumerators::AoSLexicographicEnumerator QEigenvaluesEnumerator,
          const tarch::la::Vector<Dimensions, double>&     patchCentre,
          const tarch::la::Vector<Dimensions, double>&     patchSize,
          int                                              patchIndex,
          const tarch::la::Vector<Dimensions, int>&        volumeIndex,
          double                                           t,
          double                                           dt,
          int                                              normal,
          double* __restrict__ APDQ,
          double* __restrict__ AMDQ,
          exahype2::enumerators::AoSLexicographicEnumerator DQEnumerator,
          double* __restrict__ QMaxEigenvalue,
          exahype2::enumerators::AoSLexicographicEnumerator QMaxEigenvalueEnumerator
        )  InlineMethod;
      }
    }
  }
}

#include "LoopBody.cpph"