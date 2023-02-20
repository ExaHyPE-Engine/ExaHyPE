// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once



#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>


namespace exahype2 {
  namespace fd {
    /**
     * Apply (generic) boundary conditions
     *
     * This implementation runs over the boundary and applies the generic
     * (point-wise) boundary conditions via a call-back to the user
     * implementation.
     *
     * @param unknownsPlusAuxiliaryVariables Number of unknowns plus the number
     *   of auxiliary variables. The boundary treatment routine does not
     *   distinguish how the two types of quantities are handled, so it is
     *   reasonable to only pass in the sum.
     *
     * @param faceNumber Is usually taken from marker.getSelectedFaceNumber()
     *   and is thus a number between 0 and 2d-1.
     *
     */
    void applyBoundaryConditions(
      std::function< void(
        const double* __restrict__                   Qinside,
        double * __restrict__                        Qoutside,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   boundaryCondition,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfGridCellsPerPatchPerAxis,
      int                                          overlap,
      int                                          unknownsPlusAuxiliaryVariables,
      int                                          faceNumber,
      double* __restrict__                         Q
    );


    /**
     * Apply Sommerfeld boundary conditions
     *
     * Sommerfeld boundary conditions don't need a callback for the actual
     * boundary data, but they need the max flux. If you employ Sommerfeld
     * conditions, you can ignore the generic boundary condition function
     * of your solver. It will still remain there, but you can leave it
     * empty (or add an assertion in there, as it should never be called).
     *
     *
     *
     * ## Usage
     *
     * The typical user switches to Sommerfeld conditions by exchanging the
     * boundary kernel call:
     *
     * <pre>
self._action_set_handle_boundary.TemplateHandleBoundary_KernelCalls = """
      ::exahype2::fd::applySommerfeldConditions(
        [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  faceCentre,
          const tarch::la::Vector<Dimensions,double>&  gridCellH,
          double                                       t,
          double                                       dt,
          int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, gridCellH, t, dt, normal );
        },
        marker.x(),
        marker.h(),
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
        {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}},
        {{OVERLAP}},
        {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
        marker.getSelectedFaceNumber(),
        {0.0, 0.0, 0.0},
        fineGridFace{{UNKNOWN_IDENTIFIER}}Old.value,
        fineGridFace{{UNKNOWN_IDENTIFIER}}New.value
      );
      </pre>
"""
     *
     */
    void applySommerfeldConditions(
      std::function< double(
        const double* __restrict__                   Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  gridCellH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   maxEigenvalue,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       dt,
      int                                          numberOfGridCellsPerPatchPerAxis,
      int                                          overlap,
      int                                          unknownsPlusAuxiliaryVariables,
      int                                          faceNumber,
      const tarch::la::Vector<Dimensions,double>&  systemOrigin,
      double* __restrict__                         Qold,
      double* __restrict__                         Qnew
    );
  }
}


