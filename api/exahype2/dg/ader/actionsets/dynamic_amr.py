# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.actionsets


class AdaptiveMeshFaceCoupling(exahype2.actionsets.SolverActionSet):
    """Couples a hanging face to the coarse face it hangs below.

    The coarse cell supplies its half of a hanging face, the fine cells restrict their
    flux estimates onto the coarse face, and a cell that touches a hanging face runs its
    predictor again to fill the trace the hanging face carries.
    """

    _TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME = """
  if ({{INTERPOLATE_GUARD}}) {
    ::exahype2::dg::ader::interpolateHangingFace<{{FULL_QUALIFIED_SOLVER_NAME}}>(
      marker,
      kernels::{{SOLVER_NAME}}::DGMatrices<double>::ProlongationMatrix1D,
      coarseGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(marker.getSelectedFaceNumber()).data(),
      fineGridFace{{UNKNOWN_IDENTIFIER}}Estimates.data(),
      coarseGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(marker.getSelectedFaceNumber()).data(),
      fineGridFace{{UNKNOWN_IDENTIFIER}}FluxEstimates.data()
    );
  }
"""

    _TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME = """
  if ({{RESTRICT_GUARD}}) {
    ::exahype2::dg::ader::stageHangingFaceFlux<{{FULL_QUALIFIED_SOLVER_NAME}}>(
      marker,
      _fluxRestrictionStage,
      coarseGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(marker.getSelectedFaceNumber()).data(),
      fineGridFace{{UNKNOWN_IDENTIFIER}}FluxEstimates.data()
    );
  }
"""

    _TEMPLATE_TOUCH_FACE_LAST_TIME = """
  if ({{RESTRICT_GUARD}}) {
    ::exahype2::dg::ader::restrictFaceFluxHalves<{{FULL_QUALIFIED_SOLVER_NAME}}>(
      _fluxRestrictionStage,
      kernels::{{SOLVER_NAME}}::DGMatrices<double>::WeightedRestrictionMatrix1D,
      fineGridFace{{UNKNOWN_IDENTIFIER}}FluxEstimates.data()
    );
  }
"""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{SOLVE_GUARD}}) {
    {{COMPUTE_TIME_STEP_SIZE}}
    ::exahype2::dg::ader::solveRiemannProblemsOnHangingFaces<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{"true" if IS_LINEAR else "false"}},
      {{"true" if USE_FLUX else "false"}},
      {{"true" if USE_NCP else "false"}},
      {{"true" if USE_SOURCE else "false"}},
      {{"true" if USE_DIFFUSIVE_FLUX else "false"}},
      {{"true" if USE_DIFFUSIVE_SOURCE else "false"}},
      {{"true" if USE_MATERIAL_PARAMETERS else "false"}},
      {{"true" if USE_POINT_SOURCE else "false"}}
    >(
      marker,
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates,
      fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
      {{PREDICTOR_TABLES}},
      {{PICARD_TABLES}},
      timeStepSize,
      [&](
        auto* const                                  leftFluxEstimates,
        auto* const                                  rightFluxEstimates,
        auto* const                                  leftStateEstimates,
        auto* const                                  rightStateEstimates,
        const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
        const double                                 timeStamp,
        const double                                 timeStepSize,
        const int                                    direction,
        const bool                                   isBoundary,
        const int                                    faceNumber
      ) {
        ::exahype2::dg::ader::solveRiemannProblemOnEstimates<
          {{FULL_QUALIFIED_SOLVER_NAME}},
          {{DG_ORDER}} + 1,
          {{NUMBER_OF_UNKNOWNS}},
          {{NUMBER_OF_AUXILIARY_VARIABLES}},
          {{"true" if IS_LINEAR else "false"}},
          {{"true" if USE_RIEMANN_SOLVER else "false"}},
          {{"true" if USE_FLUX else "false"}},
          {{"true" if USE_NCP else "false"}},
          {{"true" if USE_MATERIAL_PARAMETERS else "false"}}
        >(
          leftFluxEstimates,
          rightFluxEstimates,
          leftStateEstimates,
          rightStateEstimates,
          kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::QuadratureWeightsOnFace,
          faceCentre,
          marker.getH(),
          timeStamp,
          timeStepSize,
          direction,
          isBoundary,
          faceNumber
        );
      }
    );
  }
"""

    _TEMPLATE_CREATE_FACE = ""
    _TEMPLATE_CREATE_CELL = ""
    _TEMPLATE_DESTROY_CELL = ""
    _TEMPLATE_TOUCH_CELL_LAST_TIME = ""

    def __init__(
        self,
        solver,
        interpolate_guard="false",
        restrict_guard="false",
        solve_guard="false",
    ):
        super().__init__(solver, descend_invocation_order=1)
        self.interpolate_guard = interpolate_guard
        self.restrict_guard = restrict_guard
        self.solve_guard = solve_guard

    @property
    def name(self):
        return "exahype2_dg_ader_actionsets_" + type(self).__name__

    def _entries(self):
        return {
            "INTERPOLATE_GUARD": self.interpolate_guard,
            "RESTRICT_GUARD": self.restrict_guard,
            "SOLVE_GUARD": self.solve_guard,
        }

    def operation_body(self, operation_name):
        operations = exahype2.actionsets.ActionSet
        template = {
            operations.OPERATION_TOUCH_FACE_LAST_TIME: self._TEMPLATE_TOUCH_FACE_LAST_TIME,
            operations.OPERATION_TOUCH_CELL_FIRST_TIME: self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
            operations.OPERATION_TOUCH_CELL_LAST_TIME: self._TEMPLATE_TOUCH_CELL_LAST_TIME,
            operations.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME: self._TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME,
            operations.OPERATION_TOUCH_HANGING_FACE_LAST_TIME: self._TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME,
            operations.OPERATION_CREATE_FACE: self._TEMPLATE_CREATE_FACE,
            operations.OPERATION_CREATE_CELL: self._TEMPLATE_CREATE_CELL,
            operations.OPERATION_DESTROY_CELL: self._TEMPLATE_DESTROY_CELL,
        }.get(operation_name)
        if template is None:
            return "\n"
        return self.render(template, **self._entries())

    def attributes(self):
        return self.render("""
  ::exahype2::amr::ChildDataStage<{{CORRECTOR_COMPUTATION_PRECISION}}> _fluxRestrictionStage{
    ::exahype2::amr::SubfacesPerFace,
    ::exahype2::dg::NodesPerFace<{{DG_ORDER}} + 1> * {{NUMBER_OF_UNKNOWNS}}
  };
""")

    def includes(self):
        return super().includes() + [
            "exahype2/dg/ader/Correct.h",
            "exahype2/dg/ader/DynamicAMR.h",
        ]


class DynamicAMRTransfer(AdaptiveMeshFaceCoupling):
    """Moves the solution across a level jump the mesh refinement has created."""

    # No solver reads the halos in the realise sweeps, and the next time step rebuilds them
    # on the settled grid.
    _TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME = ""
    _TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME = ""
    _TEMPLATE_TOUCH_FACE_LAST_TIME = ""
    _TEMPLATE_TOUCH_CELL_FIRST_TIME = ""

    _TEMPLATE_CREATE_FACE = """
  ::exahype2::amr::raiseFaceTimeStampsFromCoarseFace(
    coarseGridFaces{{SOLVER_NAME}}FaceLabel(marker.getSelectedFaceNumber()),
    fineGridFace{{SOLVER_NAME}}FaceLabel
  );
"""

    _TEMPLATE_CREATE_CELL = """
  ::exahype2::dg::ader::createCell<{{FULL_QUALIFIED_SOLVER_NAME}}>(
    marker,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::ProlongationMatrix1D,
    coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    coarseGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
    coarseGridCell{{SOLVER_NAME}}CellLabel,
    fineGridCell{{SOLVER_NAME}}CellLabel
  );
"""

    _TEMPLATE_DESTROY_CELL = """
  ::exahype2::dg::ader::destroyCell<{{FULL_QUALIFIED_SOLVER_NAME}}>(
    marker,
    _solutionRestrictionStage,
    _previousSolutionRestrictionStage,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>::WeightedRestrictionMatrix1D,
    coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    coarseGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
    coarseGridFaces{{SOLVER_NAME}}FaceLabel,
    coarseGridCell{{SOLVER_NAME}}CellLabel,
    fineGridCell{{SOLVER_NAME}}CellLabel
  );
"""

    _TEMPLATE_TOUCH_CELL_LAST_TIME = """
  if ({{PROJECT_GUARD}}) {
{%- if SUBCELL_LIMITER %}
    ::exahype2::dg::ader::releaseFacesAfterSubcellTransfer(
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridCell{{SOLVER_NAME}}CellLabel
    );
{%- else %}
    ::exahype2::dg::ader::rederivePrediction<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{"true" if IS_LINEAR else "false"}},
      {{"true" if USE_FLUX else "false"}},
      {{"true" if USE_NCP else "false"}},
      {{"true" if USE_SOURCE else "false"}},
      {{"true" if USE_DIFFUSIVE_FLUX else "false"}},
      {{"true" if USE_DIFFUSIVE_SOURCE else "false"}},
      {{"true" if USE_MATERIAL_PARAMETERS else "false"}},
      {{"true" if USE_POINT_SOURCE else "false"}}
    >(
      marker,
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
      {{PREDICTOR_TABLES}},
      {{PICARD_TABLES}}
    );
{%- endif %}
  }
"""

    def __init__(self, solver):
        super().__init__(solver)
        self.project_guard = solver._store_cell_data_default_guard()

    def _entries(self):
        return {"PROJECT_GUARD": self.project_guard}

    def grid_control_events_body(self):
        return "  return ::exahype2::amr::gridControlEvents();\n"

    def attributes(self):
        return self.render("""
  ::exahype2::amr::ChildDataStage<{{SOLUTION_STORAGE_PRECISION}}> _solutionRestrictionStage{
    peano4::sfc::ActiveCurve::ChildrenPerCell,
    ::exahype2::dg::NodesPerCell<{{DG_ORDER}} + 1> * {{NUMBER_OF_DATA}}
  };
  ::exahype2::amr::ChildDataStage<{{SOLUTION_STORAGE_PRECISION}}> _previousSolutionRestrictionStage{
    peano4::sfc::ActiveCurve::ChildrenPerCell,
    ::exahype2::dg::NodesPerCell<{{DG_ORDER}} + 1> * {{NUMBER_OF_DATA}}
  };
""")
