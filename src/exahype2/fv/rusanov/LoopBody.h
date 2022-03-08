// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_LOOP_BODY_H_
#define _EXAHYPE2_FV_RUSANOV_LOOP_BODY_H_



#include "Functors.h"

#include "../Generic.h"
#include "../PatchUtils.h"

namespace exahype2 {
  namespace fv {
    namespace rusanov {
      namespace internal {
        /**
         * Copy solution from QIn to QOut. Typically, both fields have different
         * halo sizes, so we have to use two different enumerators. The loop body
         * copies over exactly one scalar.
         *
         * @param patchIndex: Which patch is to be updated. QIn can hold multiple
         *   patches, and this index picks one of them.
         * @param volumeIndex: Which volume is to be updated within the target
         *   patch. This index refers to the target index.
         */
        template <class QInEnumeratorType, class QOutEnumeratorType>
        void copy_solution_loop_body(
          const double* __restrict__                QIn,
          QInEnumeratorType                         QInEnumerator,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          int                                       unknown,
          double* __restrict__                      QOut,
          QOutEnumeratorType                        QOutEnumerator
        );


        template <class QInEnumeratorType, class QOutEnumeratorType>
        void copy_solution_and_add_source_term_loop_body(
          const double* __restrict__                QIn,
          QInEnumeratorType                         QInEnumerator,
          exahype2::fv::rusanov::Source             source,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          double                                    t,
          double                                    dt,
          double* __restrict__                      QOut,
          QOutEnumeratorType                        QOutEnumerator
        );

        /**
         * Specialisation which exploits that input data and output data are available
         * as AoS.
         */
        template <>
        void copy_solution_and_add_source_term_loop_body(
          const double* __restrict__                QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QInEnumerator,
          exahype2::fv::rusanov::Source             source,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          double                                    t,
          double                                    dt,
          double* __restrict__                      QOut,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QOutEnumerator
        );


        /**
         * Compute maximum eigenvalue for one voxel in one direction.
         *
         * The user's maxEigenvalue functor expects the input data of one voxel
         * as one array. We cannot assume that the input data of the input is
         * available as unknown consecutive entries in the memory. So we first
         * have to gather the unknowns in one temporary vector, and then we pass
         * this vector into the user functor. The result is written back to
         * QMaxEigenvalue. Again, we don't know anything about its ordering, so
         * we use the enumerator to access to output array.
         */
        template <class QInEnumeratorType, class QMaxEigenvalueEnumeratorType>
        void compute_max_eigenvalue_loop_body(
          const double* __restrict__                   QIn,
          QInEnumeratorType                            QInEnumerator,
          exahype2::fv::rusanov::MaxEigenvalue         maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double* __restrict__                         QMaxEigenvalue,
          QMaxEigenvalueEnumeratorType                 QMaxEigenvalueEnumerator
        );


        /**
         * Specialisation of the template which exploits the fact that the input
         * data is already organised as AoS. So we don't have to gather anything.
         */
        template <class QMaxEigenvalueEnumeratorType>
        void compute_max_eigenvalue_loop_body(
          const double* __restrict__                   QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator  QInEnumerator,
          exahype2::fv::rusanov::MaxEigenvalue         maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double* __restrict__                         QMaxEigenvalue,
          QMaxEigenvalueEnumeratorType                 QMaxEigenvalueEnumerator
        );


        template <class QInEnumeratorType>
        double reduce_max_eigenvalue_loop_body(
          const double* __restrict__                   QIn,
          QInEnumeratorType                            QInEnumerator,
          exahype2::fv::rusanov::MaxEigenvalue         maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        );


        /**
         * Specialisation, as we know that the outcome is already AoS.
         */
        template <>
        double reduce_max_eigenvalue_loop_body(
          const double* __restrict__                   QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QInEnumerator,
          exahype2::fv::rusanov::MaxEigenvalue         maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        );


        /**
         * Compute the flux in one direction for a volume and store it.
         * Computing the fluxes requires us to gather the input data, invoke
         * the flux function and then to scatter the result again, as the
         * user signatures require an AoS data format whereas the loop body
         * itself is generic.
         */
        template <class QInEnumeratorType, class QFluxEnumeratorType>
        void compute_flux_loop_body(
          const double* __restrict__                QIn,
          QInEnumeratorType                   QInEnumerator,
          exahype2::fv::rusanov::Flux         flux,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                              dt,
          int                                 normal,
          double* __restrict__                QFlux,
          QFluxEnumeratorType                 QFluxEnumerator
        );


        /**
         * Specialisation of the flux loop body which takes into account that we work
         * on a sequence of patches with lexicographic AoS data. AoS is exactly what
         * the user's flux function expects us to pass in, so there is no need to
         * gather or scatter data.
         */
        template <>
        void compute_flux_loop_body<exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator,exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator>(
          const double* __restrict__                                  QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator  QInEnumerator,
          exahype2::fv::rusanov::Flux                                 flux,
          const tarch::la::Vector<Dimensions,double>&                 patchCentre,
          const tarch::la::Vector<Dimensions,double>&                 patchSize,
          int                                                         patchIndex,
          const tarch::la::Vector<Dimensions,int>&                    volumeIndex,
          double                                                      t,
          double                                                      dt,
          int                                                         normal,
          double* __restrict__                                        QFlux,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator  QFluxEnumerator
        );


        /**
         * Update the solution in QOut.
         *
         * The Z flow array is ignored if you use the code in 2d.
         *
         */
        template <typename QInEnumeratorType, typename QFluxEnumeratorType, typename QMaxEigenvalueEnumeratorType, typename QOutEnumeratorType>
        void update_solution_with_flux_loop_body(
          const double* __restrict__                   QIn,
          QInEnumeratorType                            QInEnumerator,
          const double* __restrict__                   tempFluxX,
          const double* __restrict__                   tempFluxY,
          const double* __restrict__                   tempFluxZ,
          QFluxEnumeratorType                          fluxEnumerator,
          double* __restrict__                         QMaxEigenvalueX,
          double* __restrict__                         QMaxEigenvalueY,
          double* __restrict__                         QMaxEigenvalueZ,
          QMaxEigenvalueEnumeratorType                 QMaxEigenvalueEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* __restrict__                         QOut,
          QOutEnumeratorType                           QOutEnumerator
        );
      }
    }
  }
}


#include "LoopBody.cpph"

#endif
