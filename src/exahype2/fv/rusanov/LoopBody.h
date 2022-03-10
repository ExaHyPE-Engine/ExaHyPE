// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_LOOP_BODY_H_
#define _EXAHYPE2_FV_RUSANOV_LOOP_BODY_H_



#include "Functors.h"

#include "../Generic.h"
#include "../PatchUtils.h"

#include "tarch/multicore/multicore.h"


namespace exahype2 {
  namespace fv {
    namespace rusanov {
      /**
       * @namespace internal
       *
       * The static storage class qualifier is required to avoid that we get
       * multiple defined instances errors in Clang: We need all functions to
       * be defined in headers to allow the single-pass compiler to get the
       * inlining right, but then get multiple definitions without the static
       * marker.
       *
       *
       * Please note that the compiler will likely complain that it can't inline.
       * In this case, search for the code location twice: If you call the whole
       * thing without static function calls, it won't be inlined. But if you use
       * the kernel also with static function calls, then the code snippet
       * should be inlined.
       *
       *
       * As I've moved all the definitions into the header file, the compiler
       * can and does successfully inline all function calls within the Rusanov
       * solvers.
       *
       *
       * For all of the routines, I provide specialisations which exploits that
       * input data and output data are available as AoS. All of ExaHyPE's user
       * routines are phrased as AoS, i.e. you are given one or two or a set of
       * structs representing a volume, and then you return a struct (tuple) of
       * outputs. If the underlying data storage is not organised as AoS, then
       * we have to first gather data, pass it into the user functions, and then
       * fetch it back. If data however is organised as AoS, we can directly
       * invoke the user's functions on the data. No need to gather and scatter.
       * While I refer to these routines as specialisations, they are technically
       * realised via template overloading.
       *
       *
       * Further to the specialisation, each loop body routine is also available
       * in its static counterpart, i.e. in a version which does not accept a
       * functor but directly invokes the underlying solver's routines. These
       * special cases refer to the actual static routines which are there to
       * support accelerators.
       *
       *
       * If you compile the code, you'll get warning like
       * <pre>
../../..//src/exahype2/fv/rusanov/LoopBody.h:239:23: warning: explicit specialization cannot have a storage class
</pre>
       * but it is safe to ignore these. Without a static qualifier (which led to
       * this warning), the linking will fail due to multiple definitions.
       */
      namespace internal {
        /**
         * Copy solution from QIn to QOut. Typically, both fields have different
         * halo sizes, so we have to use two different enumerators. The loop body
         * copies over exactly one scalar.
         *
         * There is no specialisation for this one, as it is a plain copy anyway.
         *
         * @param patchIndex: Which patch is to be updated. QIn can hold multiple
         *   patches, and this index picks one of them.
         * @param volumeIndex: Which volume is to be updated within the target
         *   patch. This index refers to the target index.
         */
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void copy_solution_loop_body(
          const double* __restrict__                QIn,
          QInEnumeratorType                         QInEnumerator,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          int                                       unknown,
          double* __restrict__                      QOut,
          QOutEnumeratorType                        QOutEnumerator
        ) InlineMethod;


        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void copy_solution_and_add_source_term_loop_body(
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
        ) InlineMethod;


        template <>
        static void copy_solution_and_add_source_term_loop_body(
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
        )  InlineMethod;


        template <typename Solver, class QInEnumeratorType, class QOutEnumeratorType>
        static void copy_solution_and_add_source_term_loop_body(
          const double* __restrict__                QIn,
          QInEnumeratorType                         QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          double                                    t,
          double                                    dt,
          double* __restrict__                      QOut,
          QOutEnumeratorType                        QOutEnumerator
        ) InlineMethod;


        template <typename Solver>
        static void copy_solution_and_add_source_term_loop_body(
          const double* __restrict__                QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          double                                    t,
          double                                    dt,
          double* __restrict__                      QOut,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QOutEnumerator
        )  InlineMethod;


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
         *
         */
        template <class QInEnumeratorType, class QMaxEigenvalueEnumeratorType>
        static void compute_max_eigenvalue_loop_body(
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
        )  InlineMethod;


        template <class QMaxEigenvalueEnumeratorType>
        static void compute_max_eigenvalue_loop_body(
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
        ) InlineMethod;


        template <typename Solver, class QInEnumeratorType, class QMaxEigenvalueEnumeratorType>
        static void compute_max_eigenvalue_loop_body(
          const double* __restrict__                   QIn,
          QInEnumeratorType                            QInEnumerator,
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


        template <typename Solver, class QMaxEigenvalueEnumeratorType>
        static void compute_max_eigenvalue_loop_body(
          const double* __restrict__                   QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator  QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double* __restrict__                         QMaxEigenvalue,
          QMaxEigenvalueEnumeratorType                 QMaxEigenvalueEnumerator
        ) InlineMethod;


        template <class QInEnumeratorType>
        static double reduce_max_eigenvalue_loop_body(
          const double* __restrict__                   QIn,
          QInEnumeratorType                            QInEnumerator,
          exahype2::fv::rusanov::MaxEigenvalue         maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        ) InlineMethod;


        template <>
        static double reduce_max_eigenvalue_loop_body(
          const double* __restrict__                   QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QInEnumerator,
          exahype2::fv::rusanov::MaxEigenvalue         maxEigenvalue,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        ) InlineMethod;


        template <typename Solver, class QInEnumeratorType>
        static double reduce_max_eigenvalue_loop_body(
          const double* __restrict__                   QIn,
          QInEnumeratorType                            QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        ) InlineMethod;


        template <typename Solver>
        static double reduce_max_eigenvalue_loop_body(
          const double* __restrict__                   QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt
        ) InlineMethod;


        /**
         * Compute the flux in one direction for a volume and store it.
         * Computing the fluxes requires us to gather the input data, invoke
         * the flux function and then to scatter the result again, as the
         * user signatures require an AoS data format whereas the loop body
         * itself is generic.
         */
        template <class QInEnumeratorType, class QFluxEnumeratorType>
        static void compute_flux_loop_body(
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
        ) InlineMethod;


        static void compute_flux_loop_body(
          const double* __restrict__                                  QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator  QInEnumerator,
          exahype2::fv::rusanov::Flux         flux,
          const tarch::la::Vector<Dimensions,double>&                 patchCentre,
          const tarch::la::Vector<Dimensions,double>&                 patchSize,
          int                                                         patchIndex,
          const tarch::la::Vector<Dimensions,int>&                    volumeIndex,
          double                                                      t,
          double                                                      dt,
          int                                                         normal,
          double* __restrict__                                        QFlux,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator  QFluxEnumerator
        ) InlineMethod;


        template <typename Solver, class QInEnumeratorType, class QFluxEnumeratorType>
        static void compute_flux_loop_body(
          const double* __restrict__                   QIn,
          QInEnumeratorType                            QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                              dt,
          int                                 normal,
          double* __restrict__                QFlux,
          QFluxEnumeratorType                 QFluxEnumerator
        ) InlineMethod;


        template <typename Solver>
        static void compute_flux_loop_body(
          const double* __restrict__                                  QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator  QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&                 patchCentre,
          const tarch::la::Vector<Dimensions,double>&                 patchSize,
          int                                                         patchIndex,
          const tarch::la::Vector<Dimensions,int>&                    volumeIndex,
          double                                                      t,
          double                                                      dt,
          int                                                         normal,
          double* __restrict__                                        QFlux,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator  QFluxEnumerator
        ) InlineMethod;


        template <typename QFluxEnumeratorType, typename QOutEnumeratorType>
        static void update_solution_with_flux_loop_body(
          const double* __restrict__                   tempFluxX,
          const double* __restrict__                   tempFluxY,
          const double* __restrict__                   tempFluxZ,
          QFluxEnumeratorType                          fluxEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          int                                          d,
          double                                       dt,
          double* __restrict__                         QOut,
          QOutEnumeratorType                           QOutEnumerator
        ) InlineMethod;


        /**
         * To allow the compiler to vectorise over this routine, it is absolutely
         * essential that ::exahype2::fv::getVolumeSize() inlines properly. However,
         * this does not work without lto, and we found lto often tricky to use. So
         * instead of using
         *
         *     const double scaling = dt / ::exahype2::fv::getVolumeSize( patchSize, QOutEnumerator._numberOfVolumesPerAxisInPatch )(0);
         *
         * we calculate the scaling manually. This allows us, in theory, to run
         * non-cubic finite volumes as well.
         *
         * It is important that we do not accumulate the horizontal, vertical,
         * front-to-back fluxes directly in QOut. Instead, we store them in
         * individual variables and then add them to QOut in one rush. Otherwise,
         * we create write-write dependencies and the LLVM compiler will refuse to
         * vectorise.
         */
        template <typename QInEnumeratorType, typename QMaxEigenvalueEnumeratorType, typename QOutEnumeratorType>
        static void update_solution_with_eigenvalue_damping(
          const double* __restrict__                   QIn,
          const QInEnumeratorType &                    QInEnumerator,
          double* __restrict__                         tempMaxEigenvalueX,
          double* __restrict__                         tempMaxEigenvalueY,
          double* __restrict__                         tempMaxEigenvalueZ,
          const QMaxEigenvalueEnumeratorType &         eigenvalueEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          tarch::la::Vector<Dimensions,int>            volumeIndex,
          int                                          unknown,
          int                                          normal,
          double                                       dt,
          double* __restrict__                         QOut,
          const QOutEnumeratorType &                   QOutEnumerator
        ) InlineMethod;


        /**
         * We count faces starting with zero, i.e. effectively shift the indices
         * not by 1/2 but one volume to the right: Let the leftmost volume have
         * the index i. i is typically 0 but can also be negative (if we have halos).
         * The leftmost volume's left face does not exist. Its right face has index
         * 0.
         *
         * There is no specialisation for this class, as we have to gather data
         * always (we have to reconstruct the average and the jump term).
         */
        template <class QInEnumeratorType, class QNCPFaceEnumeratorType>
        static void compute_nonconservative_flux_loop_body(
          const double* __restrict__                     QIn,
          QInEnumeratorType                              QInEnumerator,
          exahype2::fv::rusanov::NonconservativeProduct  ncp,
          const tarch::la::Vector<Dimensions,double>&    patchCentre,
          const tarch::la::Vector<Dimensions,double>&    patchSize,
          int                                            patchIndex,
          const tarch::la::Vector<Dimensions,int>&       volumeIndex,
          double                                         t,
          double                              dt,
          int                                 normal,
          double* __restrict__                QNcp,
          QNCPFaceEnumeratorType              QNcpEnumerator
        ) InlineMethod;


        template <typename Solver, class QInEnumeratorType, class QNCPFaceEnumeratorType>
        static void compute_nonconservative_flux_loop_body(
          const double* __restrict__                     QIn,
          QInEnumeratorType                              QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&    patchCentre,
          const tarch::la::Vector<Dimensions,double>&    patchSize,
          int                                            patchIndex,
          const tarch::la::Vector<Dimensions,int>&       volumeIndex,
          double                                         t,
          double                              dt,
          int                                 normal,
          double* __restrict__                QNcp,
          QNCPFaceEnumeratorType              QNcpEnumerator
        ) InlineMethod;


        template <typename QNCPFaceEnumeratorType, typename QOutEnumeratorType>
        static void update_solution_with_nonconservative_flux_loop_body(
          const double* __restrict__                   QNcpX,
          const double* __restrict__                   QNcpY,
          const double* __restrict__                   QNcpZ,
          QNCPFaceEnumeratorType                       ncpEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          int                                          d,
          double                                       dt,
          double* __restrict__                         QOut,
          QOutEnumeratorType                           QOutEnumerator
        ) InlineMethod;
      }
    }
  }
}


#include "LoopBody.cpph"

#endif
