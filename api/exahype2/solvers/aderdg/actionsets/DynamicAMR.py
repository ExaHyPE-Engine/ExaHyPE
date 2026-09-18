# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet

from exahype2.solvers.LagrangeBasis import render_tensor_1
from exahype2.solvers.MathUtils import MathUtils

import jinja2
import peano4.solversteps


class AdaptiveMeshFaceCoupling(AbstractADERDGActionSet):
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
        if solver._polynomials is Polynomials.Gauss_Lobatto:
            weights, nodes = MathUtils.getGaussLobatto(number_of_dofs_per_axis)
            nodes = nodes[::-1]
        else:
            weights, nodes = MathUtils.getGaussLegendre(number_of_dofs_per_axis)
        # The transfer matrices follow from the nodes and weights of the basis,
        # so the generated code derives them for the subdivision factor of the
        # space-filling curve that the build selects.
        self._quadrature_nodes = render_tensor_1(nodes)
        self._quadrature_weights = render_tensor_1(weights)

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

    constexpr int FluxElementsPerFace  = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
    constexpr int BasisElementsPerFace = facedata::{{SOLVER_NAME}}QEstimates::Cardinality / 2;

    if (not marker.isInteriorFaceWithinPatch()) {
      // The fine cells supply one half of the face only, so the coarse neighbour's
      // estimates and fluxes fill the other half here.
      const auto    subfacePosition = marker.getRelativePositionWithinFatherFace();
      const double* prolongationMatrixPerAxis[DIMENSIONS - 1];
      for (int tangentialAxis = 0; tangentialAxis < DIMENSIONS - 1; tangentialAxis++) {
        prolongationMatrixPerAxis[tangentialAxis] =
          ProlongationMatrix1D.data()
          + subfacePosition(tangentialAxis) * EntriesPerTransferMatrix;
      }
      ::exahype2::amr::interpolateFaceFromCoarse(
        fineGridFace{{SOLVER_NAME}}QEstimates.data() + facePosition * BasisElementsPerFace,
        coarseGridFaces{{SOLVER_NAME}}QEstimates(marker.getSelectedFaceNumber()).data()
          + facePosition * BasisElementsPerFace,
        prolongationMatrixPerAxis,
        NumberOfDofsPerAxis,
        {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}
      );
      ::exahype2::amr::interpolateFaceFromCoarse(
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
    const auto subfaceIndex =
      subfacePosition(1) * peano4::sfc::ActiveCurve::SubdivisionFactor + subfacePosition(0);
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
    constexpr int FluxElementsPerFace = facedata::{{SOLVER_NAME}}QFluxEstimates::Cardinality / 2;
    constexpr int NumberOfSubfaces =
      peano4::Power<peano4::sfc::ActiveCurve::SubdivisionFactor, DIMENSIONS - 1>;

    for (int faceSide = 0; faceSide < 2; faceSide++) {
      auto* const restrictedFluxHalf =
        fineGridFace{{SOLVER_NAME}}QFluxEstimates.data() + faceSide * FluxElementsPerFace;
      if (_fluxRestrictionStage.hasFrame(restrictedFluxHalf)) {
        const double* restrictionMatrixPerInterval[peano4::sfc::ActiveCurve::SubdivisionFactor];
        for (int interval = 0; interval < peano4::sfc::ActiveCurve::SubdivisionFactor; interval++) {
          restrictionMatrixPerInterval[interval] =
            RestrictionMatrix1D.data() + interval * EntriesPerTransferMatrix;
        }
        const {{CORRECTOR_COMPUTATION_PRECISION}}* stagedSubfaces[NumberOfSubfaces];
        for (int subface = 0; subface < NumberOfSubfaces; subface++) {
          stagedSubfaces[subface] = _fluxRestrictionStage.hasChild(restrictedFluxHalf, subface)
                                      ? _fluxRestrictionStage.child(restrictedFluxHalf, subface)
                                      : nullptr;
        }
        ::exahype2::amr::restrictFaceFromChildren(
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
        # fine cell reproduces it here. Re-running the predictor on a scratch copy of
        # the _old buffer with the time step size the cell label carries yields the
        # trace the persistent faces hold.
        self._Template_SolveRiemannProblemsOnHangingFaces = """
  if ({{SOLVE_GUARD}}) {
    auto hasHangingFace = false;
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
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
      alignas(PEANO_WITH_ALIGNMENT) {{CORRECTOR_COMPUTATION_PRECISION}} estimatesScratch[peano4::FacesPerCell][BasisElementsPerFace];
      alignas(PEANO_WITH_ALIGNMENT) {{CORRECTOR_COMPUTATION_PRECISION}} fluxesScratch[peano4::FacesPerCell][FluxElementsPerFace];
      {{CORRECTOR_COMPUTATION_PRECISION}}* stateEstimates[peano4::FacesPerCell];
      {{CORRECTOR_COMPUTATION_PRECISION}}* fluxEstimates[peano4::FacesPerCell];
      for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
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
      for (int hangingFace = 0; hangingFace < peano4::FacesPerCell; hangingFace++) {
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
    ::exahype2::amr::ChildDataStage<{self._solver._corrector_computation_precision}>
      _fluxRestrictionStage{{
        peano4::Power<peano4::sfc::ActiveCurve::SubdivisionFactor, DIMENSIONS - 1>,
        {values_per_subface}}};
"""
        )

    def get_static_initialisations(self, full_qualified_classname):
        dictionary = {}
        self._solver._init_dictionary_with_default_parameters(dictionary)
        self._solver.add_entries_to_text_replacement_dictionary(dictionary)
        dictionary["QUADRATURE_NODES_1D"] = self._quadrature_nodes
        dictionary["QUADRATURE_WEIGHTS_1D"] = self._quadrature_weights
        return (
            super().get_static_initialisations(full_qualified_classname)
            + jinja2.Template(
                """
namespace {
  constexpr int NumberOfDofsPerAxis      = {{ORDER}} + 1;
  constexpr int EntriesPerTransferMatrix = NumberOfDofsPerAxis * NumberOfDofsPerAxis;

  constexpr double QuadratureNodes1D[NumberOfDofsPerAxis]   = {{QUADRATURE_NODES_1D}};
  constexpr double QuadratureWeights1D[NumberOfDofsPerAxis] = {{QUADRATURE_WEIGHTS_1D}};

  /**
   * Returns the transfer matrices with the two node indices of every child
   * block swapped, which is the layout the transfer routines read.
   */
  constexpr ::exahype2::dg::AMRMatrices1D<NumberOfDofsPerAxis> transposePerChild(
    const ::exahype2::dg::AMRMatrices1D<NumberOfDofsPerAxis>& matrices
  ) {
    ::exahype2::dg::AMRMatrices1D<NumberOfDofsPerAxis> result{};
    for (int child = 0; child < peano4::sfc::ActiveCurve::SubdivisionFactor; child++) {
      for (int row = 0; row < NumberOfDofsPerAxis; row++) {
        for (int column = 0; column < NumberOfDofsPerAxis; column++) {
          result[(child * NumberOfDofsPerAxis + column) * NumberOfDofsPerAxis + row] =
            matrices[(child * NumberOfDofsPerAxis + row) * NumberOfDofsPerAxis + column];
        }
      }
    }
    return result;
  }

  /**
   * Entry [child][coarse * NumberOfDofsPerAxis + fine] holds the coarse shape
   * function coarse at the node fine of the child interval child.
   */
  constexpr auto ProlongationMatrix1D = transposePerChild(
    ::exahype2::dg::interpolationMatrix1D<NumberOfDofsPerAxis>(QuadratureNodes1D)
  );

  /**
   * Entry [child][fine * NumberOfDofsPerAxis + coarse] holds the mass-lumped
   * projection of the node fine of the child interval child onto the coarse
   * shape function coarse.
   */
  constexpr auto RestrictionMatrix1D = transposePerChild(
    ::exahype2::dg::weightedRestrictionMatrix1D<NumberOfDofsPerAxis>(
      QuadratureNodes1D,
      QuadratureWeights1D
    )
  );
} // namespace
"""
            ).render(**dictionary)
        )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        dictionary = {}
        self._solver._init_dictionary_with_default_parameters(dictionary)
        self._solver.add_entries_to_text_replacement_dictionary(dictionary)

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
#include "exahype2/amr/AMRRoutines.h"
#include "exahype2/amr/ChildDataStage.h"
#include "exahype2/dg/AMRMatrices.h"
#include "peano4/sfc/ActiveCurve.h"
"""
        )


class DynamicAMRTransfer(AdaptiveMeshFaceCoupling):
    def __init__(
        self,
        solver,
        delete_guard="false",
        interpolate_guard="false",
        restrict_guard="false",
    ):
        super().__init__(solver, delete_guard, interpolate_guard, restrict_guard)

        # The hanging-face coupling stays off in the realise sweeps: No solver reads the
        # halos here, and the next time step rebuilds them on the settled grid.
        self._Template_TouchHangingFaceFirstTime = ""
        self._Template_TouchHangingFaceLastTime = ""
        self._Template_TouchFaceLastTime = ""
        self._Template_SolveRiemannProblemsOnHangingFaces = ""

        # Created and destroyed faces need no estimate transfer: Every consumed slot
        # is rewritten before its first read, and the RequiresDataTransfer entries
        # the face creation sets name the halves that still wait for their estimates.
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
  constexpr int NumberOfData = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};

  const double* prolongationMatrixPerAxis[DIMENSIONS];
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    prolongationMatrixPerAxis[axis] =
      ProlongationMatrix1D.data()
      + marker.getRelativePositionWithinFatherCell()(axis) * EntriesPerTransferMatrix;
  }
  ::exahype2::amr::applyTensorProductOrderInvariant(
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
  ::exahype2::amr::applyTensorProductOrderInvariant(
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
  // The criterion flag makes the running mesh-refinement sweeps evaluate the
  // created cell's data once.
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
  constexpr int NumberOfData = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};

  // The children of a coarsening parent restrict together once the whole
  // sibling set is staged, so the summation order cannot depend on the order
  // in which the traversal destroys them.
  const auto childIndex = peano4::dLinearised(
    marker.getRelativePositionWithinFatherCell(),
    peano4::sfc::ActiveCurve::SubdivisionFactor
  );
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
    const double* restrictionMatrixPerInterval[peano4::sfc::ActiveCurve::SubdivisionFactor];
    for (int interval = 0; interval < peano4::sfc::ActiveCurve::SubdivisionFactor; interval++) {
      restrictionMatrixPerInterval[interval] =
        RestrictionMatrix1D.data() + interval * EntriesPerTransferMatrix;
    }
    const {{SOLUTION_STORAGE_PRECISION}}* stagedChildren[peano4::sfc::ActiveCurve::ChildrenPerCell];
    for (int child = 0; child < peano4::sfc::ActiveCurve::ChildrenPerCell; child++) {
      stagedChildren[child] =
        _solutionRestrictionStage.child(coarseGridCell{{SOLVER_NAME}}Q.data(), child);
    }
    ::exahype2::amr::restrictCellFromChildren(
      coarseGridCell{{SOLVER_NAME}}Q.data(),
      stagedChildren,
      restrictionMatrixPerInterval,
      NumberOfDofsPerAxis,
      NumberOfData
    );
    _solutionRestrictionStage.discard(coarseGridCell{{SOLVER_NAME}}Q.data());
    for (int child = 0; child < peano4::sfc::ActiveCurve::ChildrenPerCell; child++) {
      stagedChildren[child] =
        _previousSolutionRestrictionStage.child(coarseGridCell{{SOLVER_NAME}}Q_old.data(), child);
    }
    ::exahype2::amr::restrictCellFromChildren(
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
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
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

        # An unlimited solver re-derives a cell's live solution and its own-side face
        # estimates from the pre-prediction solution in the _old buffer. The
        # re-derivation waits until the mesh-refinement sweeps hold no further
        # committed grid-control event, so it reads a settled topology.
        self._project_guard = solver._store_cell_data_default_guard()
        self._Template_TouchCellLastTime = """
{% if SUBCELL_LIMITER %}
  // A limiting step ends with the accepted solution in the live buffer. Its
  // next Predict sweep backs up that solution and rebuilds every face estimate,
  // so reconstructing the preceding prediction here would discard the accepted
  // finite-volume correction.
  if ({{PROJECT_GUARD}}) {
    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
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
  for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
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

    {{CORRECTOR_COMPUTATION_PRECISION}}* stateEstimates[peano4::FacesPerCell] = {
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

    {{CORRECTOR_COMPUTATION_PRECISION}}* fluxEstimates[peano4::FacesPerCell] = {
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

    for (int faceNumber = 0; faceNumber < peano4::FacesPerCell; faceNumber++) {
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
        dictionary = {}
        self._solver._init_dictionary_with_default_parameters(dictionary)
        self._solver.add_entries_to_text_replacement_dictionary(dictionary)
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
    ::exahype2::amr::ChildDataStage<{self._solver._solution_persistent_storage_precision}>
      _solutionRestrictionStage{{peano4::sfc::ActiveCurve::ChildrenPerCell, {values_per_child}}};
    ::exahype2::amr::ChildDataStage<{self._solver._solution_persistent_storage_precision}>
      _previousSolutionRestrictionStage{{peano4::sfc::ActiveCurve::ChildrenPerCell, {values_per_child}}};
"""
        )

    def get_body_of_getGridControlEvents(self):
        return "  return ::exahype2::RefinementControlService::getInstance().getGridControlEvents();\n"

    def get_includes(self):
        return (
            super().get_includes()
            + """
#include "exahype2/RefinementControlService.h"
#include "peano4/Constants.h"
"""
        )
