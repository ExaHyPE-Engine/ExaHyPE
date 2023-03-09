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
  
    std::cout << "Entered correction" << std::endl;
      
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
  
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStamp, timeStepSize );
    assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStamp, timeStepSize );
    
    double* UIn = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
          
    double* QHull[Dimensions*2];
    bool    atBoundary[Dimensions*2];
    double  maxEigenvaluePerFace[Dimensions*2];
    
    /*
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */

    for(int face=0; face<Dimensions; face++){
      QHull[2*face] = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(face).value;
      atBoundary[2*face]  = fineGridFaces{{SOLVER_NAME}}FaceLabel(face).getBoundary() and
                           not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[face];
      QHull[2*face+1] = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(face+Dimensions).value;
      atBoundary[2*face+1]  = fineGridFaces{{SOLVER_NAME}}FaceLabel(face+Dimensions).getBoundary() and
                           not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[face];        
    }
    
    ::exahype2::aderdg::riemann::getMaxEigenvaluePerFace(
      [&](
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        int                                          normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q,x,t,normal);
      },
      [&](
        const double * __restrict__                  Qinside,
        double * __restrict__                         Qoutside,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.boundaryConditions(Qinside, Qoutside, x, t, normal);
      },
      QHull,
      atBoundary,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.x(),
      timeStamp, //t
      maxEigenvaluePerFace
    );
    
    #if Dimensions==2
    double QHullForRiemann[2*Dimensions*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})]{0.0};
    #else
    double QHullForRiemann[2*Dimensions*({{ORDER}}+1)*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})]{0.0};
    #endif
    
    ::exahype2::aderdg::riemann::computeRusanovOnHull(
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,normal,F);
        },
      nullptr,
      [&](
        const double * __restrict__                  Qinside,
        double * __restrict__                         Qoutside,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal
        ) -> void {
          repositories::{{SOLVER_INSTANCE}}.boundaryConditions(Qinside, Qoutside, x, t, normal);
      },
      QHull,
      QHullForRiemann,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.x(),
      marker.h()(0), //dx, assumes cubic cell
      timeStamp,
      timeStepSize,
      maxEigenvaluePerFace,
      atBoundary,
      repositories::{{SOLVER_INSTANCE}}.QuadratureWeights,
      true, //useSource
      false //useNCP
    );    

    int nodesPerAxis = {{ORDER}}+1;
    int nodesPerFace = nodesPerAxis*nodesPerAxis;
    int strideQ = {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}};


    /*
    const double timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    {{COMPUTE_TIME_STEP_SIZE}}
     */
     
    {{PREPROCESS_RECONSTRUCTED_PATCH}}

    assertion2( tarch::la::greaterEquals( timeStamp, 0.0 ),    timeStamp, timeStepSize );
    assertion2( tarch::la::greaterEquals( timeStepSize, 0.0 ), timeStamp, timeStepSize );
    
    {{COMPUTE_NEW_TIME_STEP_SIZE}}
  
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
  
    repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );
    
    std::cout << "Left correction" << std::endl;
  
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