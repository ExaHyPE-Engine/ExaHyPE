# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2
import jinja2
import peano4

import peano4.blockstructured.DynamicAMR
import peano4.solversteps


class AdaptiveMeshFaceCoupling(peano4.blockstructured.DynamicAMR):
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
        super().__init__(
            patch=solver._patch,
            patch_overlap_interpolation=solver._patch_overlap_new,
            patch_overlap_restriction=solver._patch_overlap_new,
            clear_guard=default_guard,
            interpolate_guard=(
                f"({default_guard}) and "
                f"repositories::{solver.get_name_of_global_instance()}"
                f".getSolverState() != "
                f"{solver._name}::SolverState::GridInitialisation"
            ),
            restrict_guard=default_guard,
            additional_includes="""
#include "repositories/SolverRepository.h"
""",
        )
        self._dictionary["SOLVER_NAME"] = solver._name
        self._dictionary["SOLVER_REPOSITORY"] = (
            f"repositories::{solver.get_name_of_global_instance()}"
        )
        self._dictionary["STORAGE_PRECISION"] = solver._storage_precision

        self._dictionary["FINE_GRID_CELL_LABEL_ACCESSOR"] = (
            f"fineGridCell{cell_label_name}"
        )
        self._dictionary["COARSE_GRID_CELL_LABEL_ACCESSOR"] = (
            f"coarseGridCell{cell_label_name}"
        )
        self._dictionary["FINE_GRID_CELL_Q_NEW_ACCESSOR"] = (
            f"fineGridCell{solver.current_unknown_identifier()}"
        )
        self._dictionary["FINE_GRID_CELL_Q_OLD_ACCESSOR"] = (
            f"fineGridCell{solver.previous_unknown_identifier()}"
        )
        self._dictionary["COARSE_GRID_CELL_Q_NEW_ACCESSOR"] = (
            f"coarseGridCell{solver.current_unknown_identifier()}"
        )
        self._dictionary["COARSE_GRID_CELL_Q_OLD_ACCESSOR"] = (
            f"coarseGridCell{solver.previous_unknown_identifier()}"
        )

        self._dictionary["FINE_GRID_FACE_LABEL_ACCESSOR"] = (
            f"fineGridFace{face_label_name}"
        )
        self._dictionary["FINE_GRID_FACES_LABEL_ACCESSOR"] = (
            f"fineGridFaces{face_label_name}"
        )
        self._dictionary["COARSE_GRID_FACES_LABEL_ACCESSOR"] = (
            f"coarseGridFaces{face_label_name}"
        )
        self._dictionary["COARSE_GRID_FACES_UPDATE_LABEL_ACCESSOR"] = (
            f"coarseGridFaces{update_face_label_attribute}"
        )

        self._dictionary["FINE_GRID_FACE_Q_NEW_ACCESSOR"] = (
            f"fineGridFace{solver.current_unknown_identifier()}"
        )
        self._dictionary["FINE_GRID_FACE_Q_OLD_ACCESSOR"] = (
            f"fineGridFace{solver.previous_unknown_identifier()}"
        )
        self._dictionary["FINE_GRID_FACES_Q_NEW_ACCESSOR"] = (
            f"fineGridFaces{solver.current_unknown_identifier()}"
        )

        self._dictionary["COARSE_GRID_FACES_Q_NEW_ACCESSOR"] = (
            f"coarseGridFaces{solver.current_unknown_identifier()}"
        )
        self._dictionary["COARSE_GRID_FACES_Q_OLD_ACCESSOR"] = (
            f"coarseGridFaces{solver.previous_unknown_identifier()}"
        )

        self._Template_TouchFaceFirstTime = """
  if ({{CLEAR_GUARD}}) {
    // The unused interior half makes clearing the complete face safe.
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

        # Solver traversals cannot contain a coarsening parent because they do
        # not change the topology; DynamicAMRTransfer owns the required clears.
        self._Template_TouchCellFirstTime = ""

        self._Template_TouchHangingFaceLastTime = """
  if ({{RESTRICT_GUARD}}) {
    const auto isLeftEntryOnCoarseFaceLabel =
      marker.getSelectedFaceNumber() >= DIMENSIONS;
    const auto facePosition =
      marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
    ::toolbox::blockstructured::restrictHaloLayer(
      marker,
      {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
      {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
      facePosition,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}}
    );

    const auto coarseFacePosition = isLeftEntryOnCoarseFaceLabel ? 0 : 1;
    const auto newTimeStamp = std::max(
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
        marker.getSelectedFaceNumber()
      ).getUpdatedTimeStamp(coarseFacePosition),
      {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getUpdatedTimeStamp(
        coarseFacePosition
      )
    );
    {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
      marker.getSelectedFaceNumber()
    ).setUpdated(coarseFacePosition, true);
    {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
      marker.getSelectedFaceNumber()
    ).setUpdatedTimeStamp(
      coarseFacePosition,
      newTimeStamp
    );
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
    const auto facePosition =
      marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;
    if (not marker.isInteriorFaceWithinPatch()) {
      ::toolbox::blockstructured::interpolateHaloLayerWithTangentialSlope(
        marker,
        {{COARSE_GRID_FACES_Q_OLD_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
        {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
        facePosition,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }

    {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setNewTimeStamp(
      facePosition,
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
        marker.getSelectedFaceNumber()
      ).getNewTimeStamp(facePosition)
    );
    {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setOldTimeStamp(
      facePosition,
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
        marker.getSelectedFaceNumber()
      ).getOldTimeStamp(facePosition)
    );
  }
"""

        # DynamicAMRTransfer owns all create and destroy transfers.
        self._Template_CreateFace = ""
        self._Template_DestroyFace = ""
        self._Template_CreateCell = ""
        self._Template_DestroyCell = ""

    def get_action_set_name(self):
        return "exahype2_solvers_fv_actionsets_" + type(self).__name__

    def get_includes(self):
        return f"""{super().get_includes()}
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
        super().__init__(solver)

        # Halo restriction accumulates the destroyed fine faces into the coarse
        # face. Both of its states start at zero because either stale state
        # would contaminate the QOld/QNew roll-over.
        self._Template_TouchFaceFirstTime = """
  if (marker.hasBeenRefined() and not marker.willBeRefined()) {
    // Clearing also removes the surviving neighbour's projected half. Its fine
    // counterparts are hanging and cannot refill it, so both halves request a
    // projection; each completed projection clears its request.
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

        # Realisation sweeps do not solve the PDE, and the next time step
        # rebuilds hanging-face halos on the settled grid.
        self._Template_TouchHangingFaceFirstTime = ""
        self._Template_TouchHangingFaceLastTime = ""

        # A face half updated by the last touch contains every child
        # contribution. Clearing its request lets the next roll-over preserve
        # it; a request left set identifies a half that received no data.
        self._Template_TouchFaceLastTime = """
  if (marker.hasBeenRefined() and not marker.willBeRefined()) {
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      if (
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getRequiresDataTransfer(facePosition)
        and {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getUpdated(facePosition)
      ) {
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setRequiresDataTransfer(facePosition, false);
      }
    }
  }
"""

        self._Template_DestroyFace = """
  if (not marker.isInteriorFaceWithinPatch()) {
    // Completed projections clear their transfer requests. Restriction fills
    // only requested halves, because accumulating into a projected half would
    // double its data. A requested half has no equal-level cell that can
    // project it and therefore receives the destroyed fine-face data.
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      if (
        {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
          marker.getSelectedFaceNumber()
        ).getRequiresDataTransfer(facePosition)
      ) {
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

        {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
          marker.getSelectedFaceNumber()
        ).setUpdated(facePosition, true);
      }
    }
    // A newly persistent coarse face has no reliable boundary flag.
    const auto domainOffset =
      tarch::la::Vector<DIMENSIONS, double>(DomainOffset);
    const auto domainSize = tarch::la::Vector<DIMENSIONS, double>(DomainSize);
    auto isBoundary = false;
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
      isBoundary = isBoundary
        or tarch::la::equals(
          marker.getX()(dimension),
          domainOffset(dimension)
        );
      isBoundary = isBoundary
        or tarch::la::equals(
          marker.getX()(dimension),
          domainOffset(dimension) + domainSize(dimension)
        );
    }
    {{COARSE_GRID_FACES_UPDATE_LABEL_ACCESSOR}}(
      marker.getSelectedFaceNumber()
    ).setBoundary(isBoundary);

    for (int facePosition = 0; facePosition < 2; facePosition++) {
      const auto newTimeStamp = std::max(
        {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
          marker.getSelectedFaceNumber()
        ).getNewTimeStamp(facePosition),
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getNewTimeStamp(facePosition)
      );
      const auto oldTimeStamp = std::max(
        {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
          marker.getSelectedFaceNumber()
        ).getOldTimeStamp(facePosition),
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getOldTimeStamp(facePosition)
      );
      const auto updatedTimeStamp = std::max(
        {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
          marker.getSelectedFaceNumber()
        ).getUpdatedTimeStamp(facePosition),
        {{FINE_GRID_FACE_LABEL_ACCESSOR}}.getUpdatedTimeStamp(facePosition)
      );
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
        marker.getSelectedFaceNumber()
      ).setNewTimeStamp(facePosition, newTimeStamp);
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
        marker.getSelectedFaceNumber()
      ).setOldTimeStamp(facePosition, oldTimeStamp);
      {{COARSE_GRID_FACES_LABEL_ACCESSOR}}(
        marker.getSelectedFaceNumber()
      ).setUpdatedTimeStamp(
        facePosition,
        updatedTimeStamp
      );
    }
  }
"""

        # A created persistent face needs both states for the next roll-over.
        self._Template_CreateFace = """
  if (marker.isInteriorFaceWithinPatch()) {
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      ::toolbox::blockstructured::interpolateHaloLayerFromCoarseCell(
        marker,
        {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
        {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
        facePosition,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
      ::toolbox::blockstructured::interpolateHaloLayerFromCoarseCell(
        marker,
        {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
        {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
        facePosition,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }
  } else {
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      ::toolbox::blockstructured::interpolateHaloLayerWithTangentialSlope(
        marker,
        {{COARSE_GRID_FACES_Q_OLD_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
        {{FINE_GRID_FACE_Q_OLD_ACCESSOR}}.data(),
        facePosition,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
      ::toolbox::blockstructured::interpolateHaloLayerWithTangentialSlope(
        marker,
        {{COARSE_GRID_FACES_Q_NEW_ACCESSOR}}(marker.getSelectedFaceNumber()).data(),
        {{FINE_GRID_FACE_Q_NEW_ACCESSOR}}.data(),
        facePosition,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }
  }

  {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setNewTimeStamp(
    {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp()
  );
  {{FINE_GRID_FACE_LABEL_ACCESSOR}}.setOldTimeStamp(
    {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp()
  );
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
  constexpr int NumberOfAuxiliaryVariables = 0;
  constexpr int HaloSize = 0;
  const auto validationContext =
    std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): "
    + marker.toString();
  ::exahype2::fv::validatePatch(
    {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
    {{UNKNOWNS}},
    NumberOfAuxiliaryVariables,
    {{DOFS_PER_AXIS}},
    HaloSize,
    validationContext
  );
  ::exahype2::fv::validatePatch(
    {{FINE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
    {{UNKNOWNS}},
    NumberOfAuxiliaryVariables,
    {{DOFS_PER_AXIS}},
    HaloSize,
    validationContext
  );
#endif

  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStamp(
    {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp()
  );
  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStepSize(
    {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStepSize()
  );

  // A created cell inherits the parent's pending roll-over so its buffers swap
  // with those of its neighbours.
  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setHasUpdated(
    {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.getHasUpdated()
  );

  // Criterion sweeps inspect a created cell before descending further.
  {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setRequiresCriterionEvaluation(true);
"""

        self._Template_DestroyCell = """
  // The children of a coarsening parent restrict together once the whole
  // sibling set is staged, so the summation order cannot depend on the order
  // in which the traversal destroys them.
  const auto childIndex =
    peano4::dLinearised(marker.getRelativePositionWithinFatherCell(), 3);
  const auto siblingSetComplete = _solutionRestrictionStage.stage(
    {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
    childIndex,
    {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data()
  );
  _previousSolutionRestrictionStage.stage(
    {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
    childIndex,
    {{FINE_GRID_CELL_Q_OLD_ACCESSOR}}.data()
  );
  if (siblingSetComplete) {
    const {{STORAGE_PRECISION}}* stagedChildren[THREE_POWER_D];
    for (int child = 0; child < THREE_POWER_D; child++) {
      stagedChildren[child] = _solutionRestrictionStage.child(
        {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
        child
      );
    }
    ::toolbox::blockstructured::restrictCellFromChildren(
      {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
      stagedChildren,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}}
    );
    _solutionRestrictionStage.discard(
      {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data()
    );
    for (int child = 0; child < THREE_POWER_D; child++) {
      stagedChildren[child] = _previousSolutionRestrictionStage.child(
        {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
        child
      );
    }
    ::toolbox::blockstructured::restrictCellFromChildren(
      {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
      stagedChildren,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}}
    );
    _previousSolutionRestrictionStage.discard(
      {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data()
    );

#if PEANO_DEBUG
    constexpr int NumberOfAuxiliaryVariables = 0;
    constexpr int HaloSize = 0;
    const auto validationContext =
      std::string(__FILE__) + "(" + std::to_string(__LINE__) + "): "
      + marker.toString();
    ::exahype2::fv::validatePatch(
      {{COARSE_GRID_CELL_Q_NEW_ACCESSOR}}.data(),
      {{UNKNOWNS}},
      NumberOfAuxiliaryVariables,
      {{DOFS_PER_AXIS}},
      HaloSize,
      validationContext
    );
    ::exahype2::fv::validatePatch(
      {{COARSE_GRID_CELL_Q_OLD_ACCESSOR}}.data(),
      {{UNKNOWNS}},
      NumberOfAuxiliaryVariables,
      {{DOFS_PER_AXIS}},
      HaloSize,
      validationContext
    );
#endif
  }

  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStamp(
    {{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp()
  );
  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setTimeStepSize(
    {{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStepSize()
  );

  // A newly exposed parent inherits the children's pending roll-over so its
  // buffers swap with those of its neighbours.
  if ({{FINE_GRID_CELL_LABEL_ACCESSOR}}.getHasUpdated()) {
    {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setHasUpdated(true);
  }

  // Criterion sweeps inspect a newly exposed parent before ascending further.
  {{COARSE_GRID_CELL_LABEL_ACCESSOR}}.setRequiresCriterionEvaluation(true);
"""

        # Inter-grid face transfer can leave the two halves inconsistent with
        # their cells and produce unequal Riemann data. Projection repairs
        # coarsening parents and every requested face half, including the half
        # owned by a pre-existing neighbour. Completed halves clear their
        # requests so later refinement sweeps skip settled cells.
        self._dictionary["PROJECT_GUARD"] = solver._store_cell_data_default_guard()
        self._dictionary["DOFS_PER_AXIS"] = int(solver._patch.dim[0])
        self._dictionary["OVERLAP"] = int(solver._patch_overlap_new.dim[0] // 2)
        self._dictionary["UNKNOWNS"] = int(solver._patch.no_of_unknowns)
        self._Template_ProjectPatchOntoFaces = """
  auto requiresProjection =
    marker.hasBeenRefined() and not marker.willBeRefined();
  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    const auto cellFacePosition = faceNumber < DIMENSIONS ? 1 : 0;
    requiresProjection = requiresProjection
      or (not {{FINE_GRID_FACES_LABEL_ACCESSOR}}(faceNumber).getIsHanging()
          and {{FINE_GRID_FACES_LABEL_ACCESSOR}}(
            faceNumber
          ).getRequiresDataTransfer(cellFacePosition));
  }
  if ({{PROJECT_GUARD}} and requiresProjection) {
    const auto* const cellState = {{FINE_GRID_CELL_Q_NEW_ACCESSOR}}.data();

    {{STORAGE_PRECISION}}* faceHalos[TWO_TIMES_D]{};
    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
      faceHalos[faceNumber] =
        {{FINE_GRID_FACES_Q_NEW_ACCESSOR}}(faceNumber).data();
    }

    ::toolbox::blockstructured::projectPatch<
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{STORAGE_PRECISION}}
    >(cellState, faceHalos);

    for (int direction = 0; direction < DIMENSIONS; direction++) {
      {{FINE_GRID_FACES_LABEL_ACCESSOR}}(direction).setUpdated(1, true);
      {{FINE_GRID_FACES_LABEL_ACCESSOR}}(direction).setUpdatedTimeStamp(
        1,
        {{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp()
      );
      {{FINE_GRID_FACES_LABEL_ACCESSOR}}(
        direction + DIMENSIONS
      ).setUpdated(0, true);
      {{FINE_GRID_FACES_LABEL_ACCESSOR}}(
        direction + DIMENSIONS
      ).setUpdatedTimeStamp(
        0,
        {{FINE_GRID_CELL_LABEL_ACCESSOR}}.getTimeStamp()
      );
    }
    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
      const auto cellFacePosition = faceNumber < DIMENSIONS ? 1 : 0;
      if (not {{FINE_GRID_FACES_LABEL_ACCESSOR}}(faceNumber).getIsHanging()) {
        {{FINE_GRID_FACES_LABEL_ACCESSOR}}(
          faceNumber
        ).setRequiresDataTransfer(cellFacePosition, false);
      }
    }
    // The criterion pass precedes projection and has inspected created cells.
    {{FINE_GRID_CELL_LABEL_ACCESSOR}}.setRequiresCriterionEvaluation(false);
  }
"""

    def get_body_of_operation(self, operation_name):
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            return jinja2.Template(self._Template_ProjectPatchOntoFaces).render(
                **self._dictionary
            )
        elif (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
        ):
            return jinja2.Template(self._Template_TouchFaceLastTime).render(
                **self._dictionary
            )
        return super().get_body_of_operation(operation_name)

    def get_attributes(self):
        volumes_per_axis = self._dictionary["DOFS_PER_AXIS"]
        unknowns = self._dictionary["UNKNOWNS"]
        values_per_child = (
            f"(DIMENSIONS == 2 ? {volumes_per_axis * volumes_per_axis}"
            f" : {volumes_per_axis ** 3}) * {unknowns}"
        )
        precision = self._dictionary["STORAGE_PRECISION"]
        return f"""{super().get_attributes()}
    ::toolbox::blockstructured::ChildDataStage<{precision}>
      _solutionRestrictionStage{{THREE_POWER_D, {values_per_child}}};
    ::toolbox::blockstructured::ChildDataStage<{precision}>
      _previousSolutionRestrictionStage{{THREE_POWER_D, {values_per_child}}};
"""

    def get_body_of_getGridControlEvents(self):
        return (
            "  return ::exahype2::RefinementControlService::getInstance()"
            ".getGridControlEvents();\n"
        )

    def get_includes(self):
        return f"""{super().get_includes()}
#include "exahype2/RefinementControlService.h"
#include "peano4/Globals.h"
#include "toolbox/blockstructured/ChildDataStage.h"
#include "toolbox/blockstructured/ProjectPatch.h"
"""
