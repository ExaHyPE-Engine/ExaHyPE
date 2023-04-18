// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "Functors.h"

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/SoALexicographicEnumerator.h"
#include "exahype2/enumerator/AoSoALexicographicEnumerator.h"

#include "tarch/multicore/multicore.h"


namespace exahype2 {
  namespace fv {
    namespace musclhancock {
      constexpr int PickAllEntriesFromOutputVector = -1;

      namespace internal {
        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        #if defined(GPUOffloadingOMP)
        #pragma omp declare target
        #endif
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void copySolution_LoopBody(
          const double* __restrict__                QIn,
          const QInEnumeratorType&                  QInEnumerator,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          int                                       unknown,
          double* __restrict__                      QOut,
          const QOutEnumeratorType&                 QOutEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif

        template <class QInEnumeratorType, class QInterEnumeratorType>
        void computeTimeDerivative_LoopBody(
            const double* __restrict__                          QIn,
            const QInEnumeratorType                             QInEnumerator,
            exahype2::fv::musclhancock::Flux                    flux,
            exahype2::fv::musclhancock::NonconservativeProduct  ncp,
            exahype2::fv::musclhancock::Source                  source,
            const tarch::la::Vector<Dimensions,double>&         patchCentre,
            const tarch::la::Vector<Dimensions,double>&         patchSize,
            int                                                 patchIndex,
            const tarch::la::Vector<Dimensions,int>&            volumeIndex,
            double                                              t,
            double                                              dt,
            double* __restrict__                                timederivative,
            QInterEnumeratorType                                QInterEnumerator
        ) InlineMethod; 

        template <class QInEnumeratorType, class QInterEnumeratorType>
        void computeQonFace_LoopBody(
            const double* __restrict__                   QIn,
            const QInEnumeratorType                      QInEnumerator,
            const tarch::la::Vector<Dimensions,double>&  patchCentre,
            const tarch::la::Vector<Dimensions,double>&  patchSize,
            int                                          patchIndex,
            const tarch::la::Vector<Dimensions,int>&     volumeIndex,
            double                                       t,
            double                                       dt,
            double* __restrict__                         timederivative,
            double* __restrict__                         QfaceXneg,
            double* __restrict__                         QfaceXpos,
            double* __restrict__                         QfaceYneg,
            double* __restrict__                         QfaceYpos,
            double* __restrict__                         QfaceZneg,
            double* __restrict__                         QfaceZpos,
            QInterEnumeratorType                         QInterEnumerator
        ) InlineMethod;

        template <class QInEnumeratorType, class QMaxEigenvalueEnumeratorType>
        static void computeMaxEigenvalue_LoopBody(
          const double* __restrict__                   QIn,
          QInEnumeratorType                            QInEnumerator,
          exahype2::fv::musclhancock::MaxEigenvalue         maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double* __restrict__                         QMaxEigenvalue,
          QMaxEigenvalueEnumeratorType                 QMaxEigenvalueEnumerator
        )  InlineMethod;

        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        #if defined(GPUOffloadingOMP)
        #pragma omp declare target
        #endif
        template <typename QInEnumeratorType, typename QInterEnumeratorType, typename QMaxEigenvalueEnumeratorType, typename QOutEnumeratorType>
        static void updateSolutionWithEigenvalueDamping_LoopBody(
          const double* __restrict__                   QIn,
          const QInEnumeratorType                      QInEnumerator,
          const double* __restrict__                   tempMaxEigenvalueX,
          const double* __restrict__                   tempMaxEigenvalueY,
          const double* __restrict__                   tempMaxEigenvalueZ,
          const QMaxEigenvalueEnumeratorType &         eigenvalueEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* __restrict__                         QOut,
          const QOutEnumeratorType &                   QOutEnumerator,
          double* __restrict__                         QfaceXneg,
          double* __restrict__                         QfaceXpos,
          double* __restrict__                         QfaceYneg,
          double* __restrict__                         QfaceYpos,
          double* __restrict__                         QfaceZneg,
          double* __restrict__                         QfaceZpos,
          QInterEnumeratorType                         QInterEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif

        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        template <class QInEnumeratorType, class QInterEnumeratorType, class QFluxEnumeratorType>
        static void computeFlux_LoopBody(
          double* __restrict__                         QfaceNLeft,
          double* __restrict__                         QfaceNRight,
          QInterEnumeratorType                         QInterEnumerator,
          QInEnumeratorType                            QInEnumerator,
          exahype2::fv::musclhancock::Flux         flux,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                              dt,
          int                                 normal,
          double* __restrict__                QFluxL,
          double* __restrict__                QFluxR,
          QFluxEnumeratorType                 QFluxEnumerator

        ) InlineMethod;

        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        #if defined(GPUOffloadingOMP)
        #pragma omp declare target
        #endif
        template <typename QFluxEnumeratorType, typename QOutEnumeratorType>
        static void updateSolutionWithFlux_LoopBody(
          const double* __restrict__                   tempFluxXL,
          const double* __restrict__                   tempFluxYL,
          const double* __restrict__                   tempFluxZL,
          const double* __restrict__                   tempFluxXR,
          const double* __restrict__                   tempFluxYR,
          const double* __restrict__                   tempFluxZR,
          const QFluxEnumeratorType                   fluxEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* __restrict__                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif

        template <class QInEnumeratorType, class QInterEnumeratorType, class QNCPFaceEnumeratorType>
        static void computeDTerm_LoopBody(
            double* __restrict__                           QfaceNLeft,
            double* __restrict__                           QfaceNRight,
            QInterEnumeratorType                           QInterEnumerator,
            QInEnumeratorType                              QInEnumerator,
            exahype2::fv::musclhancock::NonconservativeProduct  ncp,
            const tarch::la::Vector<Dimensions,double>&    patchCentre,
            const tarch::la::Vector<Dimensions,double>&    patchSize,
            int                                            patchIndex,
            const tarch::la::Vector<Dimensions,int>&       volumeIndex,
            double                                         t,
            double                                         dt,
            int                                            normal,
            double* __restrict__                           QD,
            const QNCPFaceEnumeratorType                   QNcpEnumerator
        ) InlineMethod;

        #if defined(SharedOMP) and (!defined(__INTEL_LLVM_COMPILER) or !defined(GPUOffloadingOMP))
        #pragma omp declare simd
        #endif
        #if defined(GPUOffloadingOMP)
        #pragma omp declare target
        #endif
        template <typename QNCPFaceEnumeratorType, typename QOutEnumeratorType>
        static void updateSolutionWithDTerm_LoopBody(
          const double* __restrict__                   QDX,
          const double* __restrict__                   QDY,
          const double* __restrict__                   QDZ,
          const QNCPFaceEnumeratorType                 ncpEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* __restrict__                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif

        template <class QInEnumeratorType, class QInterEnumeratorType, class QOutEnumeratorType>
        void updateSolutionwithNCPandSource_LoopBody(
          const double* __restrict__                   QIn,
          const QInterEnumeratorType                   QInterEnumerator,
          const QInEnumeratorType                     QInEnumerator,
          exahype2::fv::musclhancock::NonconservativeProduct  ncp,
          exahype2::fv::musclhancock::Source                source,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          double*                                      timeDerivative,
          double* __restrict__                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator,
          bool                                         evalNCP,
          bool                                         evalSRC
        ) InlineMethod;

        template <class QInEnumeratorType>
        static double reduceMaxEigenvalue_LoopBody(
          const double* __restrict__                   QIn,
          QInEnumeratorType                            QInEnumerator,
          exahype2::fv::musclhancock::MaxEigenvalue         maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        ) InlineMethod;
      }
    }
  }
}


#include "LoopBody.cpph"

