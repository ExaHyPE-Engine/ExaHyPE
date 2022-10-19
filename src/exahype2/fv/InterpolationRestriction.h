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
     * routines in blockstructured and thus can be swapped in and out. To
     * use this guy within your FV solver, add the following steps:
     *
     *       mysolver.interpolation = "tensor_product< " + self._name + ">"
     *       mysolver..add_solver_constants( "static constexpr double  NormalInterpolationMatrix1d[]     = {1.0};""" )
     *       mysolver.add_solver_constants( "static constexpr double  TangentialInterpolationMatrix1d[] = {1.0,2.0,3.0};""" )
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
  }
}
