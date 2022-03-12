// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_RUSANOV_LOOP_BODY_H_
#define _EXAHYPE2_FV_RUSANOV_LOOP_BODY_H_



#include "Functors.h"

#include "exahype2/fv/enumerator/AoSLexicographicVolumeEnumerator.h"
#include "exahype2/fv/enumerator/SoALexicographicVolumeEnumerator.h"
#include "exahype2/fv/enumerator/AoSoALexicographicVolumeEnumerator.h"

#include "tarch/multicore/multicore.h"


namespace exahype2 {
  namespace fv {
    namespace rusanov {
      constexpr int PickAllEntriesFromOutputVector = -1;

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
       * Where it makes sense, I provide specialisations of the routines which
       * exploit that input data and output data are available as AoS. All of
       * ExaHyPE's user routines are phrased as AoS, i.e. you are given one or two
       * or a set of structs representing a volume, and then you return a struct
       * (tuple) of outputs. If the underlying data storage is not organised as AoS, then
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
       *
       */


      /******
       *
       * Don't use code like this one
       *
       * <pre>
  tarch::la::Vector<Dimensions,int> leftVolume   = volumeIndex;
  tarch::la::Vector<Dimensions,int> rightVolume  = volumeIndex;
  leftVolume(normal)--;
  rightVolume(normal)++;
         </pre>
       *
       */
      namespace internal {
        /**
         * Copy solution from QIn to QOut. Typically, both fields have different
         * halo sizes, so we have to use two different enumerators. The loop body
         * copies over exactly one scalar.
         *
         * There is no specialisation for this one, as it is a plain copy anyway.
         *
         * ## Vectorisation
         *
         * One might be tempted to think of this as a plain memcopy. However, it
         * is more than that: The input data has a halo, while the output data
         * (typically) has no halo. So they are not of the same size. We therefore
         * can copy lines from the input in one rush, but we cannot copy all of it.
         * In the outer loop calling this routine, we exploit the fact that we
         * know that we have AoS for both usually, so as long as we loop over the
         * unknowns and the x-direction internally and collapse those two loops,
         * we can actually use simd.
         *
         * It makes no sense to provide specialisation of this routine for
         * different enumerators, as the routine works per volume per unknown
         * anyway.
         *
         * @param patchIndex: Which patch is to be updated. QIn can hold multiple
         *   patches, and this index picks one of them.
         * @param volumeIndex: Which volume is to be updated within the target
         *   patch. This index refers to the target index.
         */
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void copySolution_LoopBody(
          const double* __restrict__                QIn,
          QInEnumeratorType                         QInEnumerator,
          int                                       patchIndex,
          const tarch::la::Vector<Dimensions,int>&  volumeIndex,
          int                                       unknown,
          double* __restrict__                      QOut,
          QOutEnumeratorType                        QOutEnumerator
        ) InlineMethod;


        /**
         * @see copySolution_LoopBody() for vectorisation details.
         */
        template <class QInEnumeratorType, class QOutEnumeratorType>
        static void copySolutionAndAddSourceTerm_LoopBody(
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


        /**
         * Specialisation of the other copySolutionAndAddSourceTerm_LoopBody()
         * which exploits knowledge that we do not have to gather input data,
         * as the input data is delivered as AoS already.
         *
         * In line with the discussion in copySolution_LoopBody(), this operation
         * can vectorise properly if and only if you make your x loop the
         * innermost one and if and only if your input is AoS - which is what
         * this specialisation is about.
         *
         * @see copySolution_LoopBody() for vectorisation details.
         */
        template <>
        static void copySolutionAndAddSourceTerm_LoopBody(
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
        static void copySolutionAndAddSourceTerm_LoopBody(
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
        static void copySolutionAndAddSourceTerm_LoopBody(
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
         * The nice thing with the max (directional) eigenvalue is that the value
         * is a scalar. So we have to gather the input into one Q vector (see
         * specialisation below which eliminate this step) but then the outcome
         * is a scalar which we can write straight back. So this routine should
         * straightforwardly vectorise.
         */
        template <class QInEnumeratorType, class QMaxEigenvalueEnumeratorType>
        static void computeMaxEigenvalue_LoopBody(
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


        /**
         * Specialisation if input data is already in AoS format. This holds for
         * all of our kernels unless you explicitly reorder the input.
         */
        template <class QMaxEigenvalueEnumeratorType>
        static void computeMaxEigenvalue_LoopBody(
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


        /**
         * Specialisation invoking the static eigenvalue variant (for GPUs).
         */
        template <typename Solver, class QInEnumeratorType, class QMaxEigenvalueEnumeratorType>
        static void computeMaxEigenvalue_LoopBody(
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


        /**
         * Specialisation for static eigenvalue call plus AoS input data.
         */
        template <typename Solver, class QMaxEigenvalueEnumeratorType>
        static void computeMaxEigenvalue_LoopBody(
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


        /**
         * This is the actual eigenvalue loop which reduces the directional
         * eigenvalues.
         */
        template <class QInEnumeratorType>
        static double reduceMaxEigenvalue_LoopBody(
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
        static double reduceMaxEigenvalue_LoopBody(
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
        static double reduceMaxEigenvalue_LoopBody(
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
        static double reduceMaxEigenvalue_LoopBody(
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
        static void computeFlux_LoopBody(
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


/*
        template <class QInEnumeratorType>
        static void computeFlux_LoopBody(
          const double* __restrict__                QIn,
          exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator                   QInEnumerator,
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
*/


        static void computeFlux_LoopBody(
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
        ) InlineMethod;


        template <typename Solver, class QInEnumeratorType, class QFluxEnumeratorType>
        static void computeFlux_LoopBody(
          const double* __restrict__                   QIn,
          const QInEnumeratorType &                    QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double* __restrict__                         QFlux,
          const QFluxEnumeratorType &                  QFluxEnumerator,
          int                                          pick = PickAllEntriesFromOutputVector
        ) InlineMethod;


        template <typename Solver, class QFluxEnumeratorType>
        static void computeFlux_LoopBody(
          const double* __restrict__                                  QIn,
          const exahype2::fv::enumerator::AoSLexicographicVolumeEnumerator & QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&                 patchCentre,
          const tarch::la::Vector<Dimensions,double>&                 patchSize,
          int                                                         patchIndex,
          const tarch::la::Vector<Dimensions,int>&                    volumeIndex,
          double                                                      t,
          double                                                      dt,
          int                                                         normal,
          double* __restrict__                                        QFlux,
          const QFluxEnumeratorType &                                 QFluxEnumerator,
          int                                                         pick = PickAllEntriesFromOutputVector
        ) InlineMethod;


        template <typename Solver>
        static void computeFlux_LoopBody(
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
        static void updateSolutionWithFlux_LoopBody(
          const double* __restrict__                   tempFluxX,
          const double* __restrict__                   tempFluxY,
          const double* __restrict__                   tempFluxZ,
          QFluxEnumeratorType                          fluxEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
          double                                       dt,
          double* __restrict__                         QOut,
          QOutEnumeratorType                           QOutEnumerator
        ) InlineMethod;


        /**
         * Uses the eigenvalues to damp the solution update.
         *
         *
         * ## Rationale for alternative realisations
         *
         * I originally throught the following:
         *
         * We could damp along the individual coordinate axis (normal). That is, if normal
         * equals zero, we take the left and right neighbour of a cell into account,
         * if it equals one, we look at the lower and upper face-connected neighbour.
         *
         * To allow the compiler to vectorise over this routine, it is essential that
         * you make the loop over normal the outer loop, and then you can collapse
         * and vectorise over the patch. Yet, even if you do so, there is no guarantee
         * that the compiler will manage to vectorise. It really depends upon the
         * QOutEnumerator, i.e. the ordering of the image. If the image is stored as
         * AoS (default ordering for output data), then the loops over this routine
         * cannot vectorise properly. To make a long story short: This routine will
         * vectorise if and only if you explicitly re-sort input and output values of
         * a patch prior to the update such that they are in SoA format. Obviously,
         * it is up to you to decide if the overhead for this re-sorting is worth it.
         *
         * Anyway, such a damping is a fundamental rewrite, as we then have to pick
         * either tempMaxEigenvalueX, Y or Z, so we might end up with three routines.
         *
         * Along the same lines, all of these arguments motivate why we do not specialise
         * the template further. We need to do this scattering/gathering anyway, so
         * what's the point of specialising for particular data formats?
         */
        template <typename QInEnumeratorType, typename QMaxEigenvalueEnumeratorType, typename QOutEnumeratorType>
        static void updateSolutionWithEigenvalueDamping_LoopBody(
          const double* __restrict__                   QIn,
          const QInEnumeratorType &                    QInEnumerator,
          const double* __restrict__                   tempMaxEigenvalueX,
          const double* __restrict__                   tempMaxEigenvalueY,
          const double* __restrict__                   tempMaxEigenvalueZ,
          const QMaxEigenvalueEnumeratorType &         eigenvalueEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          tarch::la::Vector<Dimensions,int>            volumeIndex,
          int                                          unknown,
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
        static void computeNonconservativeFlux_LoopBody(
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
          QNCPFaceEnumeratorType              QNcpEnumerator,
          int                                                         pick = PickAllEntriesFromOutputVector
        ) InlineMethod;


        template <typename Solver, class QInEnumeratorType, class QNCPFaceEnumeratorType>
        static void computeNonconservativeFlux_LoopBody(
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
          QNCPFaceEnumeratorType              QNcpEnumerator,
          int                                                         pick = PickAllEntriesFromOutputVector
        ) InlineMethod;


        /**
         * See remarks in updateSolutionWithNonconservativeFlux_LoopBody() for
         * some rationale.
         */
        template <typename QNCPFaceEnumeratorType, typename QOutEnumeratorType>
        static void updateSolutionWithNonconservativeFlux_LoopBody(
          const double* __restrict__                   QNcpX,
          const double* __restrict__                   QNcpY,
          const double* __restrict__                   QNcpZ,
          QNCPFaceEnumeratorType                       ncpEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          unknown,
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
