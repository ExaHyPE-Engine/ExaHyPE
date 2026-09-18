# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet
from .UpdateSolution import UpdateSolution

from exahype2.solvers.ButcherTableau import ButcherTableau

import peano4
import jinja2


class InterpolateRiemannSolutionOntoHangingFaces(AbstractRungeKuttaDGActionSet):
    """
    Solves the Riemann problems at hanging faces.

    Hanging faces store no data between grid sweeps.
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

      const double timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();

      const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;

      double scratchProjection[2 * DIMENSIONS][facedata::{{UNKNOWN_IDENTIFIER}}EstimateProjection::Cardinality];
      double* projectionTargets[2 * DIMENSIONS];
      for (int face = 0; face < 2 * DIMENSIONS; face++) {
        projectionTargets[face] = fineGridFaces{{FACE_LABEL}}(face).getIsHanging()
          ? fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(face).data()
          : scratchProjection[face];
        projectionTargets[face] += face < DIMENSIONS
          ? facedata::{{UNKNOWN_IDENTIFIER}}EstimateProjection::Cardinality / 2
          : 0;
      }
      ::exahype2::dg::extrapolateStateToFaces<
        {{DG_ORDER}} + 1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}}
      >(
        projectionTargets,
        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
        kernels::{{SOLVER_NAME}}::DGMatrices::BasisFunctionValuesLeft1D,
        kernels::{{SOLVER_NAME}}::DGMatrices::BasisFunctionValuesRight1D
      );

      constexpr int NumberOfDofsPerAxis = {{DG_ORDER}} + 1;
      constexpr int StateElementsPerFace =
        facedata::{{UNKNOWN_IDENTIFIER}}EstimateProjection::Cardinality / 2;
      constexpr int FluxElementsPerFace =
        facedata::{{UNKNOWN_IDENTIFIER}}RiemannSolution::Cardinality / 2;

      for (int hangingFaceNumber = 0; hangingFaceNumber < 2 * DIMENSIONS; hangingFaceNumber++) {
        if (fineGridFaces{{FACE_LABEL}}(hangingFaceNumber).getIsHanging()) {
          const int direction  = hangingFaceNumber % DIMENSIONS;
          const int coarseSide = hangingFaceNumber < DIMENSIONS ? 0 : 1;

          // Face nodes follow the tangential axes in ascending order.
          const double* prolongationMatrixPerAxis[DIMENSIONS - 1];
          int tangentialAxis = 0;
          for (int axis = 0; axis < DIMENSIONS; axis++) {
            if (axis != direction) {
              prolongationMatrixPerAxis[tangentialAxis] =
                kernels::{{SOLVER_NAME}}::DGMatrices::ProlongationMatrix1D
                + marker.getRelativePositionWithinFatherCell()(axis) * NumberOfDofsPerAxis * NumberOfDofsPerAxis;
              tangentialAxis++;
            }
          }
          assertion(tangentialAxis == DIMENSIONS - 1, tangentialAxis);
          ::exahype2::amr::interpolateFaceFromCoarse(
            fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(hangingFaceNumber).data()
              + coarseSide * StateElementsPerFace,
            coarseGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(hangingFaceNumber).data()
              + coarseSide * StateElementsPerFace,
            prolongationMatrixPerAxis,
            NumberOfDofsPerAxis,
            {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
          );

          auto faceCentre = marker.getX();
          faceCentre(direction) += (hangingFaceNumber < DIMENSIONS ? -0.5 : 0.5)
            * marker.getH()(direction);

          const double* leftStateEstimates =
            fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(hangingFaceNumber).data();
          const double* rightStateEstimates = leftStateEstimates + StateElementsPerFace;
          double* leftFluxEstimates =
            fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(hangingFaceNumber).data();
          double* rightFluxEstimates = leftFluxEstimates + FluxElementsPerFace;

          for (int side = 0; side < 2; side++) {
            ::exahype2::dg::evaluateFluxOnFace<
              {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}} + 1,
              {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
              {% if FLUX_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
              {{ "true" if USE_DIFFUSIVE_FLUX else "false" }}
            >(
              leftFluxEstimates + side * FluxElementsPerFace,
              leftStateEstimates,
              rightStateEstimates,
              side,
              kernels::{{SOLVER_NAME}}::Quadrature::QuadraturePoints1D,
              faceCentre,
              marker.getH(),
              timeStamp,
              timeStepSize,
              direction
            );
          }

          {{RIEMANN_SOLVER_CALL}}
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
            raise ValueError("The guards are not initialised.")
        return self._guards

    @guards.setter
    def guards(self, new_guards):
        if (
            new_guards != []
            and len(new_guards) != self._solver.number_of_Runge_Kutta_steps()
        ):
            raise ValueError(
                "One guard per Runge-Kutta step is needed. There are {} steps and the guards are {}.".format(
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
                self._butcher_tableau.time_step_sizes()[-1:]
                + self._butcher_tableau.time_step_sizes()[:-1]
            )
            d["FACE_LABEL"] = self._solver._face_label.name
            d["FACE_IS_BOUNDARY"] = (
                f"fineGridFaces{self._solver._name}FaceLabel(hangingFaceNumber).getBoundary()"
            )
            d["FACE_NUMBER"] = "hangingFaceNumber"
            d["RIEMANN_SOLVER_CALL"] = jinja2.Template(
                UpdateSolution._RIEMANN_SOLVER_CALL
            ).render(**d)
            result = jinja2.Template(self.TemplateInterpolate).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(InterpolateRiemannSolutionOntoHangingFaces, self).get_includes()
            + """
#include "exahype2/amr/AMRRoutines.h"
#include "exahype2/dg/BoundaryExtrapolation.h"
#include "exahype2/dg/Riemann.h"
"""
        )
