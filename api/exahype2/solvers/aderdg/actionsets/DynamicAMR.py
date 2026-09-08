# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

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
        super(AdaptiveMeshFaceCoupling, self).__init__(solver)
        self._delete_guard = delete_guard
        self._interpolate_guard = interpolate_guard
        self._restrict_guard = restrict_guard
        self._solve_guard = solve_guard

        self._Template_TouchFaceFirstTime = """
  if ({{DELETE_GUARD}}) {
    constexpr int FluxElementsPerFace = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;

    const int facePosition = fineGridFace{{SOLVER_NAME}}FaceLabel.getAboveHanging()[0] ? 0 : 1;
    auto* FOut = fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + facePosition * FluxElementsPerFace;

    // Delete the elements on the side of the face that the Riemann fluxes will be merged onto.
    // Because of the merge operations this must be on the side on which the fine cells are.
    std::fill_n(FOut, FluxElementsPerFace, 0);
  }
"""

        self._Template_TouchHangingFaceFirstTime = """
  if ({{INTERPOLATE_GUARD}}) {
    const int facePosition = marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;

    constexpr int FluxElementsPerFace  = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
    constexpr int BasisElementsPerFace = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;

    if (not marker.isInteriorFaceWithinPatch()) {
      // Interpolate the coarse neighbour's estimates and fluxes onto the half the fine
      // cells cannot supply. The fine half and the Riemann solve follow at the fine
      // cell's first touch, where the space-time predictor re-runs on the same input
      // the persistent faces' estimates came from.
      kernels::{{SOLVER_NAME}}::faceUnknownsProlongation(
        fineGridFace{{SOLVER_NAME}}QEstimates.data() + facePosition * BasisElementsPerFace,
        fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + facePosition * FluxElementsPerFace,
        coarseGridFaces{{SOLVER_NAME}}QEstimates(marker.getSelectedFaceNumber()).data() + facePosition * BasisElementsPerFace,
        coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).data() + facePosition * FluxElementsPerFace,
        marker.getRelativePositionWithinFatherFace()
      );
    }

    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(0, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(0), fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(1, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(1), fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(0, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(0), fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(0)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(1, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(1), fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(1)));
  }
"""

        self._Template_TouchHangingFaceLastTime = """
  if ({{RESTRICT_GUARD}}) {
    #if DIMENSIONS == 2
    constexpr int SpaceFaceSize = {{ORDER}} + 1;
    #else
    constexpr int SpaceFaceSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #endif

    constexpr int FluxElementsPerFace = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};

    const int facePosition = marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;

    kernels::{{SOLVER_NAME}}::faceFluxRestriction(
      coarseGridFaces{{SOLVER_NAME}}QFluxEstimates(marker.getSelectedFaceNumber()).data() + (1-facePosition) * FluxElementsPerFace, // lFhbndCoarse
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + facePosition * FluxElementsPerFace, // lFhbndFine
      marker.getRelativePositionWithinFatherFace() // subfaceIndex
    );
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
    bool hasHangingFace = false;
    for (int face = 0; face < TWO_TIMES_D; face++) {
      hasHangingFace = hasHangingFace or fineGridFaces{{SOLVER_NAME}}FaceLabel(face).getIsHanging();
    }
    if (hasHangingFace) {
      const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

      constexpr int BasisElementsPerFace = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;
      constexpr int FluxElementsPerFace  = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
      // The predictor overwrites whatever buffer it runs on, so the hanging-cell re-run
      // works from a stack copy of the previous solution and leaves the live cell data
      // for the ordinary sweep.
      constexpr int CellUnknowns = std::remove_reference_t<decltype(fineGridCell{{UNKNOWN_IDENTIFIER}})>::Cardinality;
#if DIMENSIONS == 2
      constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#else
      constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
#endif

      // The scratch must be aligned like the heap-allocated cell buffer, because
      // the predictor uses aligned SIMD on this pointer.
      using QStoreType = std::remove_cv_t<std::remove_pointer_t<decltype(fineGridCell{{UNKNOWN_IDENTIFIER}}.data())>>;
      alignas(PEANO_WITH_ALIGNMENT) QStoreType QProjScratch[CellUnknowns];
      std::copy_n(fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(), SpaceBasisSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}), QProjScratch);
      QStoreType* QNew = QProjScratch;

      // The persistent faces keep the estimates the previous sweep projected, so they
      // receive scratch targets; only the hanging faces' own halves are written.
      alignas(PEANO_WITH_ALIGNMENT) {{CORRECTOR_COMPUTATION_PRECISION}} estimatesScratch[TWO_TIMES_D][BasisElementsPerFace];
      alignas(PEANO_WITH_ALIGNMENT) {{CORRECTOR_COMPUTATION_PRECISION}} fluxesScratch[TWO_TIMES_D][FluxElementsPerFace];
      {{CORRECTOR_COMPUTATION_PRECISION}}* lQhbnd[TWO_TIMES_D];
      {{CORRECTOR_COMPUTATION_PRECISION}}* lFhbnd[TWO_TIMES_D];
      for (int face = 0; face < TWO_TIMES_D; face++) {
        const bool isHangingFace = fineGridFaces{{SOLVER_NAME}}FaceLabel(face).getIsHanging();
        const int  ownHalfEstimates = face < DIMENSIONS ? BasisElementsPerFace : 0;
        const int  ownHalfFluxes    = face < DIMENSIONS ? FluxElementsPerFace : 0;
        lQhbnd[face] = isHangingFace
          ? fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(face).data() + ownHalfEstimates
          : estimatesScratch[face];
        lFhbnd[face] = isHangingFace
          ? fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(face).data() + ownHalfFluxes
          : fluxesScratch[face];
      }

      {
        const double timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
        {{PREPROCESS_RECONSTRUCTED_PATCH}}
        {{SPACE_TIME_PREDICTOR_CALL}}
      }

      {{COMPUTE_TIME_STEP_SIZE}}
      for (int hangingFace = 0; hangingFace < TWO_TIMES_D; hangingFace++) {
        if (fineGridFaces{{SOLVER_NAME}}FaceLabel(hangingFace).getIsHanging()) {
          {{CORRECTOR_COMPUTATION_PRECISION}}* FL = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(hangingFace).data();
          {{CORRECTOR_COMPUTATION_PRECISION}}* FR = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(hangingFace).data() + FluxElementsPerFace;
          {{CORRECTOR_COMPUTATION_PRECISION}}* QL = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(hangingFace).data();
          {{CORRECTOR_COMPUTATION_PRECISION}}* QR = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(hangingFace).data() + BasisElementsPerFace;
          const int direction  = hangingFace % DIMENSIONS;
          const int faceNumber = hangingFace;
          const bool isBoundary = fineGridFaces{{SOLVER_NAME}}FaceLabel(hangingFace).getBoundary();
          auto faceCentre = marker.getX();
          faceCentre(direction) += (hangingFace < DIMENSIONS ? -0.5 : 0.5) * marker.getH()(direction);
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

    def get_body_of_operation(self, operation_name):
        result = "\n"
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)

        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            d["DELETE_GUARD"] = self._delete_guard
            result = jinja2.Template(self._Template_TouchFaceFirstTime).render(**d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d["SOLVE_GUARD"] = self._solve_guard
            result = jinja2.Template(
                self._Template_SolveRiemannProblemsOnHangingFaces
            ).render(**d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME
        ):
            d["INTERPOLATE_GUARD"] = self._interpolate_guard
            result = jinja2.Template(self._Template_TouchHangingFaceFirstTime).render(
                **d
            )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME
        ):
            d["RESTRICT_GUARD"] = self._restrict_guard
            result = jinja2.Template(self._Template_TouchHangingFaceLastTime).render(
                **d
            )
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_FACE:
            result = jinja2.Template(self._Template_CreateFace).render(**d)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_DESTROY_FACE:
            result = jinja2.Template(self._Template_DestroyFace).render(**d)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_CREATE_CELL:
            result = jinja2.Template(self._Template_CreateCell).render(**d)
        if operation_name == peano4.solversteps.ActionSet.OPERATION_DESTROY_CELL:
            result = jinja2.Template(self._Template_DestroyCell).render(**d)
        return result

    def get_includes(self):
        return (
            super(AdaptiveMeshFaceCoupling, self).get_includes()
            + """
#include "tarch/CompilerSpecificSettings.h"
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
        super(DynamicAMRTransfer, self).__init__(
            solver, delete_guard, interpolate_guard, restrict_guard
        )

        # The hanging-face coupling stays off in the realise sweeps: no solver reads the
        # halos here, and the next time step rebuilds them on the settled grid.
        self._Template_TouchHangingFaceFirstTime = ""
        self._Template_TouchHangingFaceLastTime = ""

        # Created and destroyed faces need no estimate transfer: every consumed slot
        # is rewritten before its first read. The projector below refills both halves
        # of a created face between local leaf cells, the following
        # prediction-on-hanging-cells sweep clears and restricts the fine-side flux
        # halves of faces above hanging faces, and the boundary handling refills
        # domain-wall halves at the start of the next time step. The face label's
        # RequiresDataTransfer entries, set on creation, tell the projector which
        # halves still need their estimates; the face-label merge carries them
        # across tree boundaries. Only the time-stamp labels carry over.
        self._Template_CreateFace = """
    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(0, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(0), fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(1, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getNewTimeStamp(1), fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(0, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(0), fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(0)));
    fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(1, std::max(coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()).getOldTimeStamp(1), fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(1)));
"""

        self._Template_DestroyFace = ""

        self._Template_CreateCell = (
            """
  kernels::{{SOLVER_NAME}}::cellUnknownsProlongation(
    fineGridCell{{SOLVER_NAME}}Q.data(),
    coarseGridCell{{SOLVER_NAME}}Q.data(),
    marker.getRelativePositionWithinFatherCell()
  );
  // The _old buffer holds the pre-prediction solution; the projector below re-derives
  // the created cell's live solution and face estimates from it.
  kernels::{{SOLVER_NAME}}::cellUnknownsProlongation(
    fineGridCell{{SOLVER_NAME}}Q_old.data(),
    coarseGridCell{{SOLVER_NAME}}Q_old.data(),
    marker.getRelativePositionWithinFatherCell()
  );

  fineGridCell"""
            + solver._cell_label.name
            + """.setTimeStamp( coarseGridCell"""
            + solver._cell_label.name
            + """.getTimeStamp() );
  fineGridCell"""
            + solver._cell_label.name
            + """.setTimeStepSize( coarseGridCell"""
            + solver._cell_label.name
            + """.getTimeStepSize() );
  // A created cell inherits its parent's update state and is marked for the
  // criterion pass, so the running mesh-refinement sweeps evaluate its data once.
  fineGridCell"""
            + solver._cell_label.name
            + """.setHasUpdated( coarseGridCell"""
            + solver._cell_label.name
            + """.getHasUpdated() );
  fineGridCell"""
            + solver._cell_label.name
            + """.setRequiresCriterionEvaluation( true );
"""
        )

        self._Template_DestroyCell = (
            """
  kernels::{{SOLVER_NAME}}::cellUnknownsRestriction(
    fineGridCell{{SOLVER_NAME}}Q.data(),
    coarseGridCell{{SOLVER_NAME}}Q.data(),
    marker.getRelativePositionWithinFatherCell()
  );
  // The _old buffer holds the pre-prediction solution; the projector below re-derives
  // the coarsening parent's live solution and face estimates from it.
  kernels::{{SOLVER_NAME}}::cellUnknownsRestriction(
    fineGridCell{{SOLVER_NAME}}Q_old.data(),
    coarseGridCell{{SOLVER_NAME}}Q_old.data(),
    marker.getRelativePositionWithinFatherCell()
  );

  // The parent's coarsening marker lasts one sweep only, while the deferred
  // projector runs once every committed grid-control event has been realised.
  // Flagging the parent's face halves here carries the pending re-derivation to
  // that point.
  for (int face = 0; face < TWO_TIMES_D; face++) {
    coarseGridFaces{{SOLVER_NAME}}FaceLabel(face).setRequiresDataTransfer(face < DIMENSIONS ? 1 : 0, true);
  }

  coarseGridCell"""
            + solver._cell_label.name
            + """.setTimeStamp( fineGridCell"""
            + solver._cell_label.name
            + """.getTimeStamp() );
  coarseGridCell"""
            + solver._cell_label.name
            + """.setTimeStepSize( fineGridCell"""
            + solver._cell_label.name
            + """.getTimeStepSize() );
"""
        )

        # Re-derive a cell's live solution and its own-side face estimates from the
        # pre-prediction solution in the _old buffer. Between grid sweeps the live
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
        # grid.
        self._project_guard = solver._store_cell_data_default_guard()
        self._Template_TouchCellLastTime = """
  bool requiresProjection = marker.hasBeenRefined() and not marker.willBeRefined();
  for (int face = 0; face < TWO_TIMES_D; face++) {
    const int ownSide = face < DIMENSIONS ? 1 : 0;
    requiresProjection = requiresProjection
      or (not fineGridFaces{{SOLVER_NAME}}FaceLabel(face).getIsHanging()
          and fineGridFaces{{SOLVER_NAME}}FaceLabel(face).getRequiresDataTransfer(ownSide));
  }
  if ({{PROJECT_GUARD}} and requiresProjection
      and not ::exahype2::RefinementControlService::getInstance().hasGridControlEvents()
      and fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize() > 0.0) {
    const double timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    const double timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();

    constexpr int BasisElementsPerFace = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;
    constexpr int FluxElementsPerFace  = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
#if DIMENSIONS == 2
    constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1);
#else
    constexpr int SpaceBasisSize = ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
#endif

    auto* QNew = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();
    for(int i=0; i<SpaceBasisSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}); i++){
      QNew[i] = fineGridCellADERDGSolverQ_old.data()[i];
    }

    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    {{CORRECTOR_COMPUTATION_PRECISION}}* lQhbnd[TWO_TIMES_D] = {
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

    {{CORRECTOR_COMPUTATION_PRECISION}}* lFhbnd[TWO_TIMES_D] = {
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

    for (int face = 0; face < TWO_TIMES_D; face++) {
      const int ownSide = face < DIMENSIONS ? 1 : 0;
      if (not fineGridFaces{{SOLVER_NAME}}FaceLabel(face).getIsHanging()) {
        fineGridFaces{{SOLVER_NAME}}FaceLabel(face).setRequiresDataTransfer(ownSide, false);
      }
    }
  }
  // The criterion pass of this sweep, which runs before this projector, has
  // evaluated any freshly created cell by now.
  if ({{PROJECT_GUARD}}) {
    fineGridCell{{SOLVER_NAME}}CellLabel.setRequiresCriterionEvaluation(false);
  }
"""

        # cellUnknownsRestriction (DestroyCell) accumulates each destroyed child
        # into the coarse parent with += and never zeroes the parent first. A
        # coarsening parent still has children in this sweep and loses them
        # afterwards, and such non-leaf cells are never initialised (the initial
        # condition and the predictor only touch leaves), so their buffers hold
        # stale values which, being handed along the space-filling curve, are
        # mirror-asymmetric. Accumulating the symmetric child restriction onto those
        # stale values yields an asymmetric coarse cell, which the projection above
        # then spreads. Zero the parent on enter, before its children restrict into
        # it on the descent.
        self._Template_ZeroCoarseningParent = """
  if (marker.hasBeenRefined() and not marker.willBeRefined()) {
    std::fill_n(
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      std::remove_reference_t<decltype(fineGridCell{{UNKNOWN_IDENTIFIER}})>::Cardinality,
      0
    );
    std::fill_n(
      fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
      std::remove_reference_t<decltype(fineGridCell{{UNKNOWN_IDENTIFIER}}_old)>::Cardinality,
      0
    );
  }
"""

    def get_body_of_operation(self, operation_name):
        # Realise-observer cell hooks, delegating everything else (create/destroy
        # cell+face, the inherited per-step coupling templates) to
        # AdaptiveMeshFaceCoupling:
        #  - touchCellFirstTime (on enter): zero a coarsening parent's buffers before
        #    its children restrict into it on the descent (clear-then-restrict).
        #  - touchCellLastTime  (on leave): re-derive the cell's live solution and its
        #    own face estimates from the _old buffer, which by then has been prolonged
        #    (refine) or restricted (coarsen).
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            return jinja2.Template(self._Template_ZeroCoarseningParent).render(**d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME
        ):
            d["PROJECT_GUARD"] = self._project_guard
            return jinja2.Template(self._Template_TouchCellLastTime).render(**d)
        return super(DynamicAMRTransfer, self).get_body_of_operation(operation_name)

    def get_body_of_getGridControlEvents(self):
        # Hand the committed refine/coarsen events back to Peano so the
        # trigger/realise traversals change the topology here, on a stable grid.
        return "  return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();\n"

    def get_includes(self):
        return (
            super(DynamicAMRTransfer, self).get_includes()
            + """
#include "exahype2/RefinementControlService.h"
"""
        )
