// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Functors.h"

#include "exahype2/CellData.h"

#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
#include "exahype2/enumerator/SoALexicographicEnumerator.h"
#include "exahype2/enumerator/AoSoALexicographicEnumerator.h"

#include "tarch/multicore/multicore.h"

namespace exahype2 {
  namespace fd {
      constexpr int PickAllEntriesFromOutputVector = -1;

      /**
       *
       */
      void reduceMaxEigenvalue_patchwise_functors(
        ::exahype2::CellData<double, double>&   patchData,
        int                                     numberOfGridCellsPerPatchPerAxis,
        int                                     overlap,
        int                                     unknowns,
        int                                     auxiliaryVariables,
        MaxEigenvalue                           maxEigenvalue
      );

      namespace internal {
        #if defined(WITH_OPENMP) and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(WITH_GPU_OMP_TARGET))
        #pragma omp declare simd
        #endif
        #if defined(WITH_GPU_OMP_TARGET)
        #pragma omp declare target
        #endif
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void copySolution_LoopBody(
          const double* NOALIAS                QIn,
          const QInEnumeratorType&                  QInEnumerator,
          int                                       patchIndex,
          const tarch::la::Vector<DIMENSIONS,int>&  volumeIndex,
          int                                       unknown,
          double* NOALIAS                      QOut,
          const QOutEnumeratorType&                 QOutEnumerator
        );
        #if defined(WITH_GPU_OMP_TARGET)
        #pragma omp end declare target
        #endif

        /**
         * The complicated way to write =0.
         */
        #if defined(WITH_OPENMP) and ((!defined(__INTEL_LLVM_COMPILER) and !defined(__clang__) and !defined(__GNUC__)) or !defined(WITH_GPU_OMP_TARGET))
        #pragma omp declare simd
        #endif
        #if defined(WITH_GPU_OMP_TARGET)
        #pragma omp declare target
        #endif
        template <class QOutEnumeratorType>
        static void clearSolution_LoopBody(
          int                                       patchIndex,
          const tarch::la::Vector<DIMENSIONS,int>&  volumeIndex,
          int                                       unknown,
          double* NOALIAS                      QOut,
          const QOutEnumeratorType&                 QOutEnumerator
        );
        #if defined(WITH_GPU_OMP_TARGET)
        #pragma omp end declare target
        #endif

        /**
         * Copy previous solution over into new time step and add source term
         *
         * This is the first step of the method of line, where we basically copy
         * @f$ Q^{(old)} @f$ into @f$ Q^{(new)} @f$. In the same sweep, we also
         * add the algebraic source term subject to the time step size:
         *
         * @f$ Q^{(new)} = Q^{(old)} + \Delta t \cdot S(Q^{(old)}) @f$
         *
         * We reiterate that Q holds two types of entries: The actual data subject
         * to the PDE (and algebraic and differential source terms) and arbitrary
         * auxiliary variables per volume that the user has specified. Auxiliary
         * parameters are coupling terms, material parameters, helper
         * variables that you use to compute some metrics, ... They are not
         * evolved as part of the PDE. As the source term affects only the PDE
         * variables, we copy these entries in Q over and add the source term 
         * contribution, while the auxilibary parameters remain plain copies.
         *
         * No SIMD declaration here, as we can't SIMDise over functors anyway.
         *
         * @see copySolution_LoopBody() for vectorisation details.
         */
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void addAlgebraicSourceTerm_LoopBody(
          const double* NOALIAS                   QIn,
          const QInEnumeratorType&                     QInEnumerator,
          exahype2::fd::Source                         AlgebraicSource,
          const tarch::la::Vector<DIMENSIONS,double>&  patchCentre,
          const tarch::la::Vector<DIMENSIONS,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<DIMENSIONS,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          double* NOALIAS                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator
        );

        /**
         * Overloaded version which uses static invocation instead of a
         * functor.
         */
        template <typename Solver, class QInEnumeratorType, class QOutEnumeratorType>
        static void addAlgebraicSourceTerm_LoopBody(
          const double* NOALIAS                   QIn,
          const QInEnumeratorType&                     QInEnumerator,
          const tarch::la::Vector<DIMENSIONS,double>&  patchCentre,
          const tarch::la::Vector<DIMENSIONS,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<DIMENSIONS,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          double* NOALIAS                         QOut,
          const QOutEnumeratorType&                    QOutEnumerator
        );

        /**
         * Specialised version.
         */
        template <typename Solver>
        static void addAlgebraicSourceTerm_LoopBody(
          const double* NOALIAS                   QIn,
          const exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
          const tarch::la::Vector<DIMENSIONS,double>&  patchCentre,
          const tarch::la::Vector<DIMENSIONS,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<DIMENSIONS,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          double* NOALIAS                         QOut,
          const exahype2::enumerator::AoSLexicographicEnumerator& QOutEnumerator
        );

        template <class QOutEnumeratorType>
        inline static double reduceMaxEigenvalue_LoopBody(
          const double* NOALIAS                   QOut,
          const QOutEnumeratorType&                    QOutEnumerator,
          exahype2::fd::MaxEigenvalue                  maxEigenvalue,
          const tarch::la::Vector<DIMENSIONS,double>&  patchCentre,
          const tarch::la::Vector<DIMENSIONS,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<DIMENSIONS,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        );

        /**
         * Overloaded version with static function calls.
         */
        template <typename Solver, class QOutEnumeratorType>
        inline static double reduceMaxEigenvalue_LoopBody(
          const double* NOALIAS                   QOut,
          const QOutEnumeratorType&                    QOutEnumerator,
          const tarch::la::Vector<DIMENSIONS,double>&  patchCentre,
          const tarch::la::Vector<DIMENSIONS,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<DIMENSIONS,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        );

        /**
         * Overloaded version with static function calls.
         */
        template <typename Solver>
        static double reduceMaxEigenvalue_LoopBody(
          const double* NOALIAS                   QOut,
          const exahype2::enumerator::AoSLexicographicEnumerator& QOutEnumerator,
          const tarch::la::Vector<DIMENSIONS,double>&  patchCentre,
          const tarch::la::Vector<DIMENSIONS,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<DIMENSIONS,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        );

        /**
         * Specialisation without the gather.
         */
        template <>
        inline double reduceMaxEigenvalue_LoopBody(
          const double* NOALIAS                   QOut,
          const exahype2::enumerator::AoSLexicographicEnumerator& QOutEnumerator,
          exahype2::fd::MaxEigenvalue                  maxEigenvalue,
          const tarch::la::Vector<DIMENSIONS,double>&  patchCentre,
          const tarch::la::Vector<DIMENSIONS,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<DIMENSIONS,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        );
      }
    }
}

#include "LoopBody.cpph"
