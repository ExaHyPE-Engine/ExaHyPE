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
    def __init__(self, solver, face_projections: FaceProjections):
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
    const int facePosition = marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
    // Across a level jump of two or more, this face carries staged subfaces of
    // its own. They reduce here, so the chained restriction is complete before
    // this face stages itself.
    if (_projectionRestrictionStage.hasFrame(fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data() + facePosition)) {
      ::exahype2::amr::restrictHaloLayerFromStagedSubfaces(
        _projectionRestrictionStage,
        fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data() + facePosition,
        fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
        repositories::{{SOLVER_INSTANCE}}.RestrictionMatrix1D.data(),
        repositories::{{SOLVER_INSTANCE}}.QuadratureWeights1D,
        marker.getSelectedFaceNumber() % DIMENSIONS,
        {{ORDER}} + 1,
        {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
      );
    }
    // The subface's trace half stages for the gathered restriction, which the
    // persistent face reduces in one order-invariant pass when the sweep leaves
    // it, so the restricted trace depends neither on the visit order nor on the
    // decomposition.
    ::exahype2::amr::stageHaloLayerForGatheredRestriction(
      marker,
      _projectionRestrictionStage,
      coarseGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(marker.getSelectedFaceNumber()).data() + facePosition,
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
      facePosition,
      {{ORDER}} + 1,
      {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
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

    __Template_TouchFaceLastTime = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ( {{PREDICATES[PREDICATE_NO]}} ) {
    // By this face's last touch every hanging subface has staged its trace
    // half, so a frame reduces here in one order-invariant pass.
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      if (_projectionRestrictionStage.hasFrame(fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data() + facePosition)) {
        ::exahype2::amr::restrictHaloLayerFromStagedSubfaces(
          _projectionRestrictionStage,
          fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data() + facePosition,
          fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
          repositories::{{SOLVER_INSTANCE}}.RestrictionMatrix1D.data(),
          repositories::{{SOLVER_INSTANCE}}.QuadratureWeights1D,
          marker.getSelectedFaceNumber() % DIMENSIONS,
          {{ORDER}} + 1,
          {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
        );
      }
    }
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
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
        ):
            result = jinja2.Template(self.__Template_TouchFaceLastTime).render(**d)
        return result

    def get_attributes(self):
        assert (
            compute_number_of_face_projection_quantities(self.face_projections) == 1
        ), "The gathered trace restriction supports one projected quantity per face."
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        return f"""
    ::exahype2::amr::ChildDataStage<double> _projectionRestrictionStage{{
      peano4::Power<peano4::sfc::ActiveCurve::SubdivisionFactor, DIMENSIONS - 1>,
      facedata::{d["UNKNOWN_IDENTIFIER"]}EstimateProjection::Cardinality / 2}};
"""

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
    _Template_CreateCell = """
  ::exahype2::amr::interpolateCell(
    marker,
    coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    repositories::{{SOLVER_INSTANCE}}.InterpolationMatrix1D.data(),
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
  // The destroyed child stages its solution; the completing sibling reduces the
  // whole set onto the parent in one order-invariant pass, so the restriction
  // cannot depend on the order the traversal destroys the children. The set
  // always completes on one rank: The load balancer refuses to place a rank
  // boundary inside a cell that has a leaf child, so a family that can coarsen
  // is never split. ~ChildDataStage catches a violation of that.
  const int linearisedChildPosition = peano4::dLinearised(
    marker.getRelativePositionWithinFatherCell(), peano4::sfc::ActiveCurve::SubdivisionFactor
  );
  if (_cellRestrictionStage.stage(
        coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
        linearisedChildPosition,
        fineGridCell{{UNKNOWN_IDENTIFIER}}.data()
      )) {
    constexpr int NodesPerAxis      = {{DG_ORDER}} + 1;
    constexpr int SubdivisionFactor = peano4::sfc::ActiveCurve::SubdivisionFactor;
    // The gathered reduction expects one matrix per child interval with entry
    // [fine * NodesPerAxis + coarse] that already carries the quadrature
    // weighting, while the solver stores the operator packed per interval as
    // [coarse][fine] with the weights separate.
    double        weightedRestrictionMatrix[SubdivisionFactor][NodesPerAxis * NodesPerAxis];
    const double* restrictionMatrixPerInterval[SubdivisionFactor];
    for (int interval = 0; interval < SubdivisionFactor; interval++) {
      restrictionMatrixPerInterval[interval] = weightedRestrictionMatrix[interval];
      for (int coarseNode = 0; coarseNode < NodesPerAxis; coarseNode++) {
        for (int fineNode = 0; fineNode < NodesPerAxis; fineNode++) {
          weightedRestrictionMatrix[interval][fineNode * NodesPerAxis + coarseNode] =
            repositories::{{SOLVER_INSTANCE}}.RestrictionMatrix1D
              [interval * NodesPerAxis * NodesPerAxis + coarseNode * NodesPerAxis + fineNode]
            * repositories::{{SOLVER_INSTANCE}}.QuadratureWeights1D[fineNode]
            / repositories::{{SOLVER_INSTANCE}}.QuadratureWeights1D[coarseNode];
        }
      }
    }
    const double* childValues[peano4::sfc::ActiveCurve::ChildrenPerCell];
    for (int childIndex = 0; childIndex < peano4::sfc::ActiveCurve::ChildrenPerCell; childIndex++) {
      childValues[childIndex] = _cellRestrictionStage.child(coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(), childIndex);
    }
    ::exahype2::amr::restrictCellFromChildren(
      coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      childValues,
      restrictionMatrixPerInterval,
      NodesPerAxis,
      {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
    );
    _cellRestrictionStage.discard(coarseGridCell{{UNKNOWN_IDENTIFIER}}.data());
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
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            return jinja2.Template(self._Template_TouchCellLastTime).render(**d)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_CELL:
            return jinja2.Template(self._Template_CreateCell).render(**d)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_DESTROY_CELL:
            return jinja2.Template(self._Template_DestroyCell).render(**d)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_FACE:
            return jinja2.Template(self._Template_CreateFace).render(**d)
        # The hanging-face coupling stays off in the realise sweeps: No solver reads the
        # projections here, and the next time step rebuilds them on the settled grid.
        return "\n"

    def get_body_of_getGridControlEvents(self):
        # The dedicated DynamicAMR traversal consumes the committed batch only
        # after the solver sweeps have left the grid stable.
        return "  return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();\n"

    def get_attributes(self):
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        return (
            super(DynamicAMRTransfer, self).get_attributes()
            + f"""
    ::exahype2::amr::ChildDataStage<double> _cellRestrictionStage{{
      peano4::sfc::ActiveCurve::ChildrenPerCell,
      celldata::{d["UNKNOWN_IDENTIFIER"]}::Cardinality}};
"""
        )

    def get_includes(self):
        return (
            super(DynamicAMRTransfer, self).get_includes()
            + """
#include "exahype2/RefinementControlService.h"
"""
        )
