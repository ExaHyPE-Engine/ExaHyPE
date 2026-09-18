# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import jinja2

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet
from .UpdateSolution import UpdateSolution


# Subfaces below one persistent face remain on one observer throughout restriction.
_TemplateStageHangingFaceTrace = """
    constexpr int NodesPerAxis = {{DG_ORDER}} + 1;
    constexpr int NumberOfSubfaces = peano4::power(peano4::sfc::ActiveCurve::SubdivisionFactor, DIMENSIONS - 1);
    constexpr int EntriesPerFaceSide =
      facedata::{{UNKNOWN_IDENTIFIER}}EstimateProjection::Cardinality / 2;
    const int restrictedSide = marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
    auto* const restrictedCoarseHalf =
      coarseGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(marker.getSelectedFaceNumber()).data()
      + restrictedSide * EntriesPerFaceSide;
#if DIMENSIONS == 2
    const int subfaceIndex = marker.getRelativePositionWithinFatherFace()(0);
#else
    const int subfaceIndex = marker.getRelativePositionWithinFatherFace()(1) * peano4::sfc::ActiveCurve::SubdivisionFactor
      + marker.getRelativePositionWithinFatherFace()(0);
#endif
    bool restrictionCompleted = false;
    if (_projectionRestrictionStage.stage(
          restrictedCoarseHalf,
          subfaceIndex,
          fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data() + restrictedSide * EntriesPerFaceSide
        )) {
      const double* stagedSubfaces[NumberOfSubfaces];
      for (int subface = 0; subface < NumberOfSubfaces; subface++) {
        stagedSubfaces[subface] = _projectionRestrictionStage.child(restrictedCoarseHalf, subface);
      }
      const double* restrictionMatrixPerInterval[peano4::sfc::ActiveCurve::SubdivisionFactor];
      for (int interval = 0; interval < peano4::sfc::ActiveCurve::SubdivisionFactor; interval++) {
        restrictionMatrixPerInterval[interval] =
          kernels::{{SOLVER_NAME}}::DGMatrices::WeightedRestrictionMatrix1D + interval * NodesPerAxis * NodesPerAxis;
      }
      ::exahype2::amr::restrictFaceFromChildren(
        restrictedCoarseHalf,
        stagedSubfaces,
        restrictionMatrixPerInterval,
        NodesPerAxis,
        {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
      );
      _projectionRestrictionStage.discard(restrictedCoarseHalf);
      restrictionCompleted = true;
    }

    coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).setUpdated(restrictedSide, true);
    coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).setUpdatedTimeStamp(
      restrictedSide,
      fineGridFace{{FACE_LABEL}}.getUpdatedTimeStamp(restrictedSide)
    );

    if (restrictionCompleted) {
      constexpr int FluxEntriesPerFaceSide =
        facedata::{{UNKNOWN_IDENTIFIER}}RiemannSolution::Cardinality / 2;
      const int direction = marker.getSelectedFaceNumber() % DIMENSIONS;
      const double* const coarseLeftTrace =
        coarseGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(marker.getSelectedFaceNumber()).data();
      tarch::la::Vector<DIMENSIONS, double> coarseFaceCentre = marker.getX();
      tarch::la::Vector<DIMENSIONS, double> coarseCellSize   = marker.getH();
      for (int axis = 0; axis < DIMENSIONS; axis++) {
        if (axis != direction) {
          coarseFaceCentre(axis) += (
            0.5 * (peano4::sfc::ActiveCurve::SubdivisionFactor - 1)
            - marker.getRelativePositionWithinFatherCell()(axis)
          ) * marker.getH()(axis);
        }
        coarseCellSize(axis) *= peano4::sfc::ActiveCurve::SubdivisionFactor;
      }
      {{COMPUTE_TIME_STEP_SIZE}}
      ::exahype2::dg::evaluateFluxOnFace<
        {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}} + 1,
        {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {% if FLUX_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
        {{ "true" if USE_DIFFUSIVE_FLUX else "false" }}
      >(
        coarseGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(marker.getSelectedFaceNumber()).data()
          + restrictedSide * FluxEntriesPerFaceSide,
        coarseLeftTrace,
        coarseLeftTrace + EntriesPerFaceSide,
        restrictedSide,
        kernels::{{SOLVER_NAME}}::Quadrature::QuadraturePoints1D,
        coarseFaceCentre,
        coarseCellSize,
        coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getUpdatedTimeStamp(restrictedSide),
        timeStepSize,
        direction
      );
    }
"""


class AdaptiveMeshFaceCoupling(AbstractRungeKuttaDGActionSet):
    """
    Restricts fine-cell traces onto coarse faces.

    Copies coarse time stamps to hanging faces.
    """

    def __init__(self, solver):
        super(AdaptiveMeshFaceCoupling, self).__init__(solver)
        self.guards = []

    def get_action_set_name(self):
        return "exahype2_solvers_rkdg_actionsets_" + type(self).__name__

    __Template_TouchFaceFirstTime = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ( {{PREDICATES[PREDICATE_NO]}} ) {
    std::fill_n(
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
      std::remove_reference_t<decltype(fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection)>::Cardinality,
      0.0
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
    {{STAGE_HANGING_FACE_TRACE}}
  }
  {% endfor %}
"""

    def get_body_of_operation(self, operation_name):
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        d["PREDICATES"] = self.guards
        d["FACE_LABEL"] = self._solver._face_label.name
        d["STAGE_HANGING_FACE_TRACE"] = jinja2.Template(
            _TemplateStageHangingFaceTrace
        ).render(**d)

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

    def get_attributes(self):
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        return (
            super(AdaptiveMeshFaceCoupling, self).get_attributes()
            + f"""
    ::exahype2::amr::ChildDataStage<double> _projectionRestrictionStage{{
      peano4::power(peano4::sfc::ActiveCurve::SubdivisionFactor, DIMENSIONS - 1),
      facedata::{d["UNKNOWN_IDENTIFIER"]}EstimateProjection::Cardinality / 2}};
"""
        )

    def get_includes(self):
        return (
            super(AdaptiveMeshFaceCoupling, self).get_includes()
            + """
#include "peano4/Constants.h"
#include "peano4/sfc/ActiveCurve.h"
#include "exahype2/amr/AMRRoutines.h"
#include "exahype2/amr/ChildDataStage.h"
"""
        )

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


class DynamicAMRTransfer(AdaptiveMeshFaceCoupling):
    """
    Transfers the solution and Runge-Kutta stages between mesh levels.

    Restriction combines siblings in a fixed order.
    """

    _Template_CreateCell = """
  ::exahype2::amr::interpolateCell(
    marker,
    coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    kernels::{{SOLVER_NAME}}::DGMatrices::InterpolationMatrix1D,
    {{DG_ORDER}} + 1,
    {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
  );

  if (repositories::{{SOLVER_INSTANCE}}.getSolverState()
      != {{SOLVER_NAME}}::SolverState::GridInitialisation) {
    for (int stage = 0; stage < {{RK_STEPS}}; stage++) {
      ::exahype2::amr::interpolateCell(
        marker,
        coarseGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data()
          + stage * {{NUMBER_OF_DOFS_PER_CELL}} * {{NUMBER_OF_UNKNOWNS}},
        fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data()
          + stage * {{NUMBER_OF_DOFS_PER_CELL}} * {{NUMBER_OF_UNKNOWNS}},
        kernels::{{SOLVER_NAME}}::DGMatrices::InterpolationMatrix1D,
        {{DG_ORDER}} + 1,
        {{NUMBER_OF_UNKNOWNS}}
      );
    }
    ::exahype2::amr::interpolateCell(
      marker,
      coarseGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
      kernels::{{SOLVER_NAME}}::DGMatrices::InterpolationMatrix1D,
      {{DG_ORDER}} + 1,
      {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
    );
  } else {
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
  }

  fineGridCell{{CELL_LABEL_NAME}}.setTimeStamp(coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
  fineGridCell{{CELL_LABEL_NAME}}.setTimeStepSize(coarseGridCell{{CELL_LABEL_NAME}}.getTimeStepSize());

  fineGridCell{{CELL_LABEL_NAME}}.setHasUpdated(coarseGridCell{{CELL_LABEL_NAME}}.getHasUpdated());
  fineGridCell{{CELL_LABEL_NAME}}.setRequiresCriterionEvaluation(true);
"""

    _Template_TouchCellLastTime = """
  const int stage = repositories::{{SOLVER_INSTANCE}}.getRkStageAfterDynamicMeshRefinement();
  bool requiresDataTransfer = marker.hasBeenRefined() and not marker.willBeRefined();
  for (int face = 0; face < peano4::FacesPerCell; face++) {
    const int ownSide = face < DIMENSIONS ? 1 : 0;
    requiresDataTransfer = requiresDataTransfer
      or (not fineGridFaces{{FACE_LABEL}}(face).getIsHanging()
          and fineGridFaces{{FACE_LABEL}}(face).getRequiresDataTransfer(ownSide));
  }
  if (not marker.willBeRefined()
      and repositories::{{SOLVER_INSTANCE}}.getSolverState()
        != {{SOLVER_NAME}}::SolverState::GridInitialisation) {
    const double timeStepSize = fineGridCell{{CELL_LABEL_NAME}}.getTimeStepSize();
    const double stageTimeStamp = fineGridCell{{CELL_LABEL_NAME}}.getTimeStamp()
      + kernels::{{SOLVER_NAME}}::ButcherTableau::RelativeTimeStepSizes[stage] * timeStepSize;
    if (requiresDataTransfer) {
      double* stageRhs = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data()
        + stage * {{NUMBER_OF_DOFS_PER_CELL}} * {{NUMBER_OF_UNKNOWNS}};
      ::exahype2::dg::rk::volumeIntegral<
        {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}}+1,
        {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{"true" if IS_LINEAR else "false"}},
        {% if FLUX_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
        {{ "true" if USE_DIFFUSIVE_FLUX else "false" }},
        {% if SOURCE_TERM_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %},
        {% if NCP_IMPLEMENTATION!="<none>" %}true{% else %}false{% endif %}
      >(
        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(), stageRhs,
        kernels::{{SOLVER_NAME}}::Quadrature::QuadraturePoints1D,
        kernels::{{SOLVER_NAME}}::DGMatrices::MassMatrixDiagonal1D,
        kernels::{{SOLVER_NAME}}::DGMatrices::StiffnessOperator1D,
        kernels::{{SOLVER_NAME}}::DGMatrices::DerivativeOperator1D,
        marker.getX(), marker.getH(), stageTimeStamp, timeStepSize
      );
    }

    {{PROJECT_STAGE_ONTO_FACES}}
  }

  if (not marker.willBeRefined() and not marker.hasBeenRefined()) {
    fineGridCell{{CELL_LABEL_NAME}}.setRequiresCriterionEvaluation(false);
  }
"""

    _Template_TouchFaceFirstTime = """
  std::fill_n(
    fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
    std::remove_reference_t<decltype(fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection)>::Cardinality,
    0.0
  );
"""

    _Template_TouchHangingFaceFirstTime = _Template_TouchFaceFirstTime

    _Template_TouchHangingFaceLastTime = """
  {{STAGE_HANGING_FACE_TRACE}}
"""

    # The load balancer keeps siblings that can coarsen on one rank.
    _Template_DestroyCell = """
  constexpr int NodesPerAxis = {{DG_ORDER}} + 1;
  const int linearisedChildPosition = peano4::dLinearised(
    marker.getRelativePositionWithinFatherCell(), peano4::sfc::ActiveCurve::SubdivisionFactor
  );
  const double* restrictionMatrixPerInterval[peano4::sfc::ActiveCurve::SubdivisionFactor];
  for (int interval = 0; interval < peano4::sfc::ActiveCurve::SubdivisionFactor; interval++) {
    restrictionMatrixPerInterval[interval] =
      kernels::{{SOLVER_NAME}}::DGMatrices::WeightedRestrictionMatrix1D + interval * NodesPerAxis * NodesPerAxis;
  }
  const double* childValues[peano4::sfc::ActiveCurve::ChildrenPerCell];

  if (_solutionRestrictionStage.stage(
        coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
        linearisedChildPosition,
        fineGridCell{{UNKNOWN_IDENTIFIER}}.data()
      )) {
    for (int childIndex = 0; childIndex < peano4::sfc::ActiveCurve::ChildrenPerCell; childIndex++) {
      childValues[childIndex] = _solutionRestrictionStage.child(coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(), childIndex);
    }
    ::exahype2::amr::restrictCellFromChildren(
      coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      childValues,
      restrictionMatrixPerInterval,
      NodesPerAxis,
      {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
    );
    _solutionRestrictionStage.discard(coarseGridCell{{UNKNOWN_IDENTIFIER}}.data());
  }

  if (_rhsEstimatesRestrictionStage.stage(
        coarseGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data(),
        linearisedChildPosition,
        fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data()
      )) {
    for (int stage = 0; stage < {{RK_STEPS}}; stage++) {
      const int stageOffset = stage * {{NUMBER_OF_DOFS_PER_CELL}} * {{NUMBER_OF_UNKNOWNS}};
      for (int childIndex = 0; childIndex < peano4::sfc::ActiveCurve::ChildrenPerCell; childIndex++) {
        childValues[childIndex] =
          _rhsEstimatesRestrictionStage.child(coarseGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data(), childIndex)
          + stageOffset;
      }
      ::exahype2::amr::restrictCellFromChildren(
        coarseGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data() + stageOffset,
        childValues,
        restrictionMatrixPerInterval,
        NodesPerAxis,
        {{NUMBER_OF_UNKNOWNS}}
      );
    }
    _rhsEstimatesRestrictionStage.discard(coarseGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data());
  }

  if (_linearCombinationRestrictionStage.stage(
        coarseGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
        linearisedChildPosition,
        fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data()
      )) {
    for (int childIndex = 0; childIndex < peano4::sfc::ActiveCurve::ChildrenPerCell; childIndex++) {
      childValues[childIndex] =
        _linearCombinationRestrictionStage.child(coarseGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(), childIndex);
    }
    ::exahype2::amr::restrictCellFromChildren(
      coarseGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data(),
      childValues,
      restrictionMatrixPerInterval,
      NodesPerAxis,
      {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
    );
    _linearCombinationRestrictionStage.discard(coarseGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data());
  }

  for (int face = 0; face < peano4::FacesPerCell; face++) {
    coarseGridFaces{{FACE_LABEL}}(face).setRequiresDataTransfer(face < DIMENSIONS ? 1 : 0, true);
  }

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
    fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.data(),
    std::remove_reference_t<decltype(fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution)>::Cardinality,
    0.0
  );

  fineGridFace{{FACE_LABEL}}.setNewTimeStamp(0, coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
  fineGridFace{{FACE_LABEL}}.setNewTimeStamp(1, coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
  fineGridFace{{FACE_LABEL}}.setOldTimeStamp(0, coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
  fineGridFace{{FACE_LABEL}}.setOldTimeStamp(1, coarseGridCell{{CELL_LABEL_NAME}}.getTimeStamp());
"""

    def get_body_of_operation(self, operation_name):
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        d["CELL_LABEL_NAME"] = self._solver._cell_label.name
        d["FACE_LABEL"] = self._solver._face_label.name
        d["PROJECT_STAGE_ONTO_FACES"] = jinja2.Template(UpdateSolution._PROJECT).render(
            **d
        )
        d["STAGE_HANGING_FACE_TRACE"] = jinja2.Template(
            _TemplateStageHangingFaceTrace
        ).render(**d)

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
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            return jinja2.Template(self._Template_TouchFaceFirstTime).render(**d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME
        ):
            return jinja2.Template(self._Template_TouchHangingFaceFirstTime).render(**d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME
        ):
            return jinja2.Template(self._Template_TouchHangingFaceLastTime).render(**d)
        return "\n"

    def get_body_of_getGridControlEvents(self):
        return "  return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();\n"

    def get_attributes(self):
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        unknown_identifier = d["UNKNOWN_IDENTIFIER"]
        return (
            super(DynamicAMRTransfer, self).get_attributes()
            + f"""
    ::exahype2::amr::ChildDataStage<double> _solutionRestrictionStage{{
      peano4::sfc::ActiveCurve::ChildrenPerCell,
      celldata::{unknown_identifier}::Cardinality}};
    ::exahype2::amr::ChildDataStage<double> _rhsEstimatesRestrictionStage{{
      peano4::sfc::ActiveCurve::ChildrenPerCell,
      celldata::{unknown_identifier}RhsEstimates::Cardinality}};
    ::exahype2::amr::ChildDataStage<double> _linearCombinationRestrictionStage{{
      peano4::sfc::ActiveCurve::ChildrenPerCell,
      celldata::{unknown_identifier}LinearCombination::Cardinality}};
"""
        )

    def get_includes(self):
        return (
            super(DynamicAMRTransfer, self).get_includes()
            + """
#include "exahype2/RefinementControlService.h"
#include "kernels/"""
            + self._solver._name
            + """/ButcherTableau.h"
"""
        )
