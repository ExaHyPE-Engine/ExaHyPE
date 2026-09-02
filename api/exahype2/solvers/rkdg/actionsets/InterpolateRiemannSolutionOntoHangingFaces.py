# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau import ButcherTableau

import peano4
import jinja2


class InterpolateRiemannSolutionOntoHangingFaces(AbstractRungeKuttaDGActionSet):
    """
    Assemble the Riemann solutions a fine cell consumes at its hanging faces.

    A resolution transition conserves only if both sides integrate the same
    numerical flux. The coarse face solves its Riemann problem from its own
    trace and the restriction of the fine traces, and that happens at its first
    touch, before any adjacent fine cell of the same secondary sweep runs.
    Every fine cell with hanging faces therefore consumes that flux. This
    action set evaluates the coarse flux polynomial at each hanging face's
    nodes and adds the jump penalty against the cell's own trace, which
    stabilises the transition without disturbing the flux integral.
    AddVolumeAndFaceSolution then integrates the same flux as the coarse
    neighbour. Mass is conserved per Runge-Kutta stage; nonconservative
    products carry the coarse side's sign convention across the transition.
    """

    TemplateInterpolate = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    bool hasHangingFace = false;
    for (int face = 0; face < 2 * DIMENSIONS; face++) {
      hasHangingFace = hasHangingFace or fineGridFaces{{FACE_LABEL}}(face).getIsHanging();
    }
    if (hasHangingFace) {
      const double timeStampOldSolution = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

      // The snippet below declares and sets double timeStepSize.
      {{COMPUTE_TIME_STEP_SIZE}}

      const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;

      // Re-project the persistent linear combination onto the hanging faces: the
      // jump penalty needs the cell's own trace, and hanging faces hold no data
      // between grid sweeps. The persistent faces keep their primary-sweep
      // projection, so they receive scratch targets here.
      double scratchProjection[2 * DIMENSIONS][facedata::{{UNKNOWN_IDENTIFIER}}EstimateProjection::Cardinality];
      double* projectionTargets[2 * DIMENSIONS];
      for (int face = 0; face < 2 * DIMENSIONS; face++) {
        projectionTargets[face] = fineGridFaces{{FACE_LABEL}}(face).getIsHanging()
          ? fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(face).data()
          : scratchProjection[face];
      }
      #if DIMENSIONS==2
      ::exahype2::dg::projectVolumetricDataOntoFaces(
        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
        {{ORDER}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1D,
        projectionTargets[0], // Left
        projectionTargets[2], // Right
        projectionTargets[1], // Down
        projectionTargets[3]  // Up
      );
      #else
      ::exahype2::dg::projectVolumetricDataOntoFaces(
        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
        {{ORDER}},
        {{NUMBER_OF_UNKNOWNS}},
        {{NUMBER_OF_AUXILIARY_VARIABLES}},
        repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1D,
        projectionTargets[0], // Left
        projectionTargets[3], // Right
        projectionTargets[1], // Down
        projectionTargets[4], // Up
        projectionTargets[2], // Front
        projectionTargets[5]  // Back
      );
      #endif

      for (int hangingFaceNumber = 0; hangingFaceNumber < 2 * DIMENSIONS; hangingFaceNumber++) {
        if (fineGridFaces{{FACE_LABEL}}(hangingFaceNumber).getIsHanging()) {
          const int normal = hangingFaceNumber % DIMENSIONS;

          // The penalty needs one eigenvalue for the whole coarse face, so its
          // subface contributions cancel: every fine cell below the face derives
          // the same value from the coarse face's two trace layers. The parent
          // geometry is reconstructed from the fine cell's own marker, so the
          // reconstruction is exact and no neighbour coordinates are read.
          auto parentCellSize   = marker.getH();
          auto parentFaceOffset = marker.getX();
          for (int d = 0; d < DIMENSIONS; d++) {
            parentCellSize(d) *= 3.0;
            parentFaceOffset(d) += (1.0 - marker.getRelativePositionWithinFatherCell()(d)) * marker.getH()(d);
            parentFaceOffset(d) -= 0.5 * parentCellSize(d);
          }
          if (hangingFaceNumber >= DIMENSIONS) {
            parentFaceOffset(normal) += parentCellSize(normal);
          }

          double maxEigenvalue = 0.0;
          constexpr int ProjectionStride = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
          const double* NOALIAS coarseFaceProjection =
            coarseGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(hangingFaceNumber).data();
          peano4::dForEachExclusive({{ORDER}} + 1, normal, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& dof) {
            tarch::la::Vector<DIMENSIONS, double> x = parentFaceOffset;
            for (int d = 0; d < DIMENSIONS; d++) {
              if (d != normal) {
                x(d) += repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1D[dof(d)] * parentCellSize(d);
              }
            }
            for (int layer = 0; layer < 2; layer++) {
              auto shiftedDoF    = dof;
              shiftedDoF(normal) = layer;
              const int offset =
                ::exahype2::serialiseVoxelIndexInOverlap(shiftedDoF, {{ORDER}} + 1, 1, normal)
                * ProjectionStride;
              maxEigenvalue = std::fmax(
                maxEigenvalue,
                repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(coarseFaceProjection + offset, x, timeStamp, timeStepSize, normal)
              );
            }
          });

          ::exahype2::dg::interpolateRiemannSolution(
            hangingFaceNumber,
            marker.getRelativePositionWithinFatherCell(),
            {{ORDER}},
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            maxEigenvalue,
            repositories::{{SOLVER_INSTANCE}}.InterpolationMatrix1D,
            coarseGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(hangingFaceNumber).data(),
            coarseFaceProjection,
            fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(hangingFaceNumber).data(),
            fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(hangingFaceNumber).data()
          );
        }
      }
    }
  }
  {% endfor %}
"""

    def __init__(self, solver):
        super(InterpolateRiemannSolutionOntoHangingFaces, self).__init__(solver)
        self.guards = []
        self._butcher_tableau = ButcherTableau(self._solver._rk_order)

    @property
    def guards(self):
        if self._guards == []:
            raise Exception("Guards are not initialised")
        return self._guards

    @guards.setter
    def guards(self, new_guards):
        if (
            new_guards != []
            and len(new_guards) != self._solver.number_of_Runge_Kutta_steps()
        ):
            raise Exception(
                "Expect one guard per Runge-Kutta step. There are {} steps, but the guards are {}".format(
                    self._solver.number_of_Runge_Kutta_steps(), new_guards
                )
            )
        self._guards = new_guards

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATES"] = self.guards
            d["BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES"] = (
                self._butcher_tableau.time_step_sizes()
            )
            d["FACE_LABEL"] = self._solver._face_label.name
            result = jinja2.Template(self.TemplateInterpolate).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(InterpolateRiemannSolutionOntoHangingFaces, self).get_includes()
            + """
#include "exahype2/dg/Riemann.h"
#include "exahype2/Enumeration.h"
"""
        )
