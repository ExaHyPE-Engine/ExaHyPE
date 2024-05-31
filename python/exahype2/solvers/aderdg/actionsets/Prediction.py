# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet
from exahype2.solvers.PDETerms import PDETerms
from .kernels import (
  create_predictor_allocations, create_predictor_frees,
  create_fstpvi_call,
  create_includes, create_solution_update,
  copy_estimates_and_perform_riemann_solution_on_hanging_faces,
  copy_estimates_into_faces, number_of_precisions_iterator
)

# import peano4
import peano4.solversteps
import jinja2


class Prediction(AbstractAderDGActionSet):
    """

    The extrapolated solution from the space-time predictor has to be projected onto
    the faces, so we can then solve the Riemann problems. So the projection
    happens in one grid sweep, the corresponding Riemann solve in the next one.

    """

    _Template_TouchCellFirstTime_Preamble = """

  bool isHangingCell = false;
  for(int d=0; d<2*Dimensions; d++){
    if(fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()){
      isHangingCell = true;
      break;
    }
  }

  if ({{PREDICATE}}) {

    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}  
  
    {{SOLUTION_STORAGE_PRECISION}}* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;

    const int Order = {{ORDER}};
    const int NumberOfVariables  = {{NUMBER_OF_UNKNOWNS}};
    const int NumberOfParameters = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    
    #if Dimensions==2
    constexpr int spaceBasisSize     = (Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceBasisSize     = (Order+1)*(Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif

    constexpr int spaceTimeBasisSize = spaceBasisSize*(Order+1);

    {{CREATE_PREDICTOR_ALLOCATIONS}}

    {% if STORE_OLD_SOLUTION %}
    std::copy_n(luh ,sizeLduh, fineGridCell{{UNKNOWN_IDENTIFIER}}_old.value);
    {% endif %}
"""

    _Template_TouchCellFirstTime_Core = """
    {{FUSED_PREDICTOR_VOLUME_INTEGRAL}}
    {{PROJECT_SOLUTION_TO_FACES}}
    {{SOLUTION_UPDATE}}
"""

    _Template_TouchCellFirstTime_Epilogue = """
    repositories::{{SOLVER_INSTANCE}}.update(timeStepSize, timeStamp, marker.h()(0));
    {{CREATE_PREDICTOR_FREES}}
  }
"""

    def __init__(self, solver, guard, on_hanging_cells=False):
        """
        guard: String (C++ code)
          Predicate which controls whether this actionset should run
        on_hanging_cells: bool
          Determines whether this is running on hanging cells or not,
          the actions on both differ
        """
        super(Prediction, self).__init__(solver)
        self.guard = guard
        self.onHangingCells = on_hanging_cells
        self.use_kernelgenerator        = solver._use_kernel_generator
        self.is_linear                  = solver._is_linear
        self.use_custom_riemann_solver  = solver._riemann_solver_implementation!=PDETerms.None_Implementation
        self.usePointSource             = solver._point_sources_implementation!=PDETerms.None_Implementation
        self.multiple_precisions        = len(solver._predictor_computation_precisions) > 1
        self.store_old_solution         = solver._hold_previous_time_step

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATE"]                = self.guard
            d["STORE_OLD_SOLUTION"]       = self.store_old_solution
            d["PRECISIONS_CAPITALIZED"]   = [
                x.replace(" ", "_").replace("std::", "").capitalize()
                for x in d["PREDICTOR_COMPUTATION_PRECISIONS"]
            ]
            d["CREATE_PREDICTOR_ALLOCATIONS"]           = create_predictor_allocations(self.use_kernelgenerator)
            d["CREATE_PREDICTOR_FREES"]                 = create_predictor_frees(self.use_kernelgenerator)
            d["FUSED_PREDICTOR_VOLUME_INTEGRAL"]        = create_fstpvi_call(self.is_linear, self.usePointSource, self.use_kernelgenerator)
            d["SOLUTION_UPDATE"]                        = create_solution_update(self.use_kernelgenerator)
            d["PROJECT_SOLUTION_TO_FACES"]              = (
                copy_estimates_and_perform_riemann_solution_on_hanging_faces(self.use_custom_riemann_solver, self.use_kernelgenerator, self.is_linear) if self.onHangingCells
                else copy_estimates_into_faces()
            )

            result = jinja2.Template(self._Template_TouchCellFirstTime_Preamble).render(**d)
            if self.multiple_precisions:
                result += jinja2.Template(number_of_precisions_iterator(self._Template_TouchCellFirstTime_Core)).render(**d)
            else:
                d["PRECISION_NUM"] = 0
                result += jinja2.Template(self._Template_TouchCellFirstTime_Core).render(**d)
            result += jinja2.Template(self._Template_TouchCellFirstTime_Epilogue).render(**d)

            result = jinja2.Template(result).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return (__name__ + ("_on_hanging_cells" if self.onHangingCells else "")).replace(".py", "").replace(".", "_")

    def get_includes(self):
        linearity = "linear" if self._solver._is_linear else "nonlinear"
        return (
            super(Prediction, self).get_includes()
            + create_includes(self.use_kernelgenerator, self._solver._is_linear)
        )