// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "toolbox/blockstructured/Interpolation.h"
#include "toolbox/blockstructured/Restriction.h"


namespace toolbox {
  namespace blockstructured {
    /**
     * This is a wrapper around the toolbox routines. It ensures that we
     * have a templated function which has the same signature as the other
     * routines in blockstructured and thus can be swapped in and out. See
     * the documentation in exahype2.solvers.FV for example how to use it
     * within Python.
     */
    template <typename C>
    void interpolateHaloLayer_AoS_tensor_product(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    ) {
      interpolateHaloLayer_AoS_tensor_product(
        marker,
        numberOfDoFsPerAxisInPatch,
        overlap,
        unknowns,
        C::NormalInterpolationMatrix1d,
        C::TangentialInterpolationMatrix1d,
        coarseGridFaceValues,
        fineGridFaceValues
      );
    }


    template <typename C>
    void interpolateHaloLayer_AoS_tensor_product(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      const double* __restrict__                coarseGridFaceValues,
      double* __restrict__                      fineGridFaceValues
    ) {
      interpolateHaloLayer_AoS_tensor_product(
        marker,
        numberOfDoFsPerAxisInPatch,
        overlap,
        unknowns,
        C::NormalInterpolationMatrix1d,
        C::TangentialInterpolationMatrix1d,
        coarseGridCellValues,
        coarseGridFaceValues,
        fineGridFaceValues
      );
    }


    template <typename C>
    void interpolateCell_AoS_tensor_product(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      const double* __restrict__                coarseGridCellValues,
      double* __restrict__                      fineGridCellValues
    ) {
      interpolateCell_AoS_tensor_product(
        marker,
        numberOfDoFsPerAxisInPatch,
        unknowns,
        C::TangentialInterpolationMatrix1d,
        coarseGridCellValues,
        fineGridCellValues
      );
    }


    template <typename C>
    void restrictCell_AoS_tensor_product(
      const peano4::datamanagement::CellMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    ) {
      restrictCell_AoS_tensor_product(
        marker,
        numberOfDoFsPerAxisInPatch,
        unknowns,
        C::TangentialRestrictionMatrix1d,
        fineGridValues,
        coarseGridValues
      );
    }


    template <typename C>
    void restrictHaloLayer_AoS_tensor_product(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues
    ) {
      restrictHaloLayer_AoS_tensor_product(
        marker,
        numberOfDoFsPerAxisInPatch,
        overlap,
        unknowns,
        C::NormalRestrictionMatrix1d,
        C::TangentialRestrictionMatrix1d,
        fineGridValues,
        coarseGridValues
      );
    }


    template <typename C>
    void restrictInnerHalfOfHaloLayer_AoS_tensor_product(
      const peano4::datamanagement::FaceMarker& marker,
      int                                       numberOfDoFsPerAxisInPatch,
      int                                       overlap,
      int                                       unknowns,
      double*                                   fineGridValues,
      double*                                   coarseGridValues,
      bool                                      swapInsideOutside=false
    ) {
      restrictInnerHalfOfHaloLayer_AoS_tensor_product(
        marker,
        numberOfDoFsPerAxisInPatch,
        overlap,
        unknowns,
        C::NormalRestrictionMatrix1d,
        C::TangentialRestrictionMatrix1d,
        fineGridValues,
        coarseGridValues,
        swapInsideOutside
      );
    }
  }
}
