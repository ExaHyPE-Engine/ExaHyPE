#pragma once

#include "Functors.h"
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/AoSoALexicographicEnumerator.h"
#include "exahype2/enumerator/SoALexicographicEnumerator.h"
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
        static void computeMaxEigenvalue_LoopBody(
          const double* __restrict__ QIn,
          exahype2::enumerator::AoSLexicographicEnumerator QInEnumerator,
          exahype2::fv::riemann::MaxEigenvalue             maxEigenvalue,
          const tarch::la::Vector<Dimensions, double>&     patchCentre,
          const tarch::la::Vector<Dimensions, double>&     patchSize,
          int                                              patchIndex,
          const tarch::la::Vector<Dimensions, int>&        volumeIndex,
          double                                           t,
          double                                           dt,
          int                                              normal,
          double* __restrict__ QMaxEigenvalue,
          exahype2::enumerator::AoSLexicographicEnumerator QMaxEigenvalueEnumerator
        ) InlineMethod;

        /**
         * Specialisation anticipating that the ordering of the output
         * data will be AoS in almost all the cases. So it makes sense
         * to exploit this knowledge performance-wisely.
         */
        static double reduceMaxEigenvalue_LoopBody(
          const double* __restrict__ QOut,
          exahype2::enumerator::AoSLexicographicEnumerator QOutEnumerator,
          exahype2::fv::riemann::MaxEigenvalue             maxEigenvalue,
          const tarch::la::Vector<Dimensions, double>&     patchCentre,
          const tarch::la::Vector<Dimensions, double>&     patchSize,
          int                                              patchIndex,
          const tarch::la::Vector<Dimensions, int>&        volumeIndex,
          double                                           t,
          double                                           dt
        ) InlineMethod;

        /**
         * This one does not have to be declared as simd. For whatever
         * reason ...
         */
        static void computeFlux_LoopBody(
          const double* __restrict__ QIn,
          exahype2::enumerator::AoSLexicographicEnumerator QInEnumerator,
          exahype2::fv::riemann::Flux                      flux,
          const tarch::la::Vector<Dimensions, double>&     patchCentre,
          const tarch::la::Vector<Dimensions, double>&     patchSize,
          int                                              patchIndex,
          const tarch::la::Vector<Dimensions, int>&        volumeIndex,
          double                                           t,
          double                                           dt,
          int                                              normal,
          double* __restrict__ QFlux,
          exahype2::enumerator::AoSLexicographicEnumerator QFluxEnumerator
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
          const double* __restrict__ tempRiemannX,
          const double* __restrict__ tempRiemannY,
          const double* __restrict__ tempRiemannZ,
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
          exahype2::enumerator::AoSLexicographicEnumerator QInEnumerator,
          exahype2::fv::riemann::SolveRiemannProblem       solveRiemannProblem,
          const double* __restrict__ QFlux,
          exahype2::enumerator::AoSLexicographicEnumerator QFluxEnumerator,
          double* __restrict__ QMaxEigenvalue,
          exahype2::enumerator::AoSLexicographicEnumerator QMaxEigenvalueEnumerator,
          const tarch::la::Vector<Dimensions, double>&     patchCentre,
          const tarch::la::Vector<Dimensions, double>&     patchSize,
          int                                              patchIndex,
          const tarch::la::Vector<Dimensions, int>&        volumeIndex,
          double                                           t,
          double                                           dt,
          int                                              normal,
          double* __restrict__ QS,
          exahype2::enumerator::AoSLexicographicEnumerator QSEnumerator
        )  InlineMethod;

#if defined(SharedOMP) \
  and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(GPUOffloadingOMP))
#pragma omp declare simd
#endif
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
        template <typename QInEnumeratorType, typename QMaxEigenvalueEnumeratorType, typename QOutEnumeratorType>
        static void updateSolutionWithEigenvalueDamping_LoopBody(
          const double* __restrict__ QIn,
          const QInEnumeratorType& QInEnumerator,
          const double* __restrict__ tempMaxEigenvalueX,
          const double* __restrict__ tempMaxEigenvalueY,
          const double* __restrict__ tempMaxEigenvalueZ,
          const QMaxEigenvalueEnumeratorType&          eigenvalueEnumerator,
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
      }
    }
  }
}

#include "LoopBody.cpph"