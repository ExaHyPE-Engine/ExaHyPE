# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractADERDGActionSet import AbstractADERDGActionSet
from exahype2.solvers.PDETerms import PDETerms

import jinja2
import peano4.solversteps


class Correction(AbstractADERDGActionSet):
    """
    The linear combination of the Runge-Kutta trials has to be projected onto
    the faces, so we can then solve the Riemann problems. So the projection
    happens in one grid sweep, the corresponding Riemann solve in the next one.
    """

    _Template_TouchFaceFirstTime = """
  if ({{PREDICATE}}) {
    const double timeStamp = coarseGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    // Needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}

    #if Dimensions == 2
    constexpr int SpaceFaceSize      = {{ORDER}} + 1;
    #else
    constexpr int SpaceFaceSize      = ({{ORDER}} + 1) * ({{ORDER}} + 1);
    #endif

    constexpr int FluxElementsPerFace   = SpaceFaceSize * {{NUMBER_OF_UNKNOWNS}};
    constexpr int BasisElementsPerFace  = SpaceFaceSize * ({{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}});

    /*
     * 2d: 0,1,2,3 -> 0,2,1,3
     * 3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
     */
    {{CORRECTOR_COMPUTATION_PRECISION}}* FL = fineGridFace{{SOLVER_NAME}}QFluxEstimates.value;
    {{CORRECTOR_COMPUTATION_PRECISION}}* FR = fineGridFace{{SOLVER_NAME}}QFluxEstimates.value + FluxElementsPerFace;
    {{CORRECTOR_COMPUTATION_PRECISION}}* QL = fineGridFace{{SOLVER_NAME}}QEstimates.value;
    {{CORRECTOR_COMPUTATION_PRECISION}}* QR = fineGridFace{{SOLVER_NAME}}QEstimates.value + BasisElementsPerFace;
    const int direction = marker.getSelectedFaceNumber() % Dimensions;
    const int faceNumber  = marker.getSelectedFaceNumber();
    const bool isBoundary = fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary();
    tarch::la::Vector<Dimensions,double> faceCentre = marker.x();

    {{RIEMANN_SOLVER}}
  }
"""

    _Template_TouchCellFirstTime = """
  if ({{PREDICATE}}) {
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    // Reuse the same timeStepSize from the predictor
    const double timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();

    {{SOLUTION_STORAGE_PRECISION}}* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;

    {{CORRECTOR_ALLOCATIONS}}

    ::exahype2::CellData<{{SOLUTION_STORAGE_PRECISION}}, {{CORRECTOR_COMPUTATION_PRECISION}}> cellData(
      fineGridCell{{UNKNOWN_IDENTIFIER}}.value,
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize,
      nullptr
    );

    for (int d = 0; d < Dimensions; d++) {
      const int direction = d;

      // Negative face
      if (!fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()) {

        // If above a hanging face, the riemann fluxes have been received from the other side and are therefore
        //   on that side.
        {{CORRECTOR_COMPUTATION_PRECISION}}* FIn = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value + 
          (fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getAboveHanging() ? 0 : fluxElementsPerFace);

        const int orientation = 0;

        {{FACE_INTEGRAL}}
      }

      // Positive face
      if (!fineGridFaces{{SOLVER_NAME}}FaceLabel(d + Dimensions).getIsHanging()) {

        // If above a hanging face, the riemann fluxes have been received from the other side and are therefore
        //   on that side.
        {{CORRECTOR_COMPUTATION_PRECISION}}* FIn = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d + Dimensions).value +
          (fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).getAboveHanging() ? fluxElementsPerFace : 0);

        const int orientation = 1;

        {{FACE_INTEGRAL}}
      }
    } // for d

    {{POSTPROCESS_UPDATED_PATCH}}

    {{COMPUTE_NEW_TIME_STEP_SIZE}}

    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp + timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);

    repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0));
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

    _Template_CorrectorAllocations = """
    const int fluxElementsPerFace = kernels::{{SOLVER_NAME}}::getBndFluxSize();
"""

    def __init__(self, solver, guard):
        """
        guard_project: String (C++ code)
          Predicate which controls if the solution is actually projected

        guard_safe_old_time_step: String (C++ code)
          Predicate which controls if the projection should be copied into
          the old solution and the time step should also be moved over
        """
        super(Correction, self).__init__(solver)

        self._guard = guard
        self._riemann_guard = guard

        self._use_custom_riemann_solver = (
            solver._riemann_solver_implementation != PDETerms.None_Implementation
        )
        self._is_linear = solver._is_linear

    def get_body_of_operation(self, operation_name):
        result = ""
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            d["PREDICATE"] = self.riemann_guard
            if self._use_custom_riemann_solver:
                d["RIEMANN_SOLVER"] = jinja2.Template(
                    self._Template_CustomRiemannSolver
                ).render(**d)
            else:
                d["RIEMANN_SOLVER"] = jinja2.Template(
                    self._Template_RiemannSolver
                ).render(**d)
            result = jinja2.Template(self._Template_TouchFaceFirstTime).render(**d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d["PREDICATE"] = self.guard
            d["CORRECTOR_ALLOCATIONS"] = jinja2.Template(
                self._Template_CorrectorAllocations
            ).render(**d)
            d["FACE_INTEGRAL"] = jinja2.Template(self._Template_FaceIntegral).render(
                **d
            )
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(Correction, self).get_includes()
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
            + """/MaxScaledEigenvalue.h"
"""
        )
