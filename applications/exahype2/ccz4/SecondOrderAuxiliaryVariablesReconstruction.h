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
       * The reconstruction uses a 4th order Finite Difference scheme.
       *
       * @todo Han, can we add some docu here.
       *
       * @param haloSize Has to be at least 3.
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
          const ::exahype2::enumerator::AoSLexicographicEnumerator&  QInEnumerator,
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
          const ::exahype2::enumerator::AoSLexicographicEnumerator&  QInEnumerator,
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
          const ::exahype2::enumerator::AoSLexicographicEnumerator&  QInEnumerator,
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
          const ::exahype2::enumerator::AoSLexicographicEnumerator&  QInEnumerator,
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
