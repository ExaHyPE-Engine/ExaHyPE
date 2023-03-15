# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

#import peano4
import peano4.solversteps
import jinja2

class Prediction(AbstractAderDGActionSet):
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
    std::cout << "Prediction: Total position: " << totalPosition << std::endl;
  
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
  
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    double* luh = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
  
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
    
    constexpr int spaceTimeBasisSize = spaceBasisSize*(Order+1);
    constexpr int sizeLQi   = (NumberOfVariables+NumberOfParameters)*spaceTimeBasisSize;
    constexpr int sizeRhs   = sizeLQi;
    constexpr int sizeLFi   = (Dimensions + 1)*NumberOfVariables*spaceTimeBasisSize;
    constexpr int sizeGradQ = Dimensions*NumberOfVariables*spaceBasisSize;
    
    constexpr int sizeLQhi = (NumberOfVariables+NumberOfParameters)*spaceBasisSize;
    constexpr int sizeLFhi = (Dimensions + 1)*NumberOfVariables*spaceBasisSize;
    
    constexpr int totalSize = sizeLQi + sizeRhs + sizeLFi + sizeGradQ +
                          sizeLQhi + sizeLFhi + sizeLQhi;
  
    double block[totalSize];
    double* memory = block;
        
    double* lQi   = memory; memory+=sizeLQi;
    double* rhs   = memory; memory+=sizeRhs;
    double* lFi   = memory; memory+=sizeLFi;
    double* gradQ = memory; memory+=sizeGradQ;
    
    double* lQhi = memory; memory+=sizeLQhi;
    double* lFhi = memory; memory+=sizeLFhi;
    double* lduh = memory; memory+=sizeLQhi;
    
    double lQhbnd[2*Dimensions*spaceFaceSize*(NumberOfVariables+NumberOfParameters)]{0.0};
    double lFhbnd[2*Dimensions*spaceFaceSize*NumberOfVariables]{0.0};
    double lGradQhbnd[2*Dimensions*Dimensions*spaceFaceSize*(NumberOfVariables+NumberOfParameters)]{0.0};

    const int strideQ = 3;
    for(int i=0; i<spaceBasisSize; i++){
      luh[i*strideQ+0] = 1.0;
    }
  
    luh[strideQ*12+0] = 1.5;
    

    int numberOfIterations = repositories::{{SOLVER_INSTANCE}}.spaceTimePredictorNonlinear(
      lQhbnd,
      lGradQhbnd,
      lFhbnd,
      lQi,
      rhs,
      lFi,
      gradQ,
      lQhi,
      lFhi,
      luh,
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize
    );
    
    repositories::{{SOLVER_INSTANCE}}.volumeIntegralNonlinear(
      lduh,
      lFhi,
      marker.h()    
    );
    
    repositories::{{SOLVER_INSTANCE}}.solutionUpdate(
      luh,
      luh,
      lduh,
      timeStepSize
    );
    
    const int basisElementsByFace    = spaceFaceSize*(NumberOfVariables+NumberOfParameters);
    const int fluxElementsByFace     = spaceFaceSize*NumberOfVariables;
    //const int gradElementsByFace   = Dimensions*spaceFaceSize*(NumberOfVariables+NumberOfParameters);
    
    /*
    Copy the estimates into the face data containers. Note that peano uses a different ordering of
    the faces that the kernels, so these are reordered somewhat.
    The kernels order by dimension, then side of the face whereas peano orders by
    side, then dimension.
    E.g. in 2d the kernels have the order left, right, lower face, upper face
    whereas peano orders: left, lower, right, upper 
    
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */
    
    for(int i=0; i<2*Dimensions*spaceFaceSize; i++){
      for(int j=0; j<NumberOfVariables+NumberOfParameters; j++){
        lQhbnd[i*(NumberOfVariables+NumberOfParameters)+j] = totalPosition;
      }
    }
    
    for(int d=0; d<Dimensions; d++){
      double leftFace = totalPosition+0.1*d;
      double rightFace = totalPosition+0.1*(d+Dimensions);

      //predicted solution projection, negative then positive face
      /*
      std::fill(fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsByFace,fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+2*basisElementsByFace,leftFace);
      std::fill(fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value+basisElementsByFace,rightFace);
      */
      std::copy_n(&lQhbnd[2*d*basisElementsByFace], basisElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d).value+basisElementsByFace);
      std::copy_n(&lQhbnd[(2*d+1)*basisElementsByFace], basisElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(d+Dimensions).value);


      //fluxes
      std::fill(fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+2*fluxElementsByFace,leftFace);
      std::fill(fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value+fluxElementsByFace,rightFace);      
      /*
      std::copy_n(&lFhbnd[2*d*fluxElementsByFace], fluxElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d).value+fluxElementsByFace);    
      std::copy_n(&lFhbnd[(2*d+1)*fluxElementsByFace], fluxElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates(d+Dimensions).value);
      */

      //gradients
      /*
      std::copy_n(&lGradQhbnd[2*d*gradElementsByFace], gradElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}GradEstimates(d).value+gradElementsByFace);
      std::copy_n(&lGradQhbnd[(2*d+1)*gradElementsByFace], gradElementsByFace, fineGridFaces{{UNKNOWN_IDENTIFIER}}GradEstimates(d+Dimensions).value);      
      */
    }

    /*
    std::cout << "position: [" << marker.x()[0] << ", " << marker.x()[1] << "]" << std::endl;
    int totalPosition = (marker.x()[1]/marker.h()[0]);
    totalPosition = 3*totalPosition;
    totalPosition += (marker.x()[0]/marker.h()[0]);
    std::cout << "Total position: " << totalPosition << std::endl;
    std::cout << "luh:" << std::endl;
    for(int i=0; i<4*4; i++){
      std::cout << "node " << i << "[";
      for(int j=0; j<3; j++){
        std::cout << luh[i*3+j] << ", ";
      }
      std::cout << "]" << std::endl;
    }
    */
    /*
    std::cout << "Qhbnd:" << std::endl;
    for(int i=0; i<4*4; i++){
      std::cout << "node " << i << "[";
      for(int j=0; j<3; j++){
        std::cout << lQhbnd[i*3+j] << ", ";
      }
      std::cout << "]" << std::endl;
    }
    */
  
  
  /*  
  std::cout << "lQhbnd:" << std::endl;
  for(int i=0; i<4*5; i++){
    std::cout << "node " << i << "[";
    for(int j=0; j<3; j++){
      std::cout << lQhbnd[i*3+j] << ", ";
    }
    std::cout << "]" << std::endl;
  }
  */


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


  /*
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
  /*
  if(timeStamp>=timeStepSize) exit(1);
  */    
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
    super(Prediction,self).__init__(solver)
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
    return super(Prediction,self).get_includes() + """
#include "exahype2/new_ader/Predictor.h"
#include "exahype2/new_ader/Riemann.h"
#include "exahype2/new_ader/Corrector.h"
#include "exahype2/aderdg_kernels/Kernels.h"
#include <typeinfo>
      """