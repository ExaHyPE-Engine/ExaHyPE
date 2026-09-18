# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractLimiterActionSet import AbstractLimiterActionSet
from ..Labels import (
    cell_status_to_face_snippet,
    hanging_face_status_from_coarse_snippet,
    hanging_face_status_reduce_snippet,
    reset_refined_face_status_snippet,
)
from ..ProjectionMatrices import create_subcell_projection_matrices

from exahype2.solvers.LagrangeBasis import render_tensor_1

import jinja2
import peano4.solversteps


class SpreadDomain(AbstractLimiterActionSet):
    """Expands the finite-volume domain around troubled cells."""

    _Template_TouchFaceFirstTime = """
  if ({{FACE_GUARD}}) {
    for (int faceSide = 0; faceSide < 2; faceSide++) {
      fineGridFace{{SOLVER_NAME}}FaceLabel.setLimiterStatusSnapshot(
        faceSide,
        fineGridFace{{SOLVER_NAME}}FaceLabel.getLimiterStatus(faceSide)
      );
    }
{{RESET_REFINED_FACE_STATUS}}
  }
"""

    _Template_TouchHangingFaceFirstTime = """
  if ({{FACE_GUARD}}) {
{{HANGING_FACE_STATUS_FROM_COARSE}}
  }
"""

    _Template_TouchHangingFaceLastTime = """
  if ({{FACE_GUARD}}) {
{{HANGING_FACE_STATUS_REDUCE}}
  }
"""

    _Template_TouchCellFirstTime = """
  if ({{CELL_GUARD}}) {
    auto status               = fineGridCell{{SOLVER_NAME}}CellLabel.getLimiterStatus();
    bool hasTroubledNeighbour = false;
    for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {
      const auto neighbourSide = faceNumber < DIMENSIONS ? 0 : 1;
      const auto neighbourStatus =
        fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getLimiterStatusSnapshot(neighbourSide);
      hasTroubledNeighbour = hasTroubledNeighbour or neighbourStatus == ::exahype2::dg::LimiterStatus::Troubled;
      status = ::exahype2::dg::mergeLimiterStatus(status, ::exahype2::dg::spreadLimiterStatus(neighbourStatus));
    }
    fineGridCell{{SOLVER_NAME}}CellLabel.setLimiterStatus(status);
    if (hasTroubledNeighbour) {
      // The directly limited ring remains on the subcell scheme for one
      // additional step together with its troubled cell.
      fineGridCell{{SOLVER_NAME}}CellLabel.setWasTroubledInPreviousStep(true);
    }
{{PROJECT_STATUS_TO_FACES}}
    if (status != ::exahype2::dg::LimiterStatus::Regular) {
      constexpr int NumberOfDofsPerAxis     = {{ORDER}} + 1;
      constexpr int NumberOfSubcellsPerAxis = 2 * {{ORDER}} + 1;
      constexpr int NumberOfData            = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
      static constexpr {{SOLUTION_STORAGE_PRECISION}} DgToFvMatrix[] = {{DG_TO_FV_MATRIX}};

      // The projection of the accepted polynomial overwrites only the subgrid of
      // a cell entering the scheme; a cell entering within the first time step
      // keeps the subcells that grid initialisation sampled.
      const bool holdsInitialSubcells =
        fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp() <= fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
      if (not ::exahype2::dg::runsSubcellScheme(fineGridCell{{SOLVER_NAME}}CellLabel.getPreviousLimiterStatus())
          and not holdsInitialSubcells) {
        ::exahype2::dg::projectTensorProduct<DIMENSIONS, NumberOfDofsPerAxis, NumberOfSubcellsPerAxis, NumberOfData>(
          fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
          fineGridCell{{SOLVER_NAME}}SubcellOld.data(),
          DgToFvMatrix
        );
      }

      // The recompute reads its halo from the previous-solution faces. Only
      // those travel between the spreading and the recompute sweep, so the
      // cell writes its boundary layers there.
      {{SOLUTION_STORAGE_PRECISION}}* haloFaces[2 * DIMENSIONS];
      for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {
        haloFaces[faceNumber] = fineGridFaces{{SOLVER_NAME}}SubcellOld(faceNumber).data();
      }
      ::exahype2::projectPatch<
        NumberOfSubcellsPerAxis, {{HALO_WIDTH}}, NumberOfData, {{SOLUTION_STORAGE_PRECISION}}
      >(fineGridCell{{SOLVER_NAME}}SubcellOld.data(), haloFaces);
      for (int faceNumber = 0; faceNumber < 2 * DIMENSIONS; faceNumber++) {
        // A boundary face has no neighbour to deliver the outer halo layers, so
        // the user's boundary conditions derive them from the inner ones.
        if (not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[faceNumber % DIMENSIONS]
            and fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getBoundary()) {
          ::exahype2::fv::applyBoundaryConditionsOnFace<
            {{FULL_QUALIFIED_SOLVER_NAME}},
            NumberOfSubcellsPerAxis,
            {{HALO_WIDTH}},
            {{NUMBER_OF_UNKNOWNS}},
            {{NUMBER_OF_AUXILIARY_VARIABLES}},
            {{SOLUTION_STORAGE_PRECISION}}>(
            repositories::{{SOLVER_INSTANCE}},
            marker.getX(),
            marker.getH(),
            fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp()
              - fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize(),
            faceNumber,
            fineGridFaces{{SOLVER_NAME}}SubcellOld(faceNumber).data()
          );
        }
      }
    }
  }
"""

    def __init__(self, solver, cell_guard, face_guard):
        super().__init__(solver)
        self._cell_guard = cell_guard
        self._face_guard = face_guard

    def _add_action_set_entries_to_dictionary(self, dictionary):
        dictionary["CELL_GUARD"] = self._cell_guard
        dictionary["FACE_GUARD"] = self._face_guard
        dictionary["PROJECT_STATUS_TO_FACES"] = cell_status_to_face_snippet(
            self._solver._name
        )
        dictionary["RESET_REFINED_FACE_STATUS"] = reset_refined_face_status_snippet(
            self._solver._name
        )
        dictionary["HANGING_FACE_STATUS_FROM_COARSE"] = (
            hanging_face_status_from_coarse_snippet(self._solver._name)
        )
        dictionary["HANGING_FACE_STATUS_REDUCE"] = hanging_face_status_reduce_snippet(
            self._solver._name
        )
        dg_to_fv, _ = create_subcell_projection_matrices(self._solver._basis)
        flattened_matrix = [value for row in dg_to_fv for value in row]
        dictionary["DG_TO_FV_MATRIX"] = render_tensor_1(flattened_matrix)
        dictionary["HALO_WIDTH"] = self._solver._subcell_limiter_solver.halo_width

    def get_body_of_operation(self, operation_name):
        result = ""
        dictionary = {}
        self._solver._init_dictionary_with_default_parameters(dictionary)
        self._solver.add_entries_to_text_replacement_dictionary(dictionary)
        self._add_action_set_entries_to_dictionary(dictionary)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            result = jinja2.Template(self._Template_TouchFaceFirstTime).render(
                **dictionary
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME
        ):
            result = jinja2.Template(self._Template_TouchHangingFaceFirstTime).render(
                **dictionary
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME
        ):
            result = jinja2.Template(self._Template_TouchHangingFaceLastTime).render(
                **dictionary
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(
                **dictionary
            )
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(SpreadDomain, self).get_includes()
            + """
#include "exahype2/dg/LimiterStatus.h"
#include "exahype2/dg/SubcellProjection.h"
#include "exahype2/fv/ApplyBoundaryConditionsOnFace.h"
#include "exahype2/ProjectPatch.h"
"""
        )
