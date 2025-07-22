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
          const double* NOALIAS                QIn,
          const QInEnumeratorType&                  QInEnumerator,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          int                                       unknown,
          double* NOALIAS                      QOut,
          const QOutEnumeratorType&                 QOutEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif

        template <class QInEnumeratorType, class QInterEnumeratorType>
        void computeTimeDerivative_LoopBody(
            const double* NOALIAS                          QIn,
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
            double* NOALIAS                                timederivative,
            QInterEnumeratorType                                QInterEnumerator
        ) InlineMethod; 

        template <class QInEnumeratorType, class QInterEnumeratorType>
        void computeQonFace_LoopBody(
            const double* NOALIAS                   QIn,
            const QInEnumeratorType                      QInEnumerator,
            const tarch::la::Vector<Dimensions,double>&  patchCentre,
            const tarch::la::Vector<Dimensions,double>&  patchSize,
            int                                          patchIndex,
            const tarch::la::Vector<Dimensions,int>&     volumeIndex,
            double                                       t,
            double                                       dt,
            double* NOALIAS                         timederivative,
            double* NOALIAS                         QfaceXneg,
            double* NOALIAS                         QfaceXpos,
            double* NOALIAS                         QfaceYneg,
            double* NOALIAS                         QfaceYpos,
            double* NOALIAS                         QfaceZneg,
            double* NOALIAS                         QfaceZpos,
            QInterEnumeratorType                         QInterEnumerator
        ) InlineMethod;

        template <class QInEnumeratorType, class QMaxEigenvalueEnumeratorType>
        static void computeMaxEigenvalue_LoopBody(
          const double* NOALIAS                   QIn,
          QInEnumeratorType                            QInEnumerator,
          exahype2::fv::musclhancock::MaxEigenvalue         maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double* NOALIAS                         QMaxEigenvalue,
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
          const double* NOALIAS                   QIn,
          const QInEnumeratorType                      QInEnumerator,
          const double* NOALIAS                   tempMaxEigenvalueX,
          const double* NOALIAS                   tempMaxEigenvalueY,
          const double* NOALIAS                   tempMaxEigenvalueZ,
          const QMaxEigenvalueEnumeratorType &         eigenvalueEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* NOALIAS                         QOut,
          const QOutEnumeratorType &                   QOutEnumerator,
          double* NOALIAS                         QfaceXneg,
          double* NOALIAS                         QfaceXpos,
          double* NOALIAS                         QfaceYneg,
          double* NOALIAS                         QfaceYpos,
          double* NOALIAS                         QfaceZneg,
          double* NOALIAS                         QfaceZpos,
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
          double* NOALIAS                         QfaceNLeft,
          double* NOALIAS                         QfaceNRight,
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
          double* NOALIAS                QFluxL,
          double* NOALIAS                QFluxR,
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
          const double* NOALIAS                   tempFluxXL,
          const double* NOALIAS                   tempFluxYL,
          const double* NOALIAS                   tempFluxZL,
          const double* NOALIAS                   tempFluxXR,
          const double* NOALIAS                   tempFluxYR,
          const double* NOALIAS                   tempFluxZR,
          const QFluxEnumeratorType                   fluxEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* NOALIAS                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif

        template <class QInEnumeratorType, class QInterEnumeratorType, class QNCPFaceEnumeratorType>
        static void computeDTerm_LoopBody(
            double* NOALIAS                           QfaceNLeft,
            double* NOALIAS                           QfaceNRight,
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
            double* NOALIAS                           QD,
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
          const double* NOALIAS                   QDX,
          const double* NOALIAS                   QDY,
          const double* NOALIAS                   QDZ,
          const QNCPFaceEnumeratorType                 ncpEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* NOALIAS                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator
        ) InlineMethod;
        #if defined(GPUOffloadingOMP)
        #pragma omp end declare target
        #endif

        template <class QInEnumeratorType, class QInterEnumeratorType, class QOutEnumeratorType>
        void updateSolutionwithNCPandSource_LoopBody(
          const double* NOALIAS                   QIn,
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
          double* NOALIAS                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator,
          bool                                         evalNCP,
          bool                                         evalSRC
        ) InlineMethod;

        template <class QInEnumeratorType>
        static double reduceMaxEigenvalue_LoopBody(
          const double* NOALIAS                   QIn,
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

