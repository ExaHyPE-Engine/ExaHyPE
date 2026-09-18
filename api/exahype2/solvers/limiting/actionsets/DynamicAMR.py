# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractLimiterActionSet import AbstractLimiterActionSet
from ..ProjectionMatrices import create_subcell_projection_matrices

from exahype2.solvers.LagrangeBasis import render_tensor_1

import jinja2
import peano4.solversteps


class DynamicAMR(AbstractLimiterActionSet):
    _Template_TouchCellFirstTime = """
  if (marker.hasBeenRefined() and not marker.willBeRefined()) {
    fineGridCell{{SOLVER_NAME}}CellLabel.setLimiterStatus(::exahype2::dg::LimiterStatus::Regular);
    fineGridCell{{SOLVER_NAME}}CellLabel.setPreviousLimiterStatus(::exahype2::dg::LimiterStatus::Regular);
    fineGridCell{{SOLVER_NAME}}CellLabel.setWasTroubledInPreviousStep(false);
    fineGridCell{{SOLVER_NAME}}CellLabel.setPredictorDiverged(false);
  }
"""

    _Template_CreateCell = """
  constexpr int NumberOfDofsPerAxis     = {{ORDER}} + 1;
  constexpr int NumberOfSubcellsPerAxis = 2 * {{ORDER}} + 1;
  constexpr int NumberOfData            = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
  constexpr int NumberOfSubcellValues   = DIMENSIONS == 2
      ? NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis * NumberOfData
      : NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis * NumberOfSubcellsPerAxis * NumberOfData;

  if (::exahype2::dg::runsSubcellScheme(coarseGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus())) {
    ::exahype2::amr::interpolateCell(
      marker,
      coarseGridCell{{SOLVER_NAME}}Subcell.data(),
      fineGridCell{{SOLVER_NAME}}Subcell.data(),
      NumberOfSubcellsPerAxis,
      NumberOfData
    );
    ::exahype2::amr::interpolateCell(
      marker,
      coarseGridCell{{SOLVER_NAME}}SubcellOld.data(),
      fineGridCell{{SOLVER_NAME}}SubcellOld.data(),
      NumberOfSubcellsPerAxis,
      NumberOfData
    );
  } else {
    static constexpr double DgToFvMatrix[] = {{DG_TO_FV_MATRIX}};
    const double*           dgToFvMatrixPerAxis[DIMENSIONS];
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      dgToFvMatrixPerAxis[axis] = DgToFvMatrix;
    }
    ::exahype2::amr::applyTensorProductOrderInvariant(
      fineGridCell{{SOLVER_NAME}}SubcellOld.data(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      dgToFvMatrixPerAxis,
      NumberOfDofsPerAxis,
      NumberOfSubcellsPerAxis,
      NumberOfData
    );
    std::copy_n(
      fineGridCell{{SOLVER_NAME}}SubcellOld.data(),
      NumberOfSubcellValues,
      fineGridCell{{SOLVER_NAME}}Subcell.data()
    );
  }

  fineGridCell{{SOLVER_NAME}}CellLabel.setLimiterStatus(
    coarseGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus()
  );
  fineGridCell{{SOLVER_NAME}}CellLabel.setPreviousLimiterStatus(
    coarseGridCell{{SOLVER_NAME}}CellLabel.getPreviousLimiterStatus()
  );
  fineGridCell{{SOLVER_NAME}}CellLabel.setWasTroubledInPreviousStep(
    coarseGridCell{{SOLVER_NAME}}CellLabel.getWasTroubledInPreviousStep()
  );
  fineGridCell{{SOLVER_NAME}}CellLabel.setPredictorDiverged(
    coarseGridCell{{SOLVER_NAME}}CellLabel.getPredictorDiverged()
  );
"""

    _Template_DestroyCell = """
  constexpr int NumberOfDofsPerAxis     = {{ORDER}} + 1;
  constexpr int NumberOfSubcellsPerAxis = 2 * {{ORDER}} + 1;
  constexpr int NumberOfData            = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};

  const {{SOLUTION_STORAGE_PRECISION}}* subcellSource;
  const {{SOLUTION_STORAGE_PRECISION}}* subcellOldSource;
  if (::exahype2::dg::runsSubcellScheme(fineGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus())) {
    subcellSource    = fineGridCell{{SOLVER_NAME}}Subcell.data();
    subcellOldSource = fineGridCell{{SOLVER_NAME}}SubcellOld.data();
  } else {
    // A coarsening parent can combine regular and limited children, while a
    // regular child's subcell buffers need not hold its accepted solution. The
    // child is about to be destroyed, so its old subcell buffer can hold the
    // projection without an additional patch-sized stack allocation.
    static constexpr double DgToFvMatrix[] = {{DG_TO_FV_MATRIX}};
    const double*           dgToFvMatrixPerAxis[DIMENSIONS];
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      dgToFvMatrixPerAxis[axis] = DgToFvMatrix;
    }
    ::exahype2::amr::applyTensorProductOrderInvariant(
      fineGridCell{{SOLVER_NAME}}SubcellOld.data(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      dgToFvMatrixPerAxis,
      NumberOfDofsPerAxis,
      NumberOfSubcellsPerAxis,
      NumberOfData
    );
    subcellSource    = fineGridCell{{SOLVER_NAME}}SubcellOld.data();
    subcellOldSource = fineGridCell{{SOLVER_NAME}}SubcellOld.data();
  }

  // The children of a coarsening parent restrict together once the whole
  // sibling set is staged, so the summation order cannot depend on the order
  // in which the traversal destroys them.
  const auto childIndex = peano4::dLinearised(
    marker.getRelativePositionWithinFatherCell(), peano4::sfc::ActiveCurve::SubdivisionFactor
  );
  const auto siblingSetComplete =
    _subcellRestrictionStage.stage(coarseGridCell{{SOLVER_NAME}}Subcell.data(), childIndex, subcellSource);
  _subcellOldRestrictionStage.stage(coarseGridCell{{SOLVER_NAME}}SubcellOld.data(), childIndex, subcellOldSource);
  if (siblingSetComplete) {
    const {{SOLUTION_STORAGE_PRECISION}}* stagedChildren[peano4::sfc::ActiveCurve::ChildrenPerCell];
    for (int child = 0; child < peano4::sfc::ActiveCurve::ChildrenPerCell; child++) {
      stagedChildren[child] = _subcellRestrictionStage.child(coarseGridCell{{SOLVER_NAME}}Subcell.data(), child);
    }
    ::exahype2::amr::restrictCellFromChildren(
      coarseGridCell{{SOLVER_NAME}}Subcell.data(),
      stagedChildren,
      NumberOfSubcellsPerAxis,
      NumberOfData
    );
    _subcellRestrictionStage.discard(coarseGridCell{{SOLVER_NAME}}Subcell.data());
    for (int child = 0; child < peano4::sfc::ActiveCurve::ChildrenPerCell; child++) {
      stagedChildren[child] = _subcellOldRestrictionStage.child(coarseGridCell{{SOLVER_NAME}}SubcellOld.data(), child);
    }
    ::exahype2::amr::restrictCellFromChildren(
      coarseGridCell{{SOLVER_NAME}}SubcellOld.data(),
      stagedChildren,
      NumberOfSubcellsPerAxis,
      NumberOfData
    );
    _subcellOldRestrictionStage.discard(coarseGridCell{{SOLVER_NAME}}SubcellOld.data());
  }

  coarseGridCell{{SOLVER_NAME}}CellLabel.setLimiterStatus(
    ::exahype2::dg::mergeLimiterStatus(
      coarseGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus(),
      fineGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus()
    )
  );
  coarseGridCell{{SOLVER_NAME}}CellLabel.setPreviousLimiterStatus(
    ::exahype2::dg::mergeLimiterStatus(
      coarseGridCell{{SOLVER_NAME}}CellLabel.getPreviousLimiterStatus(),
      fineGridCell{{SOLVER_NAME}}CellLabel.getPreviousLimiterStatus()
    )
  );
  coarseGridCell{{SOLVER_NAME}}CellLabel.setWasTroubledInPreviousStep(
    coarseGridCell{{SOLVER_NAME}}CellLabel.getWasTroubledInPreviousStep()
    or fineGridCell{{SOLVER_NAME}}CellLabel.getWasTroubledInPreviousStep()
  );
  coarseGridCell{{SOLVER_NAME}}CellLabel.setPredictorDiverged(
    coarseGridCell{{SOLVER_NAME}}CellLabel.getPredictorDiverged()
    or fineGridCell{{SOLVER_NAME}}CellLabel.getPredictorDiverged()
  );
"""

    def __init__(self, solver):
        super().__init__(solver)
        dg_to_fv, _ = create_subcell_projection_matrices(self._solver._basis)
        self._dg_to_fv_matrix = render_tensor_1(
            [value for row in dg_to_fv for value in row]
        )

    def get_body_of_operation(self, operation_name):
        dictionary = {}
        self._solver._init_dictionary_with_default_parameters(dictionary)
        self._solver.add_entries_to_text_replacement_dictionary(dictionary)
        dictionary["DG_TO_FV_MATRIX"] = self._dg_to_fv_matrix

        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            return jinja2.Template(self._Template_TouchCellFirstTime).render(
                **dictionary
            )
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_CELL:
            return jinja2.Template(self._Template_CreateCell).render(**dictionary)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_DESTROY_CELL:
            return jinja2.Template(self._Template_DestroyCell).render(**dictionary)
        return ""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_attributes(self):
        subcells_per_axis = 2 * self._solver._order + 1
        number_of_data = (
            self._solver._variable_shortcuts.unknowns
            + self._solver._variable_shortcuts.auxiliary_variables
        )
        values_per_child = (
            f"(DIMENSIONS == 2 ? {subcells_per_axis * subcells_per_axis}"
            f" : {subcells_per_axis ** 3}) * {number_of_data}"
        )
        precision = self._solver._solution_persistent_storage_precision
        return (
            super().get_attributes()
            + f"""
    ::exahype2::amr::ChildDataStage<{precision}>
      _subcellRestrictionStage{{peano4::sfc::ActiveCurve::ChildrenPerCell, {values_per_child}}};
    ::exahype2::amr::ChildDataStage<{precision}>
      _subcellOldRestrictionStage{{peano4::sfc::ActiveCurve::ChildrenPerCell, {values_per_child}}};
"""
        )

    def get_includes(self):
        return (
            super().get_includes()
            + """
#include "exahype2/dg/LimiterStatus.h"
#include "peano4/Constants.h"
#include "peano4/sfc/ActiveCurve.h"
#include "exahype2/amr/AMRRoutines.h"
#include "exahype2/amr/ChildDataStage.h"
"""
        )
