// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "exahype2/CellData.h"
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"


namespace applications {
  namespace exahype2 {
    namespace ccz4 {
      /**
       * Recompute auxiliary variables for FD4 scheme with a 4th order scheme
       *
       * We are given a patch as well as the number of mesh cells per axis
       * within this patch. I also parameterise over the unknowns and the
       * auxiliaryVariables. By default, we'd expect 59 and 0 here, but we
       * want to routine to work for augmented systems, too.
       *
       * The reconstruction uses a 4th order Finite Difference scheme. The
       * implementation in applications::exahype2::ccz4::internal::recomputeAuxiliaryVariablesFD4_4thOrder_LoopBody()
       * realises the normal stencil
       *
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       * [1 -8 0 8 -1]
       * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
       *
       * scaled with @f$ \frac{1}{12h} @f$. There is a fundamental problem with
       * this variant which is worth mentioning: To evaluate this stencil, we
       * need a halo layer of at least two. For a given haloSize, we can never
       * update the outermost two layers of the code.
       *
       *
       * ## Shortcomings
       *
       * @image html recomputeAuxiliaryVariablesFD4_4thOrder.png
       *
       * As we use a fourth order scheme, we need two neighbours around each
       * point to construct the outcome. That means, when we are given a patch
       * with size 3 (left illustation above), we effectively can only calculate
       * the auxiliary variables within the patch (green) and the first halo
       * layer around it (brownish). This is illustrated to the right.
       *
       * There's one further complexity: we usually do not have the digonal
       * values in ExaHyPE. What we really get is not the illustration above
       * to the left but the one in the centre. The diagonal blocks hold
       * garbage. As a consequence, the auxiliary data in the brown data layer
       * to the right is not properly computed. In the left brown layer, only
       * the x-derivatives are properly reconstructed. The y-derivatives
       * contain garbage.
       *
       *
       * @param patchData          Host the actual patch data. For this
       *   function, we only have to ensure that the QIn (data plus halo)
       *   are properly set, and that the patch size in patchData is
       *   correct. All the other (meta data) properties have no influence
       *   and can hold any data.
       * @param numberOfGridCellsPerPatchPerAxis Has to be at least 3, as
       *   the halo is also at least three. More general, has to be bigger or
       *   equal to haloSize.
       * @param haloSize           Has to be at least 3.
       * @param unknowns           Typically 59, but can be bigger if you hold
       *   additional quantities within the PDE.
       * @param auxiliaryVariables Typically 0.
       *
       * @see applications::exahype2::ccz4::internal::recomputeAuxiliaryVariablesFD4_4thOrder_LoopBody()
       */
      void recomputeAuxiliaryVariablesFD4_4thOrder(
        ::exahype2::CellData&   patchData,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables
      );

      void recomputeAuxiliaryVariablesFD4_centralDifferences(
        ::exahype2::CellData&   patchData,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables
      );

      void recomputeAuxiliaryVariablesFD4_leftDifferences(
        ::exahype2::CellData&   patchData,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables
      );

      void recomputeAuxiliaryVariablesFD4_rightDifferences(
        ::exahype2::CellData&   patchData,
        int                     numberOfGridCellsPerPatchPerAxis,
        int                     haloSize,
        int                     unknowns,
        int                     auxiliaryVariables
      );

      namespace internal {
        /**
         * Recompute auxiliary variables
         *
         * This function calculates the auxiliary variables as gradietns of the primary
         * variables. The graident is calculated using a 5-point stencil in 1D which has
         * 2 neighbouring cells on each side of the central one. The computation requires
          * us to have access to 2 halo layers on each side of a given patch.
         *
         */
        void recomputeAuxiliaryVariablesFD4_4thOrder_LoopBody(
          double* __restrict__                                       QIn,
          const ::exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          normal
        );


        /**
         *
         */
        void recomputeAuxiliaryVariablesFD4_centralDifferences_LoopBody(
          double* __restrict__                                       QIn,
          const ::exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          normal
        );


        /**
         *
         */
        void recomputeAuxiliaryVariablesFD4_leftDifferences_LoopBody(
          double* __restrict__                                       QIn,
          const ::exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          normal
        );


        /**
         *
         */
        void recomputeAuxiliaryVariablesFD4_rightDifferences_LoopBody(
          double* __restrict__                                       QIn,
          const ::exahype2::enumerator::AoSLexicographicEnumerator& QInEnumerator,
          const tarch::la::Vector<Dimensions,double>&  patchCentre,
          const tarch::la::Vector<Dimensions,double>&  patchSize,
          int                                          patchIndex,
          const tarch::la::Vector<Dimensions,int>&     volumeIndex,
          int                                          normal
        );
      }
    }
  }
}
