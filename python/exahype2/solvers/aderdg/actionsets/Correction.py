# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet
from exahype2.solvers.PDETerms import PDETerms
# import peano4
import peano4.solversteps
import jinja2

from .kernels import (
    create_corrector_allocations, create_includes,
    riemann_solver, face_integral,
    create_solution_update
)


class Correction(AbstractAderDGActionSet):
    """

    The linear combination of the Runge Kutta trials has to be projected onto
    the faces, so we can then solve the Riemann problems. So the projection
    happens in one grid sweep, the corresponding Riemann solve in the next one.


    """

    Template_TouchFaceFirstTime = """

  if ({{PREDICATE}}) {
  
    const double timeStamp = coarseGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
  
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    constexpr int Order = {{ORDER}};
    constexpr int NumberOfVariables = {{NUMBER_OF_UNKNOWNS}};
    constexpr int NumberOfParameters = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    
    #if Dimensions==2
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif

    constexpr int basisElementsPerFace  = spaceFaceSize*(NumberOfVariables+NumberOfParameters);
    constexpr int fluxElementsPerFace   = spaceFaceSize*NumberOfVariables;

    /*
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */
    {{CORRECTOR_COMPUTATION_PRECISION}}* FL = fineGridFace{{SOLVER_NAME}}QFluxEstimates.value;
    {{CORRECTOR_COMPUTATION_PRECISION}}* FR = fineGridFace{{SOLVER_NAME}}QFluxEstimates.value+fluxElementsPerFace;
    {{CORRECTOR_COMPUTATION_PRECISION}}* QL = fineGridFace{{SOLVER_NAME}}QEstimates.value;
    {{CORRECTOR_COMPUTATION_PRECISION}}* QR = fineGridFace{{SOLVER_NAME}}QEstimates.value+basisElementsPerFace;
    const int direction = marker.getSelectedFaceNumber()%Dimensions;
    const int faceNumber  = marker.getSelectedFaceNumber();
    const bool isBoundary = fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary();
    tarch::la::Vector<Dimensions,double> faceCentre = marker.x();

    {{RIEMANN_SOLVER}}

  }  

"""

    Template_TouchCellFirstTime = """
  if ({{PREDICATE}}) {
  
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
  
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
        
    {{SOLUTION_STORAGE_PRECISION}}* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;

    {{CORRECTOR_ALLOCATIONS}}
    
    for(int d=0; d<Dimensions; d++){
    
      const int direction = d;

      //negative face
      if(!fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()){
        {{CORRECTOR_COMPUTATION_PRECISION}}* FIN = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsPerFace;
        const int orientation = 0;

        {{FACE_INTEGRAL}}

      }

      //positive face
      if(!fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).getIsHanging()){
        {{CORRECTOR_COMPUTATION_PRECISION}}* FIN = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value;
        const int orientation = 1;

        {{FACE_INTEGRAL}}

      }

    }// for d
    
    {{SOLUTION_UPDATE}}

    {{COMPUTE_NEW_TIME_STEP_SIZE}}
  
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      
    repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );
   
  }
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
        
        self.guard = guard
        self.riemann_guard  = guard

        self.use_custom_riemann_solver  = solver._riemann_solver_implementation!=PDETerms.None_Implementation
        self.use_kernelgenerator        = solver._use_kernel_generator
        self.is_linear                  = solver._is_linear

    def get_body_of_operation(self, operation_name):
        result = ""
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            d[ "PREDICATE" ]    = self.riemann_guard
            d["RIEMANN_SOLVER"] = jinja2.Template(riemann_solver(self.use_custom_riemann_solver, self.use_kernelgenerator, self.is_linear)).render(**d)
            result = jinja2.Template(self.Template_TouchFaceFirstTime).render(**d)
            pass
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            d["PREDICATE" ]             = self.guard
            d["CORRECTOR_ALLOCATIONS"]  = jinja2.Template(create_corrector_allocations(self.use_kernelgenerator)).render(**d)
            d["SOLUTION_UPDATE"]        = jinja2.Template(create_solution_update(self.use_kernelgenerator)).render(**d)
            d["FACE_INTEGRAL"]          = jinja2.Template(face_integral(self.use_kernelgenerator, self.is_linear)).render(**d)
            result = jinja2.Template(self.Template_TouchCellFirstTime).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        linearity = "linear" if self._solver._is_linear else "nonlinear"
        return (
            super(Correction, self).get_includes()
            + create_includes(self.use_kernelgenerator, self._solver._is_linear)
        )
