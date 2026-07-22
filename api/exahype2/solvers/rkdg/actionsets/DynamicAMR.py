# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import jinja2

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet
from .ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections
from .ProjectLinearCombinationOfEstimatesOntoFaces import (
    compute_number_of_face_projection_quantities,
)


class AdaptiveMeshFaceCoupling(AbstractRungeKuttaDGActionSet):
    """
    Per-time-step resolution-transition coupling for Runge-Kutta DG adaptive meshes.

    In the primary sweep of every Runge-Kutta step it restricts the fine cells'
    projected traces onto the coarse face, whose own Riemann solve then serves
    both sides of the transition: the fine cells consume its flux through the
    action set InterpolateRiemannSolutionOntoHangingFaces in the secondary
    sweep, and the hanging faces lift their time stamps from the coarse face.
    It changes no topology and reports no grid-control events. The file
    exahype2/dg/Riemann.h describes the DG AMR semantics in detail. This action
    set mirrors the FV and ADER-DG AdaptiveMeshFaceCoupling.
    """

    def __init__(self, solver, face_projections: FaceProjections):
        """The guards are those for the projection onto the faces."""
        super(AdaptiveMeshFaceCoupling, self).__init__(solver)
        self.guards = []
        self.face_projections = face_projections

    def get_action_set_name(self):
        return "exahype2_solvers_rkdg_actionsets_" + type(self).__name__

    __Template_TouchFaceFirstTime = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ( {{PREDICATES[PREDICATE_NO]}} ) {
    ::exahype2::dg::clearSolutionProjection(
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_PROJECTED_QUANTITIES}},
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data()
    );
  }
  {% endfor %}
"""

    __Template_TouchHangingFaceFirstTime = """
  fineGridFace{{FACE_LABEL}}.setNewTimeStamp(0, std::max( coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getNewTimeStamp(0), fineGridFace{{FACE_LABEL}}.getNewTimeStamp(0)) );
  fineGridFace{{FACE_LABEL}}.setNewTimeStamp(1, std::max( coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getNewTimeStamp(1), fineGridFace{{FACE_LABEL}}.getNewTimeStamp(1)) );
  fineGridFace{{FACE_LABEL}}.setOldTimeStamp(0, std::max( coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getOldTimeStamp(0), fineGridFace{{FACE_LABEL}}.getOldTimeStamp(0)) );
  fineGridFace{{FACE_LABEL}}.setOldTimeStamp(1, std::max( coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getOldTimeStamp(1), fineGridFace{{FACE_LABEL}}.getOldTimeStamp(1)) );
"""

    __Template_TouchHangingFaceLastTime = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ( {{PREDICATES[PREDICATE_NO]}} ) {
    ::exahype2::dg::restrictAndAccumulateProjectedFacePolynomial(
      marker,
      {{ORDER}},
      {{NUMBER_OF_PROJECTED_QUANTITIES}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.RestrictionMatrix1d,
      repositories::{{SOLVER_INSTANCE}}.QuadratureWeights1d,
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
      coarseGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(marker.getSelectedFaceNumber()).data()
    );

    const bool isLeftEntryOnCoarseFaceLabel = marker.getSelectedFaceNumber() >= DIMENSIONS;
    coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).setUpdated( isLeftEntryOnCoarseFaceLabel ? 0 : 1,true);
    coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).setUpdatedTimeStamp(
      isLeftEntryOnCoarseFaceLabel ? 0 : 1,
      std::max(
        coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ),
        fineGridFace{{FACE_LABEL}}.getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 )
      )
    );
  }
  {% endfor %}
"""

    def get_body_of_operation(self, operation_name):
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        d["PREDICATES"] = self.guards
        d["FACE_LABEL"] = self._solver._face_label.name
        d["NUMBER_OF_PROJECTED_QUANTITIES"] = (
            compute_number_of_face_projection_quantities(self.face_projections)
        )

        result = "\n"
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME
        ):
            result = jinja2.Template(self.__Template_TouchHangingFaceFirstTime).render(
                **d
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME
        ):
            result = jinja2.Template(self.__Template_TouchHangingFaceLastTime).render(
                **d
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            result = jinja2.Template(self.__Template_TouchFaceFirstTime).render(**d)
        return result

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


class DynamicAMRTransfer(AdaptiveMeshFaceCoupling):
    """
    Full inter-grid transfer for the dedicated DynamicAMR observer.

    This is the action set that changes the topology: it hands the committed
    refine/coarsen events to Peano via getGridControlEvents, so the observer's
    traversals trigger and realise the requests the RefinementCriterion
    accumulated during the preceding time step. It interpolates the nodal DG
    solution onto cells Peano creates and restricts it back onto cells Peano
    destroys; the Runge-Kutta estimates and the face projections are rebuilt by
    the next time step on the settled grid, so created cells and faces start
    from cleared buffers.

    No PDE update happens here, so the inherited hanging-face coupling is
    switched off. This action set mirrors the FV and ADER-DG DynamicAMRTransfer.
    """

    _Template_TouchCellFirstTime = """
  if (marker.hasBeenRefined() and not marker.willBeRefined()) {
    // The restriction accumulates the destroyed children into the parent, so a coarsening
    // parent's stale buffer is cleared before its children restrict into it on the descent.
    std::fill_n(
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      std::remove_reference_t<decltype(fineGridCell{{UNKNOWN_IDENTIFIER}})>::Cardinality,
      0.0
    );
  }
"""

    _Template_CreateCell = """
  ::toolbox::blockstructured::interpolateCell(
    marker,
    coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    repositories::{{SOLVER_INSTANCE}}.InterpolationMatrix1d,
    {{DG_ORDER}} + 1,
    {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
  );

  std::fill_n(
    fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data(),
    std::remove_reference_t<decltype(fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates)>::Cardinality,
    0.0
  );
  std::fill_n(
    fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
    std::remove_reference_t<decltype(fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination)>::Cardinality,
    0.0
  );

  fineGridCell{{CELL_LABEL_NAME}}.setTimeStamp(coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
  fineGridCell{{CELL_LABEL_NAME}}.setTimeStepSize(coarseGridCell{{CELL_LABEL_NAME}}.getTimeStepSize());

  // A created cell inherits its parent's update state and is marked for the
  // criterion pass, so the mesh-refinement sweeps in progress evaluate its data once.
  fineGridCell{{CELL_LABEL_NAME}}.setHasUpdated(coarseGridCell{{CELL_LABEL_NAME}}.getHasUpdated());
  fineGridCell{{CELL_LABEL_NAME}}.setRequiresCriterionEvaluation(true);
"""

    _Template_TouchCellLastTime = """
  // The criterion pass of this sweep runs before this transfer's cell event, so
  // any freshly created cell has already been evaluated.
  if (not marker.willBeRefined() and not marker.hasBeenRefined()) {
    fineGridCell{{CELL_LABEL_NAME}}.setRequiresCriterionEvaluation(false);
  }
"""

    _Template_DestroyCell = """
  ::toolbox::blockstructured::restrictCell(
    marker,
    coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    repositories::{{SOLVER_INSTANCE}}.RestrictionMatrix1d,
    repositories::{{SOLVER_INSTANCE}}.QuadratureWeights1d,
    {{DG_ORDER}} + 1,
    {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
  );

  coarseGridCell{{CELL_LABEL_NAME}}.setTimeStamp(
    std::max(coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp(), fineGridCell{{CELL_LABEL_NAME}}.getTimeStamp())
  );
  coarseGridCell{{CELL_LABEL_NAME}}.setTimeStepSize(
    std::max(coarseGridCell{{CELL_LABEL_NAME}}.getTimeStepSize(), fineGridCell{{CELL_LABEL_NAME}}.getTimeStepSize())
  );
"""

    _Template_CreateFace = """
  std::fill_n(
    fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
    std::remove_reference_t<decltype(fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection)>::Cardinality,
    0.0
  );
  std::fill_n(
    fineGridFace{{UNKNOWN_IDENTIFIER}}OldSolutionProjection.data(),
    std::remove_reference_t<decltype(fineGridFace{{UNKNOWN_IDENTIFIER}}OldSolutionProjection)>::Cardinality,
    0.0
  );
  std::fill_n(
    fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.data(),
    std::remove_reference_t<decltype(fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution)>::Cardinality,
    0.0
  );

  fineGridFace{{FACE_LABEL}}.setNewTimeStamp(0, coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
  fineGridFace{{FACE_LABEL}}.setNewTimeStamp(1, coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
  fineGridFace{{FACE_LABEL}}.setOldTimeStamp(0, coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
  fineGridFace{{FACE_LABEL}}.setOldTimeStamp(1, coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
"""

    def __init__(self, solver, face_projections: FaceProjections):
        super(DynamicAMRTransfer, self).__init__(solver, face_projections)

    def get_body_of_operation(self, operation_name):
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        d["CELL_LABEL_NAME"] = self._solver._cell_label.name
        d["FACE_LABEL"] = self._solver._face_label.name

        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            return jinja2.Template(self._Template_TouchCellFirstTime).render(**d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            return jinja2.Template(self._Template_TouchCellLastTime).render(**d)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_CELL:
            return jinja2.Template(self._Template_CreateCell).render(**d)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_DESTROY_CELL:
            return jinja2.Template(self._Template_DestroyCell).render(**d)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_FACE:
            return jinja2.Template(self._Template_CreateFace).render(**d)
        # The hanging-face coupling stays off in the realise sweeps: no solver reads the
        # projections here, and the next time step rebuilds them on the settled grid.
        return "\n"

    def get_body_of_getGridControlEvents(self):
        # Hand the committed refine/coarsen events back to Peano, so the trigger/realise
        # traversals change the topology here, on a stable grid.
        return "  return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();\n"

    def get_includes(self):
        return (
            super(DynamicAMRTransfer, self).get_includes()
            + """
#include "toolbox/blockstructured/AMRRoutines.h"
#include "exahype2/RefinementControlService.h"
"""
        )
