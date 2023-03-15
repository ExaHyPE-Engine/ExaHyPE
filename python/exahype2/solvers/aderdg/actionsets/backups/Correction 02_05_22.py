# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

#import peano4
import peano4.solversteps
import jinja2

class Correction(AbstractAderDGActionSet):
  """
  
  The linear combination of the Runge Kutta trials has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  
  """
  
  TemplateProject = """
  if ({{PREDICATE}}) {
  
  int totalPosition = (marker.x()[1]/marker.h()[0]);
  totalPosition = 3*totalPosition;
  totalPosition += (marker.x()[0]/marker.h()[0]);

  std::cout << "TotalPosition: " << totalPosition << std::endl;

  for(int d=0; d<Dimensions; d++){
    std::cout << "Face " << d << ":" << std::endl;
    for(int i=0; i<2*({{ORDER}}+1); i++){
    std::cout << "node " << i << ":  [";
      for(int j=0; j<3; j++){
        std::cout << fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value[i*3+j] << ", ";        
      }
      std::cout << "]" << std::endl;
    }
  
    std::cout << "Face " << d+Dimensions << ":" << std::endl;
    for(int i=0; i<2*({{ORDER}}+1); i++){
    std::cout << "node " << i << ": [";
      for(int j=0; j<3; j++){
        std::cout << fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value[i*3+j] << ", ";        
      }
      std::cout << "]" << std::endl;
    }
  
  }
  
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
  
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStamp, timeStepSize );
    assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStamp, timeStepSize );
              
    const int Order = {{ORDER}};
    const int NumberOfVariables = {{NUMBER_OF_UNKNOWNS}};
    const int NumberOfParameters = {{NUMBER_OF_AUXILIARY_VARIABLES}};
    
    #if Dimensions==2
    constexpr int spaceBasisSize     = (Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1);
    #else
    constexpr int spaceBasisSize     = (Order+1)*(Order+1)*(Order+1);
    constexpr int spaceFaceSize      = (Order+1)*(Order+1);
    #endif
    
    double* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
    double lduh[spaceBasisSize*NumberOfVariables]{0.0};

    const int basisElementsByFace = spaceFaceSize*(NumberOfVariables+NumberOfParameters);
    const int fluxElementsByFace = spaceFaceSize*NumberOfVariables;
//    const int gradElementsByFace = spaceFaceSize*(NumberOfVariables+NumberOfParameters);

    int nodesPerFace = {{ORDER}}+1;
    int strideQ = {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}};
    double* face = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(0).value;
    
    

    /*
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */
    
    for(int d=0; d<Dimensions; d++){

      //negative face
      double* QIn   = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsByFace;
      double* QOut  = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value;
      double* FIn   = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsByFace;
      double* FOut  = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value;
      double* gradQ = fineGridFaces{{UNKNOWN_IDENTIFIER}}GradEstimates(d).value;
      
      //if face is at boundary
      bool atBoundary = fineGridFaces{{SOLVER_NAME}}FaceLabel(d).getBoundary() and
           not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[d];
      if(true){
        repositories::{{SOLVER_INSTANCE}}.boundaryConditions(
          FOut,
          QOut,
          FIn,
          QIn,
          gradQ,
          marker.x(),
          marker.h(),
          timeStamp,
          timeStepSize,
          2*d, //faceIndex
          d  //normalNonZero
        );
      }
      
      //ALL SAME UNTIL AT LEAST NOW
      
      repositories::{{SOLVER_INSTANCE}}.riemannSolverNonlinear(
        FOut,
        FIn,
        QOut,
        QIn,
        timeStamp  ,
        timeStepSize,
        marker.h(),
        d //direction
      );
      
      /*
      std::fill(lduh, lduh+spaceBasisSize*NumberOfVariables, 0.0);
            
      repositories::{{SOLVER_INSTANCE}}.faceIntegralNonlinear(
        lduh,
        FOut,
        d, //direction
        0, //orientation
        marker.h()
      );
      
      
      std::cout << "lduh negative in direction " << d << std::endl;
      std::cout << "node 10: " <<lduh [10*NumberOfVariables+0] << std::endl;      
      std::cout << "node 2 : " <<lduh [2*NumberOfVariables+0] << std::endl << std::endl;
      */
      /*
      for(int i=0; i<spaceBasisSize; i++){
        std::cout << "node " << i << ":[";
        for(int j=0; j<NumberOfVariables; j++){
          std::cout << lduh[i*NumberOfVariables+j] << ", ";
        }
        std::cout << "]" << std::endl;      
      }
      */

      
      //positive face
      QIn   = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value;
      QOut  = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value+basisElementsByFace;
      FIn   = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value;
      FOut  = fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value+fluxElementsByFace;
      gradQ = fineGridFaces{{UNKNOWN_IDENTIFIER}}GradEstimates(d+Dimensions).value;  
          
      atBoundary = fineGridFaces{{SOLVER_NAME}}FaceLabel(d+Dimensions).getBoundary() and
           not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[d+Dimensions];
          
      //if face is at boundary 
      if(true){
        repositories::{{SOLVER_INSTANCE}}.boundaryConditions(
          FOut,
          QOut,
          FIn,
          QIn,
          nullptr, //gradQ
          marker.x(),
          marker.h(),
          timeStamp,
          timeStepSize,
          2*d+1, //faceIndex
          d  //normalNonZero
        );
        
      }
      
      //ALL SAME UNTIL NOW
      
      repositories::{{SOLVER_INSTANCE}}.riemannSolverNonlinear(
        FIn,
        FOut,
        QIn,
        QOut,
        timeStamp  ,
        timeStepSize,
        marker.h(),
        d //direction
      );
      
      std::cout << "Dimension " << d << ", Positive face: " << std::endl;
      
      /*
      std::cout << "FIn:" << std::endl;
      for(int i=0; i<nodesPerFace; i++){
        std::cout << "node " << i << ": [";
        for(int j=0; j<NumberOfVariables; j++){
          std::cout << FIn[i*NumberOfVariables+j] << ", ";
        }
        std::cout << "]" << std::endl;
      }
      */
      /*
      std::fill(lduh, lduh+spaceBasisSize*NumberOfVariables, 0.0);
      
      repositories::{{SOLVER_INSTANCE}}.faceIntegralNonlinear(
        lduh,
        FIn,
        d,
        1,
        marker.h()
      );

      std::cout << "lduh positive in direction " << d << std::endl;
      std::cout << "node 10: " <<lduh [10*NumberOfVariables+0] << std::endl;      
      std::cout << "node 2 : " <<lduh [2*NumberOfVariables+0] << std::endl << std::endl;
      */
            
      /*
      std::cout << "lduh negative in direction " << d << std::endl;
      for(int i=0; i<spaceBasisSize; i++){
        std::cout << "node " << i << ":[";
        for(int j=0; j<NumberOfVariables; j++){
          std::cout << lduh[i*NumberOfVariables+j] << ", ";
        }
        std::cout << "]" << std::endl;      
      }
      */
      
    }

    repositories::{{SOLVER_INSTANCE}}.solutionUpdate(
      luh,
      luh,
      lduh,
      timeStepSize
    );
  
  /*
  std::cout << "Correction: Total position: " << totalPosition << std::endl;       
  std::cout << "lduh, h: " << std::endl;
  std::cout << "Node 10 [" << lduh[10*strideQ+0] << "]" << std::endl;
  std::cout << "Node 11 [" << lduh[11*strideQ+0] << "]" << std::endl;
  std::cout << "Node 12 [" << lduh[12*strideQ+0] << "]" << std::endl;
  std::cout << "Node 13 [" << lduh[13*strideQ+0] << "]" << std::endl;
  std::cout << "Node 14 [" << lduh[14*strideQ+0] << "]" << std::endl;
  std::cout << std::endl;
  std::cout << "Node 2 [" << lduh[2*strideQ+0] << "]" << std::endl;
  std::cout << "Node 7 [" << lduh[7*strideQ+0] << "]" << std::endl;
  std::cout << "Node 12 [" << lduh[12*strideQ+0] << "]" << std::endl;
  std::cout << "Node 17 [" << lduh[17*strideQ+0] << "]" << std::endl;
  std::cout << "Node 22 [" << lduh[22*strideQ+0] << "]" << std::endl;
  std::cout << std::endl;

  std::cout << "lduh, px: " << std::endl;
  std::cout << "Node 10 [" << lduh[10*strideQ+1] << "]" << std::endl;
  std::cout << "Node 11 [" << lduh[11*strideQ+1] << "]" << std::endl;
  std::cout << "Node 12 [" << lduh[12*strideQ+1] << "]" << std::endl;
  std::cout << "Node 13 [" << lduh[13*strideQ+1] << "]" << std::endl;
  std::cout << "Node 14 [" << lduh[14*strideQ+1] << "]" << std::endl;
  std::cout << std::endl;

  std::cout << "lduh, py: " << std::endl;
  std::cout << "Node 2 [" << lduh[2*strideQ+2] << "]" << std::endl;
  std::cout << "Node 7 [" << lduh[7*strideQ+2] << "]" << std::endl;
  std::cout << "Node 12 [" << lduh[12*strideQ+2] << "]" << std::endl;
  std::cout << "Node 17 [" << lduh[17*strideQ+2] << "]" << std::endl;
  std::cout << "Node 22 [" << lduh[22*strideQ+2] << "]" << std::endl;
  std::cout << std::endl;    
  */
  
    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStamp, timeStepSize );
    assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStamp, timeStepSize );
    
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
    super(Correction,self).__init__(solver)
    self.guard            = guard
    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]                 = self.guard
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def get_includes(self):
    return super(Correction,self).get_includes() + """
#include "exahype2/new_ader/Riemann.h"
#include "exahype2/new_ader/Corrector.h"
      """