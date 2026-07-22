# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2
import peano4
import exahype2

import peano4.solversteps
import peano4.toolbox.blockstructured.DynamicAMR


class AdaptiveMeshFaceCoupling(peano4.toolbox.blockstructured.DynamicAMR):
    """
    Per-step coupling across a resolution transition (hanging faces).

    On an adaptive mesh, every solver traversal interpolates the coarse
    solution down onto the hanging faces of the finer level and restricts the
    fine data back up. This is a per-sweep operation on a grid whose topology
    does not change, so this action set goes into InitGrid and TimeStep.

    The create/destroy inter-grid transfer is switched off here: changing the
    topology (creating and destroying cells and faces) is the job of the
    dedicated DynamicAMR observer via DynamicAMRTransfer.

    The halo prolongation uses the tangential-slope variant: it removes the
    piecewise-constant staircase along a coarse-fine face - which a Riemann
    flux would turn into a spurious tangential velocity - while staying
    bitwise mirror-symmetric.
    """

    def __init__(self, solver):
        face_label_name = solver._face_label.name
        cell_label_name = solver._cell_label.name
        update_face_label_attribute = exahype2.grid.UpdateFaceLabel.get_attribute_name(
            solver._name
        )
        default_guard = solver._provide_face_data_to_compute_kernels_default_guard()
        super(AdaptiveMeshFaceCoupling, self).__init__(
            patch=solver._patch,
            patch_overlap_interpolation=solver._patch_overlap_new,
            patch_overlap_restriction=solver._patch_overlap_new,
            clear_guard=default_guard,
            interpolate_guard=(
                f"{default_guard} and \n"
                f" repositories::{solver.get_name_of_global_instance()}.getSolverState()!={solver._name}::SolverState::GridInitialisation\n"
            ),
            restrict_guard=default_guard,
            additional_includes="""
#include "../repositories/SolverRepository.h"
""",
        )

        # Solver identity
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
    // Only the exterior side of the face is needed, but the interior half is
    // unused, so we clear the entire face.
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

        # The coarsening-parent clears live in DynamicAMRTransfer: a solver traversal
        # never changes the topology, so no cell can be a coarsening parent here.
        self._Template_TouchCellFirstTime = ""

        # Restricts the inner half of the halo layer from fine to coarse. In
        # addition to the data restriction, we also restrict the updated flag
        # and the face's time stamp.
        self._Template_TouchHangingFaceLastTime = """
  if ({{RESTRICT_GUARD}}) {
    const auto isLeftEntryOnCoarseFaceLabel = marker.getSelectedFaceNumber() >= DIMENSIONS;
    const auto facePosition                 = marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
    ::toolbox::blockstructured::restrictHaloLayer(
      marker,
      {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
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

        # Hanging-face interpolation fills QOld only. The solver callback
        # handles solution initialisation separately.
        self._Template_TouchHangingFaceFirstTime = """
  if ({{RESTRICT_GUARD}}) {
    // Across a level jump of two or more, a hanging face between two hanging
    // levels stages the chained restriction in its QNew halo. QNew is an
    // accumulation target, so it has to start at zero even when Peano reuses a
    // storage slot. The lowest hanging level is unaffected because its cell's
    // projection overwrites the staging layer.
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

        # A solver traversal never changes the topology, so the create/destroy
        # inter-grid transfer is disabled here; DynamicAMRTransfer re-enables it.
        self._Template_CreateFace = ""
        self._Template_DestroyFace = ""
        self._Template_CreateCell = ""
        self._Template_DestroyCell = ""

    def get_action_set_name(self):
        return "exahype2_solvers_fv_actionsets_" + type(self).__name__

    def get_includes(self):
        return f"""{super(AdaptiveMeshFaceCoupling, self).get_includes()}
#include "exahype2/fv/PatchUtils.h"
#include "toolbox/blockstructured/AMRRoutines.h"
"""


class DynamicAMRTransfer(AdaptiveMeshFaceCoupling):
    """
    Full inter-grid transfer for the dedicated DynamicAMR observer.

    This is the action set that changes the topology: it hands the committed
    refine/coarsen events to Peano via getGridControlEvents, so the observer's
    traversals trigger and realise the requests the RefinementCriterion
    accumulated during the preceding time step. It interpolates the coarse
    solution (QNew and QOld) onto cells and faces Peano creates and restricts
    the fine solution back onto cells and faces Peano destroys. No PDE update
    happens here.
    """

    def __init__(self, solver):
        super(DynamicAMRTransfer, self).__init__(solver)

        # The averaging restriction accumulates the destroyed fine entities onto
        # their coarsening parent, so the parent's cell and face targets are zeroed
        # before the children restrict into them on the descent. Both QNew and QOld
        # are cleared, since stale QOld would blow up through the QOld/QNew
        # roll-over.
        self._Template_TouchFaceFirstTime = """
  if (marker.hasBeenRefined() and not marker.willBeRefined()) {
    // The clear also wipes the half a surviving equal-level neighbour projected: its
    // fine counterparts hang, so no restriction refills the face, and the neighbour
    // re-projects only where a data-transfer flag asks for it. Flag both halves; each
    // side's projection completes its half and takes its flag back down.
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

        # The hanging-face coupling stays off in the realise sweeps: no solver reads the
        # halos here, and the next time step rebuilds them on the settled grid.
        self._Template_TouchHangingFaceFirstTime = ""
        self._Template_TouchHangingFaceLastTime = ""

        # By this face's last touch every destroyed child has contributed its share, so a
        # half the restriction filled is complete and current. Its transfer flag comes down
        # here, so the roll-over of the following time step backs the half up instead of
        # skipping it as pending. Halves that stay flagged received no data in this sweep.
        self._Template_TouchFaceLastTime = """
  if (marker.hasBeenRefined() and not marker.willBeRefined()) {
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      if ({{FINE_GRID_FACE_LABEL_ACCESSOR}}.getRequiresDataTransfer(facePosition)
          and {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getUpdated(facePosition)) {
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setRequiresDataTransfer(facePosition, false);
      }
    }
  }
"""

        self._Template_DestroyFace = """
  if (not marker.isInteriorFaceWithinPatch()) {
    // The coarsening sweep zeroed the coarse face and raised both data-transfer
    // flags, and every projection that completes a half takes its flag down
    // within this sweep. The restriction therefore fills only halves whose flag
    // is still up: accumulating into a half a projection already assigned would
    // double it, in whichever order the traversal visits the two writers. The
    // flagged halves have no equal-level cell to project them, such as the half
    // beneath the still-refined neighbour of a newly formed level jump, and
    // take their content from the destroyed fine faces instead.
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      if ({{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).getRequiresDataTransfer(facePosition)) {
        ::toolbox::blockstructured::restrictHaloLayer(
          marker,
          {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
          {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
          facePosition,
          {{DOFS_PER_AXIS}},
          {{UNKNOWNS}}
        );

        ::toolbox::blockstructured::restrictHaloLayer(
          marker,
          {{COARSE_GRID_FACES_Q_OLD_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
          {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
          facePosition,
          {{DOFS_PER_AXIS}},
          {{UNKNOWNS}}
        );

        {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(marker.getSelectedFaceNumber()).setUpdated(facePosition, true);
      }
    }
    // A coarse face might have been non-persistent before. So it might
    // not carry a valid boundary flag, and we have to re-analyse it and
    // set it accordingly.
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

        # This variant interpolates both QOld and QNew. As we deal with a
        # persistent face, we set both the left and the right time stamp from
        # the coarse face.
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

  // The parent's pending roll-over travels with its data: without it a created
  // cell keeps hasUpdated=false, the next time step never swaps its buffers,
  // and the cell resumes from the interpolation of the parent's previous state
  // while its neighbours already advanced.
  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setHasUpdated({{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getHasUpdated());

  // The created cell is marked for the criterion pass, so the running
  // mesh-refinement sweeps evaluate its data once and keep descending while the
  // data justifies it.
  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setRequiresCriterionEvaluation(true);
"""

        self._Template_DestroyCell = """
  ::toolbox::blockstructured::restrictCell(
    marker,
    {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
    {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
    {{DOFS_PER_AXIS}},
    {{UNKNOWNS}}
  );
  ::toolbox::blockstructured::restrictCell(
    marker,
    {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
    {{FINE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
    {{DOFS_PER_AXIS}},
    {{UNKNOWNS}}
  );

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

  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStamp({{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp());
  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStepSize({{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStepSize());

  // The children's pending roll-over travels with their data: without it a
  // parent that has never been a leaf keeps hasUpdated=false, the next time
  // step never swaps its buffers, and the cell resumes from the restriction of
  // the children's previous state while its neighbours already advanced.
  if ({{FINE_GRID_CELL_LABEL_ACCESSOR}}.getHasUpdated()) {
    {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setHasUpdated(true);
  }

  // The freshly exposed parent is marked for the criterion pass, so the running
  // mesh-refinement sweeps evaluate its restricted data once and keep
  // ascending while the data justifies it.
  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setRequiresCriterionEvaluation(true);
"""

        # The create/destroy transfer interpolates or restricts face halves, so
        # their content differs from the exact boundary volumes of the owning cells.
        # The two sides of such a face would solve different Riemann problems in
        # the next time step and mass would leak across it. The projection runs
        # only where the topology change left work behind: a coarsening parent,
        # identified by its marker, or a cell with a face half whose
        # RequiresDataTransfer entry the face creation set. The second condition
        # also reaches the pre-existing neighbour of a created face, whose
        # projection supplies the other half of that face's halo. Completed
        # halves are marked current, so a later mesh-refinement sweep skips
        # every settled cell.
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

    kernels::{{SOLVER_NAME}}::projectPatch(QIn, facesOut);

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

    def get_includes(self):
        return f"""{super(DynamicAMRTransfer, self).get_includes()}
#include "exahype2/RefinementControlService.h"
#include "kernels/{self.d["SOLVER_NAME"]}/ProjectPatch.h"
"""
