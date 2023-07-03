# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

#import peano4
import peano4.solversteps
import jinja2

class Correction_kernelgenerator(AbstractAderDGActionSet):
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
    
    const int Order = {{ORDER}};
    const int NumberOfVariables = {{NUMBER_OF_UNKNOWNS}};
    const int NumberOfParameters = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    
    #if Dimensions==2
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif

    const int basisElementsPerFace  = spaceFaceSize*(NumberOfVariables+NumberOfParameters);
    const int fluxElementsPerFace   = spaceFaceSize*NumberOfVariables;

    /*
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */

    {% if RIEMANN_SOLVER_IMPLEMENTATION!="<none>" %}
    repositories::{{SOLVER_INSTANCE}}.riemannSolver(
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.value,                        //FL
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.value+fluxElementsPerFace,    //FR
      fineGridFace{{SOLVER_NAME}}QEstimates.value,                          //QL
      fineGridFace{{SOLVER_NAME}}QEstimates.value+basisElementsPerFace,     //QR
      timeStamp,
      timeStepSize,
      marker.x(),
      marker.h(),
      marker.getSelectedFaceNumber()%Dimensions, //direction
      fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary(),
      marker.getSelectedFaceNumber()
    );
    {% else %}
    generated::kernels::AderDG::{{LINEARITY}}::riemannSolver<{{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.value,                        //FL
      fineGridFace{{SOLVER_NAME}}QFluxEstimates.value+fluxElementsPerFace,    //FR
      fineGridFace{{SOLVER_NAME}}QEstimates.value,                          //QL
      fineGridFace{{SOLVER_NAME}}QEstimates.value+basisElementsPerFace,     //QR
      timeStamp,
      timeStepSize,
      marker.x(),
      marker.h(),
      marker.getSelectedFaceNumber()%Dimensions //direction
    );
    {% endif %}

  }  

"""  
  
  Template_TouchCellFirstTime = """
  if ({{PREDICATE}}) {
  
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
  
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
        
    {{SOLUTION_STORAGE_PRECISION}}* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
    {{CORRECTOR_COMPUTATION_PRECISION}} lduh[generated::kernels::AderDG::{{LINEARITY}}::getUpdateSize()]{0.0};

    const int fluxElementsPerFace  = generated::kernels::AderDG::{{LINEARITY}}::getBndFluxSize();
    
    /*
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */
    
    for(int d=0; d<Dimensions; d++){
    
      const double inverseDxDirection = 1/marker.h()[d];
      
      //negative face
      if(!fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getIsHanging()){
        generated::kernels::AderDG::{{LINEARITY}}::faceIntegral<{{CORRECTOR_COMPUTATION_PRECISION}}>(
          lduh,
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsPerFace,
          d, //direction
          0, //orientation
          inverseDxDirection
        );
      }

      //positive face      
      if(!fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).getIsHanging()){
        generated::kernels::AderDG::{{LINEARITY}}::faceIntegral<{{CORRECTOR_COMPUTATION_PRECISION}}>(
          lduh,
          fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value,
          d, //direction
          1, //orientation
          inverseDxDirection
        );
      }

    }// for d    
    
    generated::kernels::AderDG::{{LINEARITY}}::solutionUpdate( 
      luh,
      luh, 
      lduh, 
      timeStepSize
    );

    {{COMPUTE_NEW_TIME_STEP_SIZE}}
  
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
      
    repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );
   
  }
"""
  
  def __init__(self,solver,guard):
    """
    
    guard_project: String (C++ code)
      Predicate which controls if the solution is actually projected
      
    guard_safe_old_time_step: String (C++ code)
      Predicate which controls if the projection should be copied into
      the old solution and the time step should also be moved over
    
    """
    super(Correction_kernelgenerator,self).__init__(solver)
    self.guard          = guard
    self.riemann_guard  = guard
    

  def get_body_of_operation(self,operation_name):
    result = ""
    d = {}
    self._solver._init_dictionary_with_default_parameters(d)
    self._solver.add_entries_to_text_replacement_dictionary(d)
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      d[ "PREDICATE" ] = self.riemann_guard
      result = jinja2.Template(self.Template_TouchFaceFirstTime).render(**d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d[ "PREDICATE" ] = self.guard
      result = jinja2.Template(self.Template_TouchCellFirstTime).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
    linearity = ( "linear" if self._solver._is_linear else "nonlinear" )
    return super(Correction_kernelgenerator,self).get_includes() + """
#include "../generated/kernels/aderdg/""" + linearity + """/Kernels.h"
      """