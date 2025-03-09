# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet
from exahype2.solvers.PDETerms import PDETerms

import jinja2
import peano4.solversteps


class Prediction(AbstractADERDGActionSet):
    """
    The extrapolated solution from the space-time predictor has to be projected onto
    the faces, so we can then solve the Riemann problems. So the projection
    happens in one grid sweep, the corresponding Riemann solve in the next one.
    """

    _Template_TouchCellFirstTime_Preamble = """
  bool isHangingCell = false;
  for (int d = 0; d < 2 * Dimensions; d++) {
    if (fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()) {
      isHangingCell = true;
      break;
    }
  }

  if ({{PREDICATE}}) {
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    // Needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}

    #if Dimensions == 2
    constexpr int SpaceFaceSize         = {{ORDER}} + 1;
    constexpr int SpaceBasisSize        = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #else
    constexpr int SpaceFaceSize         = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    constexpr int SpaceBasisSize        = ({{ORDER}} + 1) * ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #endif

    constexpr int SpaceTimeBasisSize    = SpaceBasisSize * ({{ORDER}} + 1);
    constexpr int BasisElementsPerFace  = kernels::{{SOLVER_NAME}}::getBndFaceSize();
    constexpr int FluxElementsPerFace   = kernels::{{SOLVER_NAME}}::getBndFluxSize();

    auto* Q = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
    auto* oldQ = fineGridCell{{UNKNOWN_IDENTIFIER}}_old.value;

    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    {{CREATE_PREDICTOR_ALLOCATIONS}}

    std::copy_n(Q, SpaceBasisSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}), oldQ);
"""

    _Template_TouchCellFirstTime_Core = """
    {{FUSED_PREDICTOR_VOLUME_INTEGRAL}}
    {{PROJECT_SOLUTION_TO_FACES}}
"""

    _Template_TouchCellFirstTime_Epilogue = """
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(timeStepSize);

    repositories::{{SOLVER_INSTANCE}}.update(timeStepSize, timeStamp, marker.h()(0));
  }
"""

    _Template_PredictorAllocations = """
    {{CORRECTOR_COMPUTATION_PRECISION}}  boundaryData[kernels::{{SOLVER_NAME}}::getBndFaceTotalSize() + kernels::{{SOLVER_NAME}}::getBndFluxTotalSize()];
    {{CORRECTOR_COMPUTATION_PRECISION}}* lQhbnd = boundaryData;
    {{CORRECTOR_COMPUTATION_PRECISION}}* lFhbnd = boundaryData + kernels::{{SOLVER_NAME}}::getBndFaceTotalSize();

    ::exahype2::CellData<{{SOLUTION_STORAGE_PRECISION}}, {{CORRECTOR_COMPUTATION_PRECISION}}> cellData(
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize,
      boundaryData
    );
"""

    _Template_FSTPVI_Linear_Without_PS = """
    // Linear, without point sources
    int numberOfIterations = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      lQhbnd, lFhbnd, cellData.QIn[0],
      marker.x(), marker.h(), timeStamp, timeStepSize,
      std::vector<int>() // Empty vector
    );
"""

    _Template_FSTPVI_Linear_With_PS = """
    const auto pointSources = kernels::{{SOLVER_NAME}}::pointSources(
      repositories::{{SOLVER_INSTANCE}},
      marker.x(),
      marker.h()
    );

    if (!pointSources.empty()) {
      // Linear, with point sources
      int numberOfIterations = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
        repositories::{{SOLVER_INSTANCE}},
        lQhbnd, lFhbnd, cellData.QIn[0],
        marker.x(), marker.h(), timeStamp, timeStepSize,
        pointSources
      );
    } else {
      // Linear, without point sources
      int numberOfIterations = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegralWithoutPointSources<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
        repositories::{{SOLVER_INSTANCE}},
        lQhbnd, lFhbnd, cellData.QIn[0],
        marker.x(), marker.h(), timeStamp, timeStepSize,
        pointSources
      );
    }
"""

    _Template_FSTPVI_Nonlinear = """
    // Nonlinear, without point sources
    int numberOfIterations = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[PRECISION_NUM]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      lQhbnd, lFhbnd, cellData.QIn[0],
      marker.x(), marker.h(), timeStamp, timeStepSize
    );
"""

    _Template_CopyEstimatesIntoFaces = """
    /**
     * Copy the estimates into the face data containers. Note that Peano uses a different ordering of
     * the faces that the kernels, so these are reordered somewhat.
     * The kernels order by dimension, then side of the face whereas Peano orders by
     * side, then dimension.
     * E.g., in 2d the kernels have the order left, right, lower face, upper face
     * whereas Peano orders: left, lower, right, upper.
     *
     * 2d: 0,1,2,3 -> 0,2,1,3
     * 3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
     */

    for (int d = 0; d < Dimensions; d++) {
      // Projected solution estimations
      std::copy_n(lQhbnd +  2 * d * BasisElementsPerFace, BasisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value + BasisElementsPerFace);
      std::copy_n(lQhbnd + (2 * d + 1) * BasisElementsPerFace, BasisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d + Dimensions).value);

      // Projected flux estimations
      std::copy_n(lFhbnd +  2 * d * FluxElementsPerFace, FluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value + FluxElementsPerFace);
      std::copy_n(lFhbnd + (2 * d + 1) * FluxElementsPerFace, FluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d + Dimensions).value);

      // Timestamp information for boundary handling
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setNewTimeStamp(1, timeStamp);
      fineGridFaces{{SOLVER_NAME}}FaceLabel(d + Dimensions).setNewTimeStamp(0, timeStamp);
    }
"""

    _Template_NumberOfPrecisionsIterator = """
    ::exahype2::PrecisionCommand myPrecision = repositories::{{SOLVER_INSTANCE}}.precisionCriterion(
      cellData.QIn[0],
      marker.x(),
      marker.h(),
      timeStepSize,
      repositories::{{SOLVER_INSTANCE}}.getSolverState()
    );

    switch (myPrecision) {
    {% for PRECISION_NUM in range(0,PREDICTOR_COMPUTATION_PRECISIONS|length) %}
      case ::exahype2::PrecisionCommand::{{PRECISIONS_CAPITALIZED[PRECISION_NUM]}}:
        {{ITERATION_CONTENT}}
        break;
    {% endfor %}
      default:
        assertion1WithExplanation(
          false, myPrecision, "chosen precision was not in the previously specified options, as such there are no compiled kernels for this option."
        );
    }
"""

    _Template_CopyEstimatesAndPerformRiemannSolutionOnHangingFaces = """
    /**
     * Copy the estimates into the face data containers. Note that Peano uses a different ordering of
     * the faces that the kernels, so these are reordered somewhat.
     * The kernels order by dimension, then side of the face whereas Peano orders by
     * side, then dimension.
     * E.g., in 2d the kernels have the order left, right, lower face, upper face
     * whereas Peano orders: left, lower, right, upper.
     *
     * 2d: 0,1,2,3 -> 0,2,1,3
     * 3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
     *
     * On hanging faces, this instead computes the Riemann solution and face integral, then
     * stores the solution of this into the face. The reason for this is that performing the
     * Riemann solution on the hanging cell and restricting these is much more accurate than
     * performing them on the coarse faces and projecting those to the fine faces, to the
     * degree that the later breaks many simulations.
     */

    for (int d = 0; d < Dimensions; d++) {
      int direction = d;

      // Left values
      if (fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()) {
        {{CORRECTOR_COMPUTATION_PRECISION}}* FL = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value;
        {{CORRECTOR_COMPUTATION_PRECISION}}* FR = lFhbnd + 2 * d * FluxElementsPerFace;
        {{CORRECTOR_COMPUTATION_PRECISION}}* QL = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value;
        {{CORRECTOR_COMPUTATION_PRECISION}}* QR = lQhbnd + 2 * d * BasisElementsPerFace ;
        int orientation = 0;
        int faceNumber  = d;
        bool isBoundary = false;
        tarch::la::Vector<Dimensions, double> faceCentre = marker.x();
        faceCentre[d] -= 0.5 * marker.h()[d];
        {{RIEMANN_SOLVER}}
        {{CORRECTOR_COMPUTATION_PRECISION}}* FIn = FR;
        {{FACE_INTEGRAL}}
      } else {
        // Project to faces and update face timestamp
        std::copy_n(lQhbnd + 2 * d * BasisElementsPerFace, BasisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value + BasisElementsPerFace);
        std::copy_n(lFhbnd + 2 * d * FluxElementsPerFace,  FluxElementsPerFace,  fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value + FluxElementsPerFace);
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d).setNewTimeStamp(1, timeStamp);
      }

      // Right values
      if (fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).getIsHanging()) {
        {{CORRECTOR_COMPUTATION_PRECISION}}* FL = lFhbnd + (2 * d + 1) * FluxElementsPerFace;
        {{CORRECTOR_COMPUTATION_PRECISION}}* FR = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d + Dimensions).value + FluxElementsPerFace;
        {{CORRECTOR_COMPUTATION_PRECISION}}* QL = lQhbnd + (2 * d + 1) * BasisElementsPerFace;
        {{CORRECTOR_COMPUTATION_PRECISION}}* QR = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d + Dimensions).value + BasisElementsPerFace;
        int orientation = 1;
        int faceNumber  = d + Dimensions;
        bool isBoundary = false;
        tarch::la::Vector<Dimensions, double> faceCentre = marker.x();
        faceCentre[d] += 0.5 * marker.h()[d];
        {{RIEMANN_SOLVER}}
        {{CORRECTOR_COMPUTATION_PRECISION}}* FIn = FL;
        {{FACE_INTEGRAL}}
      } else {
        // Project to faces and update face timestamp
        std::copy_n(lQhbnd + (2 * d + 1) * BasisElementsPerFace, BasisElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d + Dimensions).value);
        std::copy_n(lFhbnd + (2 * d + 1) * FluxElementsPerFace,  FluxElementsPerFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d + Dimensions).value);
        fineGridFaces{{SOLVER_NAME}}FaceLabel(d + Dimensions).setNewTimeStamp(0, timeStamp);
      }
    }
"""

    _Template_RiemannSolver = """
        kernels::{{SOLVER_NAME}}::riemannSolver<{{CORRECTOR_COMPUTATION_PRECISION}}>(
          repositories::{{SOLVER_INSTANCE}},
          FL,
          FR,
          QL,
          QR,
          timeStamp,
          timeStepSize,
          faceCentre,
          marker.h(),
          direction
        );
"""

    _Template_CustomRiemannSolver = """
        repositories::{{SOLVER_INSTANCE}}.riemannSolver(
          FL,
          FR,
          QL,
          QR,
          timeStamp,
          timeStepSize,
          marker.x(),
          faceCentre,
          direction,
          isBoundary,
          faceNumber
        );
"""

    _Template_FaceIntegral = """
        const double inverseDxDirection = 1.0 / marker.h()[d];
        kernels::{{SOLVER_NAME}}::faceIntegral(
          cellData.QIn[0],
          FIn,
          direction,
          orientation,
          inverseDxDirection,
          timeStepSize
        );
"""

    def __init__(self, solver, guard, on_hanging_cells=False):
        """
        guard: String (C++ code)
          Predicate which controls whether this action set should run
        on_hanging_cells: bool
          Determines whether this is running on hanging cells or not,
          the actions on both differ
        """
        super(Prediction, self).__init__(solver)
        self._guard = guard
        self._on_hanging_cells = on_hanging_cells
        self._is_linear = solver._is_linear
        self._use_custom_riemann_solver = (
            solver._riemann_solver_implementation != PDETerms.None_Implementation
        )
        self._use_point_source = (
            solver._point_sources_implementation != PDETerms.None_Implementation
        )
        self._multiple_precisions = len(solver._predictor_computation_precisions) > 1

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d[
                "PREDICATE"
            ] = self.guard  # self._guard TODO: Why does self._guard not work?
            d["PRECISIONS_CAPITALIZED"] = [
                x.replace(" ", "_").replace("std::", "").capitalize()
                for x in d["PREDICTOR_COMPUTATION_PRECISIONS"]
            ]
            d["CREATE_PREDICTOR_ALLOCATIONS"] = self._Template_PredictorAllocations
            if self._is_linear:
                if self._use_point_source:
                    d[
                        "FUSED_PREDICTOR_VOLUME_INTEGRAL"
                    ] = self._Template_FSTPVI_Linear_With_PS
                else:
                    d[
                        "FUSED_PREDICTOR_VOLUME_INTEGRAL"
                    ] = self._Template_FSTPVI_Linear_Without_PS
            else:
                d["FUSED_PREDICTOR_VOLUME_INTEGRAL"] = self._Template_FSTPVI_Nonlinear
            if self._on_hanging_cells:
                if self._use_custom_riemann_solver:
                    d["RIEMANN_SOLVER"] = jinja2.Template(
                        self._Template_CustomRiemannSolver
                    ).render(**d)
                else:
                    d["RIEMANN_SOLVER"] = jinja2.Template(
                        self._Template_RiemannSolver
                    ).render(**d)
                d["FACE_INTEGRAL"] = jinja2.Template(
                    self._Template_FaceIntegral
                ).render(**d)
                d[
                    "PROJECT_SOLUTION_TO_FACES"
                ] = self._Template_CopyEstimatesAndPerformRiemannSolutionOnHangingFaces
            else:
                d["PROJECT_SOLUTION_TO_FACES"] = self._Template_CopyEstimatesIntoFaces

            result = jinja2.Template(self._Template_TouchCellFirstTime_Preamble).render(
                **d
            )
            if self._multiple_precisions:
                d["ITERATION_CONTENT"] = jinja2.Template(
                    self._Template_TouchCellFirstTime_Core
                ).render(**d)
                result += jinja2.Template(
                    self._Template_NumberOfPrecisionsIterator
                ).render(**d)
            else:
                d["PRECISION_NUM"] = 0
                result += jinja2.Template(
                    self._Template_TouchCellFirstTime_Core
                ).render(**d)
            result += jinja2.Template(
                self._Template_TouchCellFirstTime_Epilogue
            ).render(**d)

            result = jinja2.Template(result).render(**d)
        return result

    def get_action_set_name(self):
        return (
            (__name__ + ("OnHangingCells" if self._on_hanging_cells else ""))
            .replace(".py", "")
            .replace(".", "_")
        )

    def get_includes(self):
        includes = (
            super(Prediction, self).get_includes()
            + """
#include "exahype2/CellData.h"
#include "kernels/"""
            + self._solver._name
            + """/CellData.h"
#include "kernels/"""
            + self._solver._name
            + """/FaceIntegral.h"
#include "kernels/"""
            + self._solver._name
            + """/RiemannSolver.h"
#include "kernels/"""
            + self._solver._name
            + """/FusedSpaceTimePredictorVolumeIntegral.h"
"""
        )
        if self._use_point_source:
            includes += (
                """
#include "kernels/"""
                + self._solver._name
                + """/PointSources.h"
#include "kernels/"""
                + self._solver._name
                + """/FusedSpaceTimePredictorVolumeIntegralWithoutPointSources.h"
"""
            )
        return includes
