# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2
import peano4
import exahype2

import peano4.solversteps


class AdaptiveMeshFaceCoupling(peano4.solversteps.ActionSet):
    def __init__(self, solver):
        super(AdaptiveMeshFaceCoupling, self).__init__(descend_invocation_order=1)

        patch_overlap = solver._patch_overlap_new
        if patch_overlap.dim[0] % 2 != 0:
            raise Exception(
                "Error: Patch associated to face has to have an even number of cells, as the overlap would not be symmetric otherwise."
            )

        face_label_name = solver._face_label.name
        cell_label_name = solver._cell_label.name
        update_face_label_attribute = exahype2.grid.UpdateFaceLabel.get_attribute_name(
            solver._name
        )
        default_guard = solver._provide_face_data_to_compute_kernels_default_guard()

        self.d = {}
        self.d["UNKNOWNS"] = str(patch_overlap.no_of_unknowns)
        self.d["DOFS_PER_AXIS"] = str(patch_overlap.dim[1])
        self.d["OVERLAP"] = str(int(patch_overlap.dim[0] / 2))
        self.d["CLEAR_GUARD"] = default_guard
        self.d["RESTRICT_GUARD"] = default_guard
        self.d["INTERPOLATE_GUARD"] = (
            f"{default_guard} and \n"
            f" repositories::{solver.get_name_of_global_instance()}.getSolverState()!={solver._name}::SolverState::GridInitialisation\n"
        )

        self.d["SOLVER_NAME"] = solver._name
        self.d["SOLVER_REPOSITORY"] = (
            f"repositories::{solver.get_name_of_global_instance()}"
        )
        self.d["STORAGE_PRECISION"] = solver._storage_precision

        self.d["FINE_GRID_CELL_LABEL_ACCESSOR"] = f"fineGridCell{cell_label_name}"
        self.d["COARSE_GRID_CELL_LABEL_ACCESSOR"] = f"coarseGridCell{cell_label_name}"
        self.d["FINE_GRID_CELL_Q_NEW_ACCESSOR"] = (
            f"fineGridCell{solver.current_unknown_identifier()}"
        )
        self.d["FINE_GRID_CELL_Q_OLD_ACCESSOR"] = (
            f"fineGridCell{solver.previous_unknown_identifier()}"
        )
        self.d["COARSE_GRID_CELL_Q_NEW_ACCESSOR"] = (
            f"coarseGridCell{solver.current_unknown_identifier()}"
        )
        self.d["COARSE_GRID_CELL_Q_OLD_ACCESSOR"] = (
            f"coarseGridCell{solver.previous_unknown_identifier()}"
        )

        self.d["FINE_GRID_FACE_LABEL_ACCESSOR"] = f"fineGridFace{face_label_name}"
        self.d["FINE_GRID_FACES_LABEL_ACCESSOR"] = f"fineGridFaces{face_label_name}"
        self.d["COARSE_GRID_FACES_LABEL_ACCESSOR"] = f"coarseGridFaces{face_label_name}"
        self.d["COARSE_GRID_FACES_UPDATE_LABEL_ACCESSOR"] = (
            f"coarseGridFaces{update_face_label_attribute}"
        )

        self.d["FINE_GRID_FACE_Q_NEW_ACCESSOR"] = (
            f"fineGridFace{solver.current_unknown_identifier()}"
        )
        self.d["FINE_GRID_FACE_Q_OLD_ACCESSOR"] = (
            f"fineGridFace{solver.previous_unknown_identifier()}"
        )
        self.d["FINE_GRID_FACES_Q_NEW_ACCESSOR"] = (
            f"fineGridFaces{solver.current_unknown_identifier()}"
        )

        self.d["COARSE_GRID_FACES_Q_NEW_ACCESSOR"] = (
            f"coarseGridFaces{solver.current_unknown_identifier()}"
        )
        self.d["COARSE_GRID_FACES_Q_OLD_ACCESSOR"] = (
            f"coarseGridFaces{solver.previous_unknown_identifier()}"
        )

        self._Template_TouchFaceFirstTime = """
  if ({{CLEAR_GUARD}}) {
    std::fill_n(
      {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
#if DIMENSIONS == 2
      2 * {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{OVERLAP}},
#else
      2 * {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}} * {{OVERLAP}},
#endif
      0
    );
  }
"""

        self._Template_TouchCellFirstTime = ""

        # The inner half of the halo layer is staged here, and the coarse face
        # reduces every staged subface in one order-invariant pass when the sweep
        # leaves it.
        self._Template_TouchHangingFaceLastTime = """
  if ({{RESTRICT_GUARD}}) {
    const auto isLeftEntryOnCoarseFaceLabel = marker.getSelectedFaceNumber() >= DIMENSIONS;
    const auto facePosition                 = marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
    // Across a level jump of two or more, this face carries staged subfaces of
    // its own. They reduce here, so the chained restriction is complete before
    // this face stages itself.
    if (_haloRestrictionStageQNew.hasFrame({{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data() + facePosition)) {
      ::toolbox::blockstructured::restrictHaloLayerFromStagedSubfaces(
        _haloRestrictionStageQNew,
        {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data() + facePosition,
        {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
        marker.getSelectedFaceNumber() % DIMENSIONS,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }
    ::toolbox::blockstructured::stageHaloLayerForGatheredRestriction(
      marker,
      _haloRestrictionStageQNew,
      {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data() + facePosition,
      {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
      facePosition,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}}
    );

    const auto newTimeStamp = std::max(
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getUpdatedTimeStamp(isLeftEntryOnCoarseFaceLabel ? 0 : 1),
      {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getUpdatedTimeStamp(isLeftEntryOnCoarseFaceLabel ? 0 : 1)
    );
    {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setUpdated(isLeftEntryOnCoarseFaceLabel ? 0 : 1, true);
    {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber())
      .setUpdatedTimeStamp(isLeftEntryOnCoarseFaceLabel ? 0 : 1, newTimeStamp);
  }
"""

        # The last touch of every subface precedes the last touch of this face, so
        # the staged set is complete apart from subfaces other ranks hold.
        self._Template_TouchFaceLastTime = """
  if ({{RESTRICT_GUARD}}) {
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      if (_haloRestrictionStageQNew.hasFrame({{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data() + facePosition)) {
        ::toolbox::blockstructured::restrictHaloLayerFromStagedSubfaces(
          _haloRestrictionStageQNew,
          {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data() + facePosition,
          {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
          marker.getSelectedFaceNumber() % DIMENSIONS,
          {{DOFS_PER_AXIS}},
          {{UNKNOWNS}}
        );
      }
    }
  }
"""

        # The interpolation fills the QOld halo of a hanging face.
        self._Template_TouchHangingFaceFirstTime = """
  if ({{RESTRICT_GUARD}}) {
    // Across a level jump of two or more, a hanging face between two hanging
    // levels takes the chained restriction in its QNew halo. A half that no
    // subface stages into keeps what its storage slot holds, so the halo starts
    // at zero.
    std::fill_n(
      {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
#if DIMENSIONS == 2
      2 * {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{OVERLAP}},
#else
      2 * {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}} * {{OVERLAP}},
#endif
      0
    );
  }
  if ({{INTERPOLATE_GUARD}}) {
    const auto leftRightEntry = marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;
    if (not marker.isInteriorFaceWithinPatch()) {
      ::toolbox::blockstructured::interpolateHaloLayerWithTangentialSlope(
        marker,
        {{COARSE_GRID_FACES_Q_OLD_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
        {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
        leftRightEntry,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }

    {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setNewTimeStamp(
      leftRightEntry,
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getNewTimeStamp(leftRightEntry)
    );
    {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setOldTimeStamp(
      leftRightEntry,
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getOldTimeStamp(leftRightEntry)
    );
  }
"""

        # A solver traversal never changes the topology, so no cell or face is
        # created or destroyed while this action set runs.
        self._Template_CreateFace = ""
        self._Template_DestroyFace = ""
        self._Template_CreateCell = ""
        self._Template_DestroyCell = ""

    def get_action_set_name(self):
        return "exahype2_solvers_fv_actionsets_" + type(self).__name__

    def get_constructor_body(self):
        return ""

    def get_destructor_body(self):
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< ::peano4::grid::GridControlEvent >();\n"

    def user_should_modify_template(self):
        return False

    def get_body_of_operation(self, operation_name):
        operations = peano4.solversteps.ActionSet
        template_of_operation = {
            operations.OPERATION_CREATE_FACE: self._Template_CreateFace,
            operations.OPERATION_DESTROY_FACE: self._Template_DestroyFace,
            operations.OPERATION_CREATE_CELL: self._Template_CreateCell,
            operations.OPERATION_DESTROY_CELL: self._Template_DestroyCell,
            operations.OPERATION_TOUCH_FACE_FIRST_TIME: self._Template_TouchFaceFirstTime,
            operations.OPERATION_TOUCH_FACE_LAST_TIME: self._Template_TouchFaceLastTime,
            operations.OPERATION_TOUCH_CELL_FIRST_TIME: self._Template_TouchCellFirstTime,
            operations.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME: self._Template_TouchHangingFaceFirstTime,
            operations.OPERATION_TOUCH_HANGING_FACE_LAST_TIME: self._Template_TouchHangingFaceLastTime,
        }
        if operation_name in template_of_operation:
            return jinja2.Template(template_of_operation[operation_name]).render(
                **self.d
            )
        return "\n"

    def get_attributes(self):
        return f"""
    ::toolbox::blockstructured::ChildDataStage<{self.d["STORAGE_PRECISION"]}> _haloRestrictionStageQNew{{
      THREE_POWER_D / 3,
      (DIMENSIONS == 3 ? {self.d["DOFS_PER_AXIS"]} * {self.d["DOFS_PER_AXIS"]} : {self.d["DOFS_PER_AXIS"]}) * {self.d["UNKNOWNS"]}}};
"""

    def get_includes(self):
        return """
#include "peano4/Loop.h"
#include "toolbox/blockstructured/AMRRoutines.h"
#include "exahype2/fv/PatchUtils.h"
#include "toolbox/blockstructured/ChildDataStage.h"
#include "../repositories/SolverRepository.h"
"""


class DynamicAMRTransfer(AdaptiveMeshFaceCoupling):
    def __init__(self, solver):
        super(DynamicAMRTransfer, self).__init__(solver)

        # A coarsening parent's cell and face targets are zeroed before the
        # destroyed children stage into them, so a target that nothing on this rank
        # reduces into carries a defined value.
        self._Template_TouchFaceFirstTime = """
  if (marker.hasBeenRefined() and not marker.willBeRefined()) {
    // The clear also wipes the half a surviving equal-level neighbour projected.
    // Both halves are flagged, and each side's projection completes its half and
    // takes its flag back down.
    {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setRequiresDataTransfer(0, true);
    {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setRequiresDataTransfer(1, true);
    std::fill_n(
      {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
#if DIMENSIONS == 2
      2 * {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{OVERLAP}},
#else
      2 * {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}} * {{OVERLAP}},
#endif
      0
    );
    std::fill_n(
      {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
#if DIMENSIONS == 2
      2 * {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{OVERLAP}},
#else
      2 * {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}} * {{OVERLAP}},
#endif
      0
    );
  }
"""

        self._Template_TouchCellFirstTime = """
  if (marker.hasBeenRefined() and not marker.willBeRefined()) {
    std::fill_n(
      {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
#if DIMENSIONS == 2
      {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}},
#else
      {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}},
#endif
      0
    );
    std::fill_n(
      {{FINE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
#if DIMENSIONS == 2
      {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}},
#else
      {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}},
#endif
      0
    );
  }
"""

        # The hanging-face coupling stays off in the realise sweeps: No solver reads
        # the halos here, and the next time step rebuilds them on the settled grid.
        self._Template_TouchHangingFaceFirstTime = ""
        self._Template_TouchHangingFaceLastTime = ""

        # Every destroyed child has staged its halo layers by this face's last touch.
        # A half whose transfer flag is still up takes its content from the staged
        # subfaces in one order-invariant reduction, and a half that a projection
        # completed discards its staged subfaces, so each half has one writer.
        self._Template_TouchFaceLastTime = """
  for (int facePosition = 0; facePosition < 2; facePosition++) {
    if (_haloRestrictionStageQNew.hasFrame({{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data() + facePosition)) {
      if ({{FINE_GRID_FACE_LABEL_ACCESSOR}}.getRequiresDataTransfer(facePosition)) {
        ::toolbox::blockstructured::restrictHaloLayerFromStagedSubfaces(
          _haloRestrictionStageQNew,
          {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data() + facePosition,
          {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
          marker.getSelectedFaceNumber() % DIMENSIONS,
          {{DOFS_PER_AXIS}},
          {{UNKNOWNS}}
        );
        ::toolbox::blockstructured::restrictHaloLayerFromStagedSubfaces(
          _haloRestrictionStageQOld,
          {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data() + facePosition,
          {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
          marker.getSelectedFaceNumber() % DIMENSIONS,
          {{DOFS_PER_AXIS}},
          {{UNKNOWNS}}
        );
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setUpdated(facePosition, true);
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setRequiresDataTransfer(facePosition, false);
      } else {
        _haloRestrictionStageQNew.discard({{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data() + facePosition);
        _haloRestrictionStageQOld.discard({{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data() + facePosition);
      }
    }
  }
"""

        self._Template_DestroyFace = """
  if (not marker.isInteriorFaceWithinPatch()) {
    // Every destroyed subface stages both of its halves here, and the coarse
    // face's last touch reduces the halves whose data-transfer flag is still up.
    // A face on the domain wall carries no refined vertex, so its first touch
    // clears neither its content nor its flags.
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      ::toolbox::blockstructured::stageHaloLayerForGatheredRestriction(
        marker,
        _haloRestrictionStageQNew,
        {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data() + facePosition,
        {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
        facePosition,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
      ::toolbox::blockstructured::stageHaloLayerForGatheredRestriction(
        marker,
        _haloRestrictionStageQOld,
        {{COARSE_GRID_FACES_Q_OLD_ACCESSOR}}(marker.getSelectedFaceNumber()).data() + facePosition,
        {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
        facePosition,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }
    // The coarse face carries no valid boundary flag until this loop derives it
    // from the face position within the domain.
    const tarch::la::Vector<DIMENSIONS, double> offset(DomainOffset);
    const tarch::la::Vector<DIMENSIONS, double> size(DomainSize);
    bool                                        isBoundary = false;
    for (int d = 0; d < DIMENSIONS; d++) {
      isBoundary = isBoundary or tarch::la::equals(marker.getX()(d), offset(d));
      isBoundary = isBoundary or tarch::la::equals(marker.getX()(d), offset(d) + size(d));
    }
    {{COARSE_GRID_FACES_UPDATE_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setBoundary(isBoundary);

    // Index 0 addresses the left side of the face, index 1 the right one.
    for (int i = 0; i < 2; i++) {
      const auto newTimeStamp = std::max(
        {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getNewTimeStamp(i),
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getNewTimeStamp(i)
      );
      const auto oldTimeStamp = std::max(
        {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getOldTimeStamp(i),
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getOldTimeStamp(i)
      );
      const auto updatedTimeStamp = std::max(
        {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getUpdatedTimeStamp(i),
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getUpdatedTimeStamp(i)
      );
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setNewTimeStamp(i, newTimeStamp);
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setOldTimeStamp(i, oldTimeStamp);
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setUpdatedTimeStamp(i, updatedTimeStamp);
    }
  }
"""

        # A created face is persistent, so both its halves take their time stamps
        # from the coarse cell.
        self._Template_CreateFace = """
  if (marker.isInteriorFaceWithinPatch()) {
    for (int side = 0; side < 2; side++) {
      ::toolbox::blockstructured::interpolateHaloLayerFromCoarseCell(
        marker,
        {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
        {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
        side,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
      ::toolbox::blockstructured::interpolateHaloLayerFromCoarseCell(
        marker,
        {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
        {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
        side,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }
  } else {
    for (int side = 0; side < 2; side++) {
      ::toolbox::blockstructured::interpolateHaloLayerWithTangentialSlope(
        marker,
        {{COARSE_GRID_FACES_Q_OLD_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
        {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
        side,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
      ::toolbox::blockstructured::interpolateHaloLayerWithTangentialSlope(
        marker,
        {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
        {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
        side,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }
  }

  {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setNewTimeStamp({{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp());
  {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setOldTimeStamp({{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp());
"""

        self._Template_CreateCell = """
  ::toolbox::blockstructured::interpolateCell(
    marker,
    {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
    {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
    {{DOFS_PER_AXIS}},
    {{UNKNOWNS}}
  );
  ::toolbox::blockstructured::interpolateCell(
    marker,
    {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
    {{FINE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
    {{DOFS_PER_AXIS}},
    {{UNKNOWNS}}
  );

#if PEANO_DEBUG
  ::exahype2::fv::validatePatch(
    {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
    {{UNKNOWNS}},
    0, // Auxiliary values. Not known here.
    {{DOFS_PER_AXIS}},
    0, // No halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  );
  ::exahype2::fv::validatePatch(
    {{FINE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
    {{UNKNOWNS}},
    0, // Auxiliary values. Not known here.
    {{DOFS_PER_AXIS}},
    0, // No halo
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
  );
#endif

  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStamp({{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp());
  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStepSize({{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStepSize());

  // The parent's pending roll-over travels with its data. A created cell that
  // keeps hasUpdated at false never swaps its buffers in the next time step.
  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setHasUpdated({{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getHasUpdated());

  // The criterion flag makes the running mesh-refinement sweeps evaluate the
  // created cell's data once.
  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setRequiresCriterionEvaluation(true);
"""

        self._Template_DestroyCell = """
  // The destroyed child stages its patches, and the sibling that completes the
  // set reduces the whole set onto the parent in one order-invariant pass. The
  // load balancer places no rank boundary inside a cell that has a leaf child,
  // so the set always completes on one rank.
  const int linearisedChildPosition = peano4::dLinearised(marker.getRelativePositionWithinFatherCell(), 3);
  _cellRestrictionStageQOld.stage(
    {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
    linearisedChildPosition,
    {{FINE_GRID_CELL_Q_OLD_ACCESSOR}}.data()
  );
  if (_cellRestrictionStageQNew.stage(
        {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
        linearisedChildPosition,
        {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data()
      )) {
    const {{STORAGE_PRECISION}}* childValuesQNew[THREE_POWER_D];
    const {{STORAGE_PRECISION}}* childValuesQOld[THREE_POWER_D];
    for (int childIndex = 0; childIndex < THREE_POWER_D; childIndex++) {
      childValuesQNew[childIndex] = _cellRestrictionStageQNew.child({{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(), childIndex);
      childValuesQOld[childIndex] = _cellRestrictionStageQOld.child({{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(), childIndex);
    }
    ::toolbox::blockstructured::restrictCellFromChildren(
      {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
      childValuesQNew,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}}
    );
    ::toolbox::blockstructured::restrictCellFromChildren(
      {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
      childValuesQOld,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}}
    );
    _cellRestrictionStageQNew.discard({{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data());
    _cellRestrictionStageQOld.discard({{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data());

#if PEANO_DEBUG
    ::exahype2::fv::validatePatch(
      {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
      {{UNKNOWNS}},
      0, // Auxiliary values. Not known here.
      {{DOFS_PER_AXIS}},
      0, // No halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
    );
    ::exahype2::fv::validatePatch(
      {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
      {{UNKNOWNS}},
      0, // Auxiliary values. Not known here.
      {{DOFS_PER_AXIS}},
      0, // No halo
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): " + marker.toString()
    );
#endif
  }

  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStamp({{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp());
  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStepSize({{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStepSize());

  // The children's pending roll-over travels with their data. A parent that
  // keeps hasUpdated at false never swaps its buffers in the next time step.
  if ({{FINE_GRID_CELL_LABEL_ACCESSOR}}.getHasUpdated()) {
    {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setHasUpdated(true);
  }

  // The criterion flag makes the running mesh-refinement sweeps evaluate the
  // exposed parent's restricted data once.
  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setRequiresCriterionEvaluation(true);
"""

        # The interpolated or restricted content of a face half differs from the
        # boundary volumes of the cell that owns it, so the owner projects its
        # patch onto that half again. The projection runs on a coarsening parent
        # and on every cell with a face half whose RequiresDataTransfer entry is
        # up.
        self.d["PROJECT_GUARD"] = solver._store_cell_data_default_guard()
        self._Template_ProjectPatchOntoFaces = """
  bool requiresProjection = marker.hasBeenRefined() and not marker.willBeRefined();
  for (int face = 0; face < TWO_TIMES_D; face++) {
    const int ownSide = face < DIMENSIONS ? 1 : 0;
    requiresProjection = requiresProjection
      or (not {{FINE_GRID_FACES_LABEL_ACCESSOR}}(face).getIsHanging()
          and {{FINE_GRID_FACES_LABEL_ACCESSOR}}(face).getRequiresDataTransfer(ownSide));
  }
  if ({{PROJECT_GUARD}} and requiresProjection) {
    const auto* const QIn = {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data();

    {{STORAGE_PRECISION}}* facesOut[TWO_TIMES_D];
    for (int face = 0; face < TWO_TIMES_D; face++) {
      facesOut[face] = {{FINE_GRID_FACES_Q_NEW_ACCESSOR}}(face).data();
    }

    ::toolbox::blockstructured::projectPatch<
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{STORAGE_PRECISION}}
    >(QIn, facesOut);

    for (int d = 0; d < DIMENSIONS; d++) {
      {{FINE_GRID_FACES_LABEL_ACCESSOR}}(d).setUpdated(1, true);
      {{FINE_GRID_FACES_LABEL_ACCESSOR}}(d).setUpdatedTimeStamp(1, {{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp());
      {{FINE_GRID_FACES_LABEL_ACCESSOR}}(d + DIMENSIONS).setUpdated(0, true);
      {{FINE_GRID_FACES_LABEL_ACCESSOR}}(d + DIMENSIONS).setUpdatedTimeStamp(0, {{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp());
    }
    for (int face = 0; face < TWO_TIMES_D; face++) {
      const int ownSide = face < DIMENSIONS ? 1 : 0;
      if (not {{FINE_GRID_FACES_LABEL_ACCESSOR}}(face).getIsHanging()) {
        {{FINE_GRID_FACES_LABEL_ACCESSOR}}(face).setRequiresDataTransfer(ownSide, false);
      }
    }
    // The criterion pass of this sweep, which runs before this projection, has
    // evaluated any freshly created cell by now.
    {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setRequiresCriterionEvaluation(false);
  }
"""

    def get_body_of_operation(self, operation_name):
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            return jinja2.Template(self._Template_ProjectPatchOntoFaces).render(
                **self.d
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
        ):
            return jinja2.Template(self._Template_TouchFaceLastTime).render(**self.d)
        return super(DynamicAMRTransfer, self).get_body_of_operation(operation_name)

    def get_body_of_getGridControlEvents(self):
        return "  return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();\n"

    def get_attributes(self):
        return (
            super(DynamicAMRTransfer, self).get_attributes()
            + f"""
    ::toolbox::blockstructured::ChildDataStage<{self.d["STORAGE_PRECISION"]}> _haloRestrictionStageQOld{{
      THREE_POWER_D / 3,
      (DIMENSIONS == 3 ? {self.d["DOFS_PER_AXIS"]} * {self.d["DOFS_PER_AXIS"]} : {self.d["DOFS_PER_AXIS"]}) * {self.d["UNKNOWNS"]}}};
    ::toolbox::blockstructured::ChildDataStage<{self.d["STORAGE_PRECISION"]}> _cellRestrictionStageQNew{{
      THREE_POWER_D,
      (DIMENSIONS == 3 ? {self.d["DOFS_PER_AXIS"]} * {self.d["DOFS_PER_AXIS"]} * {self.d["DOFS_PER_AXIS"]} : {self.d["DOFS_PER_AXIS"]} * {self.d["DOFS_PER_AXIS"]}) * {self.d["UNKNOWNS"]}}};
    ::toolbox::blockstructured::ChildDataStage<{self.d["STORAGE_PRECISION"]}> _cellRestrictionStageQOld{{
      THREE_POWER_D,
      (DIMENSIONS == 3 ? {self.d["DOFS_PER_AXIS"]} * {self.d["DOFS_PER_AXIS"]} * {self.d["DOFS_PER_AXIS"]} : {self.d["DOFS_PER_AXIS"]} * {self.d["DOFS_PER_AXIS"]}) * {self.d["UNKNOWNS"]}}};
"""
        )

    def get_includes(self):
        return f"""{super(DynamicAMRTransfer, self).get_includes()}
#include "exahype2/RefinementControlService.h"
#include "toolbox/blockstructured/ProjectPatch.h"
"""
