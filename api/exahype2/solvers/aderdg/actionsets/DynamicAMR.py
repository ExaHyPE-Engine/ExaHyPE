# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet
from ..kernels.DGMatrices import create_triadic_transfer_matrices

from exahype2.solvers.LagrangeBasis import render_tensor_1

import jinja2
import peano4.solversteps


class AdaptiveMeshFaceCoupling(AbstractADERDGActionSet):
    """
    Per-time-step resolution-transition coupling for ADER-DG adaptive meshes.

    On an adaptive mesh, every time step interpolates the coarse solution down
    onto the hanging faces of the finer level and solves the Riemann problem
    there, plus restricts the fine fluxes back onto the coarse face. This action
    set does that coupling on a stable grid: it switches the create/destroy
    inter-grid transfer off and reports no grid-control events. Changing the
    topology (creating/destroying cells and faces and moving the solution across
    the resolution transitions) is the job of the dedicated DynamicAMR observer's
    DynamicAMRTransfer.

    The per-step coupling is this class; the one-off topology-change transfer is
    DynamicAMRTransfer below. They mirror the FV solver's AdaptiveMeshFaceCoupling
    / DynamicAMRTransfer split.
    """

    def __init__(
        self,
        solver,
        delete_guard="false",
        interpolate_guard="false",
        restrict_guard="false",
        solve_guard="false",
    ):
        super().__init__(solver)
        self._delete_guard = delete_guard
        self._interpolate_guard = interpolate_guard
        self._restrict_guard = restrict_guard
        self._solve_guard = solve_guard

        from exahype2.solvers.aderdg.ADERDG import Polynomials

        number_of_dofs_per_axis = solver._order + 1
        prolongation, restriction = create_triadic_transfer_matrices(
            (
                "lobatto"
                if solver._polynomials is Polynomials.Gauss_Lobatto
                else "legendre"
            ),
            number_of_dofs_per_axis,
        )
        # The transfer kernels read both operators input-major: the prolongation
        # by coarse node, the restriction by fine node.
        self._prolongation_matrices = [
            render_tensor_1([value for row in matrix for value in row])
            for matrix in prolongation
        ]
        self._restriction_matrices = [
            render_tensor_1(
                [
                    matrix[coarse][fine]
                    for fine in range(number_of_dofs_per_axis)
                    for coarse in range(number_of_dofs_per_axis)
                ]
            )
            for matrix in restriction
        ]

        self._Template_TouchFaceFirstTime = """
  if ({{DELETE_GUARD}}) {
    constexpr int FluxElementsPerFace = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;

    const auto facePosition =
      fineGridFace{{SOLVER_NAME}}FaceLabel.getAboveHanging()[0] ? 0 : 1;
    auto* const outputFluxEstimates =
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.data()
      + facePosition * FluxElementsPerFace;

    // Flux restriction accumulates onto the fine-cell side, so it starts at zero.
    std::fill_n(outputFluxEstimates, FluxElementsPerFace, 0.0);
  }
"""

        self._Template_TouchHangingFaceFirstTime = """
  if ({{INTERPOLATE_GUARD}}) {
    const auto facePosition =
      marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;

    constexpr int NumberOfDofsPerAxis  = {{ORDER}} + 1;
    constexpr int FluxElementsPerFace  = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
    constexpr int BasisElementsPerFace = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;
    static constexpr double ProlongationMatrix1d[3][NumberOfDofsPerAxis * NumberOfDofsPerAxis] = {
      {{PROLONGATION_MATRIX_0}},
      {{PROLONGATION_MATRIX_1}},
      {{PROLONGATION_MATRIX_2}}
    };

    if (not marker.isInteriorFaceWithinPatch()) {
      // Interpolate the coarse neighbour's estimates and fluxes onto the half the fine
      // cells cannot supply. The fine half and the Riemann solve follow at the fine
      // cell's first touch, where the space-time predictor re-runs on the same input
      // the persistent faces' estimates came from.
      const auto subfacePosition = marker.getRelativePositionWithinFatherFace();
      const double* prolongationMatrixPerAxis[DIMENSIONS - 1];
      for (int tangentialAxis = 0; tangentialAxis < DIMENSIONS - 1; tangentialAxis++) {
        prolongationMatrixPerAxis[tangentialAxis] =
          ProlongationMatrix1d[subfacePosition(tangentialAxis)];
      }
      ::toolbox::blockstructured::interpolateFaceFromCoarse(
        fineGridFace{{SOLVER_NAME}}QEstimates.data() + facePosition * BasisElementsPerFace,
        coarseGridFaces{{SOLVER_NAME}}QEstimates(marker.getSelectedFaceNumber()).data()
          + facePosition * BasisElementsPerFace,
        prolongationMatrixPerAxis,
        NumberOfDofsPerAxis,
        {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
      );
      ::toolbox::blockstructured::interpolateFaceFromCoarse(
        fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + facePosition * FluxElementsPerFace,
        coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).data()
          + facePosition * FluxElementsPerFace,
        prolongationMatrixPerAxis,
        NumberOfDofsPerAxis,
        {{NUMBER_OF_UNKNOWNS}}
      );
    }
  }
"""

        # The subfaces of one coarse face restrict together once the sweep
        # leaves the face, so the summation order cannot depend on the order in
        # which the traversal visits the hanging subfaces.
        self._Template_TouchHangingFaceLastTime = """
  if ({{RESTRICT_GUARD}}) {
    constexpr int FluxElementsPerFace = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;

    const auto facePosition =
      marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;
    const auto subfacePosition = marker.getRelativePositionWithinFatherFace();
#if DIMENSIONS == 2
    const auto subfaceIndex = subfacePosition(0);
#else
    const auto subfaceIndex = subfacePosition(1) * 3 + subfacePosition(0);
#endif
    _fluxRestrictionStage.stage(
      coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).data()
        + (1 - facePosition) * FluxElementsPerFace,
      subfaceIndex,
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.data()
        + facePosition * FluxElementsPerFace
    );
  }
"""

        self._Template_TouchFaceLastTime = """
  if ({{RESTRICT_GUARD}}) {
    constexpr int NumberOfDofsPerAxis = {{ORDER}} + 1;
    constexpr int FluxElementsPerFace = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
    constexpr int NumberOfSubfaces = DIMENSIONS == 2 ? 3 : 9;
    static constexpr double RestrictionMatrix1d[3][NumberOfDofsPerAxis * NumberOfDofsPerAxis] = {
      {{RESTRICTION_MATRIX_0}},
      {{RESTRICTION_MATRIX_1}},
      {{RESTRICTION_MATRIX_2}}
    };

    for (int faceSide = 0; faceSide < 2; faceSide++) {
      auto* const restrictedFluxHalf =
        fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + faceSide * FluxElementsPerFace;
      if (_fluxRestrictionStage.hasFrame(restrictedFluxHalf)) {
        const double* restrictionMatrixPerInterval[3] = {
          RestrictionMatrix1d[0],
          RestrictionMatrix1d[1],
          RestrictionMatrix1d[2]
        };
        const {{CORRECTOR_COMPUTATION_PRECISION}}* stagedSubfaces[NumberOfSubfaces];
        for (int subface = 0; subface < NumberOfSubfaces; subface++) {
          stagedSubfaces[subface] = _fluxRestrictionStage.hasChild(restrictedFluxHalf, subface)
                                      ? _fluxRestrictionStage.child(restrictedFluxHalf, subface)
                                      : nullptr;
        }
        ::toolbox::blockstructured::restrictFaceFromChildren(
          restrictedFluxHalf,
          stagedSubfaces,
          restrictionMatrixPerInterval,
          NumberOfDofsPerAxis,
          {{NUMBER_OF_UNKNOWNS}}
        );
        _fluxRestrictionStage.discard(restrictedFluxHalf);
      }
    }
  }
"""

        # A hanging face carries no valid fine-side trace when it is touched, so the
        # fine cell reproduces it here. The trace the persistent faces hold was
        # predicted from the pre-prediction solution (kept in the _old buffer) with
        # the time step size the prediction stored in the cell label; re-running the
        # predictor on a scratch copy of exactly that state yields the identical
        # trace. The fused predictor would otherwise add the volume integral into
        # the live solution a second time. With both halves in place the Riemann
        # problems on the hanging faces are solved at fine resolution; the
        # restriction then feeds the same fluxes to the coarse cell.
        self._Template_SolveRiemannProblemsOnHangingFaces = """
  if ({{SOLVE_GUARD}}) {
    auto hasHangingFace = false;
    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
      hasHangingFace = hasHangingFace
        or fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getIsHanging();
    }
    if (hasHangingFace) {
      const auto timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

      constexpr int BasisElementsPerFace = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;
      constexpr int FluxElementsPerFace  = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
      // The predictor overwrites whatever buffer it runs on, so the hanging-cell re-run
      // works from a stack copy of the previous solution and leaves the live cell data
      // for the ordinary sweep.
#if DIMENSIONS == 2
      constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#else
      constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
#endif
      constexpr int CellUnknowns =
        SpaceBasisSize
        * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});

      // The scratch must be aligned like the heap-allocated cell buffer, because
      // the predictor uses aligned SIMD on this pointer.
      alignas(PEANO_WITH_ALIGNMENT) {{SOLUTION_STORAGE_PRECISION}}
        updatedStateScratch[CellUnknowns];
      std::copy_n(
        fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
        SpaceBasisSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}),
        updatedStateScratch
      );
      auto* const updatedState = updatedStateScratch;

      // The persistent faces keep the estimates the previous sweep projected, so they
      // receive scratch targets; only the hanging faces' own halves are written.
      alignas(PEANO_WITH_ALIGNMENT) {{CORRECTOR_COMPUTATION_PRECISION}} estimatesScratch[TWO_TIMES_D][BasisElementsPerFace];
      alignas(PEANO_WITH_ALIGNMENT) {{CORRECTOR_COMPUTATION_PRECISION}} fluxesScratch[TWO_TIMES_D][FluxElementsPerFace];
      {{CORRECTOR_COMPUTATION_PRECISION}}* stateEstimates[TWO_TIMES_D];
      {{CORRECTOR_COMPUTATION_PRECISION}}* fluxEstimates[TWO_TIMES_D];
      for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
        const auto isHangingFace =
          fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getIsHanging();
        const auto ownHalfEstimates =
          faceNumber < DIMENSIONS ? BasisElementsPerFace : 0;
        const auto ownHalfFluxes =
          faceNumber < DIMENSIONS ? FluxElementsPerFace : 0;
        stateEstimates[faceNumber] = isHangingFace
          ? fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(faceNumber).data()
            + ownHalfEstimates
          : estimatesScratch[faceNumber];
        fluxEstimates[faceNumber] = isHangingFace
          ? fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(faceNumber).data()
            + ownHalfFluxes
          : fluxesScratch[faceNumber];
      }

      {
        const auto timeStepSize =
          fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
        // User preprocessing kernels use the established QNew and QOld names.
        [[maybe_unused]] auto* const QNew = updatedState;
        [[maybe_unused]] auto* const QOld =
          fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data();
        {{PREPROCESS_RECONSTRUCTED_PATCH}}
        {{SPACE_TIME_PREDICTOR_CALL}}
      }

      {{COMPUTE_TIME_STEP_SIZE}}
      for (int hangingFace = 0; hangingFace < TWO_TIMES_D; hangingFace++) {
        if (fineGridFaces{{SOLVER_NAME}}FaceLabel(hangingFace).getIsHanging()) {
          auto* const leftFluxEstimates =
            fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(hangingFace).data();
          auto* const rightFluxEstimates =
            leftFluxEstimates + FluxElementsPerFace;
          auto* const leftStateEstimates =
            fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(hangingFace).data();
          auto* const rightStateEstimates =
            leftStateEstimates + BasisElementsPerFace;
          const auto direction = hangingFace % DIMENSIONS;
          const auto faceNumber = hangingFace;
          const auto isBoundary =
            fineGridFaces{{SOLVER_NAME}}FaceLabel(hangingFace).getBoundary();
          auto faceCentre = marker.getX();
          faceCentre(direction) +=
            (hangingFace < DIMENSIONS ? -0.5 : 0.5)
            * marker.getH()(direction);
          {{RIEMANN_SOLVER_CALL}}
        }
      }
    }
  }
"""

        # Topology-change (inter-grid transfer) operations are disabled for the
        # per-step coupling; DynamicAMRTransfer switches them back on.
        self._Template_CreateCell = ""
        self._Template_DestroyCell = ""
        self._Template_CreateFace = ""
        self._Template_DestroyFace = ""

    def get_action_set_name(self):
        return "exahype2_solvers_aderdg_actionsets_" + type(self).__name__

    def get_attributes(self):
        number_of_dofs_per_axis = self._solver._order + 1
        values_per_subface = (
            f"(DIMENSIONS == 2 ? {number_of_dofs_per_axis}"
            f" : {number_of_dofs_per_axis * number_of_dofs_per_axis})"
            f" * {self._solver._variable_shortcuts.unknowns}"
        )
        return (
            super().get_attributes()
            + f"""
    ::toolbox::blockstructured::ChildDataStage<{self._solver._corrector_computation_precision}>
      _fluxRestrictionStage{{DIMENSIONS == 2 ? 3 : 9, {values_per_subface}}};
"""
        )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        dictionary = {}
        self._solver._init_dictionary_with_default_parameters(dictionary)
        self._solver.add_entries_to_text_replacement_dictionary(dictionary)
        for interval in range(3):
            dictionary[f"PROLONGATION_MATRIX_{interval}"] = self._prolongation_matrices[
                interval
            ]
            dictionary[f"RESTRICTION_MATRIX_{interval}"] = self._restriction_matrices[
                interval
            ]

        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            dictionary["DELETE_GUARD"] = self._delete_guard
            result = jinja2.Template(self._Template_TouchFaceFirstTime).render(
                **dictionary
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
        ):
            dictionary["RESTRICT_GUARD"] = self._restrict_guard
            result = jinja2.Template(self._Template_TouchFaceLastTime).render(
                **dictionary
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            dictionary["SOLVE_GUARD"] = self._solve_guard
            result = jinja2.Template(
                self._Template_SolveRiemannProblemsOnHangingFaces
            ).render(**dictionary)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME
        ):
            dictionary["INTERPOLATE_GUARD"] = self._interpolate_guard
            result = jinja2.Template(self._Template_TouchHangingFaceFirstTime).render(
                **dictionary
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME
        ):
            dictionary["RESTRICT_GUARD"] = self._restrict_guard
            result = jinja2.Template(self._Template_TouchHangingFaceLastTime).render(
                **dictionary
            )
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_FACE:
            result = jinja2.Template(self._Template_CreateFace).render(**dictionary)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_DESTROY_FACE:
            result = jinja2.Template(self._Template_DestroyFace).render(**dictionary)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_CELL:
            result = jinja2.Template(self._Template_CreateCell).render(**dictionary)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_DESTROY_CELL:
            result = jinja2.Template(self._Template_DestroyCell).render(**dictionary)
        return result

    def get_includes(self):
        return (
            super().get_includes()
            + """
#include "tarch/CompilerSpecificSettings.h"
#include "toolbox/blockstructured/AMRRoutines.h"
#include "toolbox/blockstructured/ChildDataStage.h"
"""
        )


class DynamicAMRTransfer(AdaptiveMeshFaceCoupling):
    """
    The action set that realises ADER-DG topology changes.

    It lives in the dedicated DynamicAMR observer and runs on a stable,
    completed-time-step grid. It (i) hands the committed refine/coarsen events
    back to Peano (get_body_of_getGridControlEvents) so the trigger/realise
    traversals create/destroy cells and faces, and (ii) prolongs the DG solution
    and face estimates onto cells/faces Peano creates and restricts them back from
    cells/faces Peano destroys.

    No PDE update happens here, so the inherited hanging-face coupling is switched
    off. Mirrors exahype2.solvers.fv.actionsets DynamicAMRTransfer.
    """

    def __init__(
        self,
        solver,
        delete_guard="false",
        interpolate_guard="false",
        restrict_guard="false",
    ):
        super().__init__(solver, delete_guard, interpolate_guard, restrict_guard)

        # The hanging-face coupling stays off in the realise sweeps: no solver reads the
        # halos here, and the next time step rebuilds them on the settled grid.
        self._Template_TouchHangingFaceFirstTime = ""
        self._Template_TouchHangingFaceLastTime = ""
        self._Template_TouchFaceLastTime = ""
        self._Template_SolveRiemannProblemsOnHangingFaces = ""

        # Created and destroyed faces need no estimate transfer: every consumed slot
        # is rewritten before its first read. The deferred projector in an unlimited
        # solver, or the next Predict sweep in a limited solver, refills both halves
        # of a created face between local leaf cells; the following
        # prediction-on-hanging-cells sweep clears and restricts the fine-side flux
        # halves of faces above hanging faces, and the boundary handling refills
        # domain-wall halves at the start of the next time step. The face label's
        # RequiresDataTransfer entries, set on creation, tell the projector which
        # halves still need their estimates; the face-label merge carries them
        # across tree boundaries. Only the time-stamp labels carry over.
        self._Template_CreateFace = """
  for (int facePosition = 0; facePosition < 2; facePosition++) {
    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(
      facePosition,
      std::max(
        coarseGridFaces{{SOLVER_NAME}}FaceLabel(
          marker.getSelectedFaceNumber()
        ).getNewTimeStamp(facePosition),
        fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(facePosition)
      )
    );
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(
      facePosition,
      std::max(
        coarseGridFaces{{SOLVER_NAME}}FaceLabel(
          marker.getSelectedFaceNumber()
        ).getOldTimeStamp(facePosition),
        fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(facePosition)
      )
    );
  }
"""

        self._Template_DestroyFace = ""

        self._Template_CreateCell = (
            """
  constexpr int NumberOfDofsPerAxis = {{ORDER}} + 1;
  constexpr int NumberOfData = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
  static constexpr double ProlongationMatrix1d[3][NumberOfDofsPerAxis * NumberOfDofsPerAxis] = {
    {{PROLONGATION_MATRIX_0}},
    {{PROLONGATION_MATRIX_1}},
    {{PROLONGATION_MATRIX_2}}
  };

  const double* prolongationMatrixPerAxis[DIMENSIONS];
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    prolongationMatrixPerAxis[axis] =
      ProlongationMatrix1d[marker.getRelativePositionWithinFatherCell()(axis)];
  }
  ::toolbox::blockstructured::applyTensorProductOrderInvariant(
    fineGridCell{{SOLVER_NAME}}Q.data(),
    coarseGridCell{{SOLVER_NAME}}Q.data(),
    prolongationMatrixPerAxis,
    NumberOfDofsPerAxis,
    NumberOfDofsPerAxis,
    NumberOfData
  );
  // The _old buffer holds the pre-prediction solution. An unlimited solver uses
  // it to re-derive the live prediction; a limited solver keeps the accepted live
  // solution and starts a new prediction in its next Predict sweep.
  ::toolbox::blockstructured::applyTensorProductOrderInvariant(
    fineGridCell{{SOLVER_NAME}}Q_old.data(),
    coarseGridCell{{SOLVER_NAME}}Q_old.data(),
    prolongationMatrixPerAxis,
    NumberOfDofsPerAxis,
    NumberOfDofsPerAxis,
    NumberOfData
  );

  fineGridCell"""
            + solver._cell_label.name
            + """.setTimeStamp(coarseGridCell"""
            + solver._cell_label.name
            + """.getTimeStamp());
  fineGridCell"""
            + solver._cell_label.name
            + """.setTimeStepSize(coarseGridCell"""
            + solver._cell_label.name
            + """.getTimeStepSize());
  // A created cell inherits its parent's update state and is marked for the
  // criterion pass, so the running mesh-refinement sweeps evaluate its data once.
  fineGridCell"""
            + solver._cell_label.name
            + """.setHasUpdated(coarseGridCell"""
            + solver._cell_label.name
            + """.getHasUpdated());
  fineGridCell"""
            + solver._cell_label.name
            + """.setRequiresCriterionEvaluation(true);
"""
        )

        self._Template_DestroyCell = (
            """
  constexpr int NumberOfDofsPerAxis = {{ORDER}} + 1;
  constexpr int NumberOfData = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
  static constexpr double RestrictionMatrix1d[3][NumberOfDofsPerAxis * NumberOfDofsPerAxis] = {
    {{RESTRICTION_MATRIX_0}},
    {{RESTRICTION_MATRIX_1}},
    {{RESTRICTION_MATRIX_2}}
  };

  // The children of a coarsening parent restrict together once the whole
  // sibling set is staged, so the summation order cannot depend on the order
  // in which the traversal destroys them.
  const auto childIndex = peano4::dLinearised(marker.getRelativePositionWithinFatherCell(), 3);
  const auto siblingSetComplete = _solutionRestrictionStage.stage(
    coarseGridCell{{SOLVER_NAME}}Q.data(),
    childIndex,
    fineGridCell{{SOLVER_NAME}}Q.data()
  );
  // The _old buffer holds the pre-prediction solution. An unlimited solver uses
  // it to re-derive the live prediction; a limited solver keeps the restricted
  // accepted solution and starts a new prediction in its next Predict sweep.
  _previousSolutionRestrictionStage.stage(
    coarseGridCell{{SOLVER_NAME}}Q_old.data(),
    childIndex,
    fineGridCell{{SOLVER_NAME}}Q_old.data()
  );
  if (siblingSetComplete) {
    const double* restrictionMatrixPerInterval[3] = {
      RestrictionMatrix1d[0],
      RestrictionMatrix1d[1],
      RestrictionMatrix1d[2]
    };
    const {{SOLUTION_STORAGE_PRECISION}}* stagedChildren[THREE_POWER_D];
    for (int child = 0; child < THREE_POWER_D; child++) {
      stagedChildren[child] =
        _solutionRestrictionStage.child(coarseGridCell{{SOLVER_NAME}}Q.data(), child);
    }
    ::toolbox::blockstructured::restrictCellFromChildren(
      coarseGridCell{{SOLVER_NAME}}Q.data(),
      stagedChildren,
      restrictionMatrixPerInterval,
      NumberOfDofsPerAxis,
      NumberOfData
    );
    _solutionRestrictionStage.discard(coarseGridCell{{SOLVER_NAME}}Q.data());
    for (int child = 0; child < THREE_POWER_D; child++) {
      stagedChildren[child] =
        _previousSolutionRestrictionStage.child(coarseGridCell{{SOLVER_NAME}}Q_old.data(), child);
    }
    ::toolbox::blockstructured::restrictCellFromChildren(
      coarseGridCell{{SOLVER_NAME}}Q_old.data(),
      stagedChildren,
      restrictionMatrixPerInterval,
      NumberOfDofsPerAxis,
      NumberOfData
    );
    _previousSolutionRestrictionStage.discard(coarseGridCell{{SOLVER_NAME}}Q_old.data());
  }

  // The parent's coarsening marker lasts one sweep only, while the deferred
  // projector runs once every committed grid-control event has been realised.
  // Flagging the parent's face halves here carries the pending re-derivation to
  // that point.
  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    const auto ownSide = faceNumber < DIMENSIONS ? 1 : 0;
    coarseGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).setRequiresDataTransfer(
      ownSide,
      true
    );
  }

  coarseGridCell"""
            + solver._cell_label.name
            + """.setTimeStamp(fineGridCell"""
            + solver._cell_label.name
            + """.getTimeStamp());
  coarseGridCell"""
            + solver._cell_label.name
            + """.setTimeStepSize(fineGridCell"""
            + solver._cell_label.name
            + """.getTimeStepSize());
"""
        )

        # For an unlimited pipelined solver, re-derive a cell's live solution and
        # its own-side face estimates from the pre-prediction solution in the _old
        # buffer. Between grid sweeps the live
        # buffer already carries the volume-integral part of the next update and the
        # persistent faces carry the matching predicted traces, so a cell whose _old
        # buffer the create/destroy transfer just filled (a created cell or a
        # coarsening parent) obtains exactly the state it would hold had it existed
        # during the last prediction: the prediction is re-run in place with the
        # time step size the cell label carries. The pass runs only where the
        # topology change left work behind: a coarsening parent, or a cell with a
        # face half whose RequiresDataTransfer entry the face creation set. The
        # second condition also reaches the pre-existing neighbour of a created
        # face, whose re-run supplies the other half of that face's estimates, and
        # cells the surrounded-vertex post-refinement creates without a committed
        # grid-control event. The pass is deferred until the mesh-refinement sweeps
        # have no further committed events, so it runs once per changed cell on the
        # settled topology and no fabricated state is ever prolonged further down;
        # while those sweeps still realise deeper levels, only the pure solution in
        # the _old buffers flows down. A coarsening parent keeps its marker only in
        # the sweep that removes its children, so it is flagged through its faces
        # too. In the very first round of mesh-refinement sweeps no prediction has
        # run yet and the label carries a zero time step size; the prolonged data is
        # already consistent then and the initial prediction follows on the settled
        # grid. A limiter pipeline instead ends with an accepted live solution;
        # its branch below retains that solution and leaves face reconstruction to
        # the next ordinary Predict sweep.
        self._project_guard = solver._store_cell_data_default_guard()
        self._Template_TouchCellLastTime = """
{% if SUBCELL_LIMITER %}
  // A limiting step ends with the accepted solution in the live buffer. Its
  // next Predict sweep backs up that solution and rebuilds every face estimate,
  // so reconstructing the preceding prediction here would discard the accepted
  // finite-volume correction.
  if ({{PROJECT_GUARD}}) {
    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
      const auto ownSide = faceNumber < DIMENSIONS ? 1 : 0;
      if (not fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getIsHanging()) {
        fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).setRequiresDataTransfer(
          ownSide,
          false
        );
      }
    }
  }
{% else %}
  auto requiresProjection = marker.hasBeenRefined() and not marker.willBeRefined();
  for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
    const auto ownSide = faceNumber < DIMENSIONS ? 1 : 0;
    requiresProjection = requiresProjection
      or (not fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getIsHanging()
          and fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getRequiresDataTransfer(ownSide));
  }
  if ({{PROJECT_GUARD}} and requiresProjection
      and not ::exahype2::RefinementControlService::getInstance().hasGridControlEvents()
      and fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize() > 0.0) {
    const auto timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    const auto timeStepSize =
      fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();

    constexpr int BasisElementsPerFace = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;
    constexpr int FluxElementsPerFace  = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
#if DIMENSIONS == 2
    constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#else
    constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
#endif

    auto* const updatedState = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();
    std::copy_n(
      fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
      SpaceBasisSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}),
      updatedState
    );

    // User preprocessing kernels use the established QNew and QOld names.
    [[maybe_unused]] auto* const QNew = updatedState;
    [[maybe_unused]] auto* const QOld =
      fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data();
    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    {{CORRECTOR_COMPUTATION_PRECISION}}* stateEstimates[TWO_TIMES_D] = {
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0).data() + BasisElementsPerFace,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(1).data() + BasisElementsPerFace,
#if DIMENSIONS==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(2).data() + BasisElementsPerFace,
#endif
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0 + DIMENSIONS).data(),
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(1 + DIMENSIONS).data(),
#if DIMENSIONS==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(2 + DIMENSIONS).data()
#endif
    };

    {{CORRECTOR_COMPUTATION_PRECISION}}* fluxEstimates[TWO_TIMES_D] = {
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(0).data() + FluxElementsPerFace,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(1).data() + FluxElementsPerFace,
#if DIMENSIONS==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(2).data() + FluxElementsPerFace,
#endif
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(0 + DIMENSIONS).data(),
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(1 + DIMENSIONS).data(),
#if DIMENSIONS==3
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(2 + DIMENSIONS).data()
#endif
    };

    {{SPACE_TIME_PREDICTOR_CALL}}

    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; faceNumber++) {
      const auto ownSide = faceNumber < DIMENSIONS ? 1 : 0;
      if (not fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).getIsHanging()) {
        fineGridFaces{{SOLVER_NAME}}FaceLabel(faceNumber).setRequiresDataTransfer(
          ownSide,
          false
        );
      }
    }
  }
{% endif %}
  // The criterion pass runs before this final cell hook and has evaluated any
  // freshly created cell by now.
  if ({{PROJECT_GUARD}}) {
    fineGridCell{{SOLVER_NAME}}CellLabel.setRequiresCriterionEvaluation(false);
  }
"""

    def get_body_of_operation(self, operation_name):
        # Realise-observer cell hooks, delegating everything else (create/destroy
        # cell+face, the inherited per-step coupling templates) to
        # AdaptiveMeshFaceCoupling. touchCellLastTime (on leave) re-derives the
        # cell's live solution and its own face estimates from the _old buffer,
        # which by then has been prolonged (refine) or restricted (coarsen).
        dictionary = {}
        self._solver._init_dictionary_with_default_parameters(dictionary)
        self._solver.add_entries_to_text_replacement_dictionary(dictionary)
        for interval in range(3):
            dictionary[f"PROLONGATION_MATRIX_{interval}"] = self._prolongation_matrices[
                interval
            ]
            dictionary[f"RESTRICTION_MATRIX_{interval}"] = self._restriction_matrices[
                interval
            ]
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            dictionary["PROJECT_GUARD"] = self._project_guard
            return jinja2.Template(self._Template_TouchCellLastTime).render(
                **dictionary
            )
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_CELL:
            return jinja2.Template(self._Template_CreateCell).render(**dictionary)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_DESTROY_CELL:
            return jinja2.Template(self._Template_DestroyCell).render(**dictionary)
        return super().get_body_of_operation(operation_name)

    def get_attributes(self):
        number_of_dofs_per_axis = self._solver._order + 1
        number_of_data = (
            self._solver._variable_shortcuts.unknowns
            + self._solver._variable_shortcuts.auxiliary_variables
        )
        values_per_child = (
            f"(DIMENSIONS == 2 ? {number_of_dofs_per_axis * number_of_dofs_per_axis}"
            f" : {number_of_dofs_per_axis ** 3}) * {number_of_data}"
        )
        return (
            super().get_attributes()
            + f"""
    ::toolbox::blockstructured::ChildDataStage<{self._solver._solution_persistent_storage_precision}>
      _solutionRestrictionStage{{THREE_POWER_D, {values_per_child}}};
    ::toolbox::blockstructured::ChildDataStage<{self._solver._solution_persistent_storage_precision}>
      _previousSolutionRestrictionStage{{THREE_POWER_D, {values_per_child}}};
"""
        )

    def get_body_of_getGridControlEvents(self):
        # Hand the committed refine/coarsen events back to Peano so the
        # trigger/realise traversals change the topology here, on a stable grid.
        return "  return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();\n"

    def get_includes(self):
        return (
            super().get_includes()
            + """
#include "exahype2/RefinementControlService.h"
#include "peano4/Globals.h"
"""
        )
