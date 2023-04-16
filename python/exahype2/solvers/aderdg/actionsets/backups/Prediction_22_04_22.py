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
  
    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
  
    // needs to declare and define timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    double* UIn = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
  
    #if Dimensions==2
    double spaceTimeQ[ ({{ORDER}}+1)*({{ORDER}}+1)*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})  ];
    #else
    double spaceTimeQ[ ({{ORDER}}+1)*({{ORDER}}+1)*({{ORDER}}+1)*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})  ];
    #endif
    
    int numberOfIterations = ::exahype2::aderdg::predictor::PicardLoop(
      UIn,
      spaceTimeQ,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.x(), //x
      timeStamp, //t
      marker.h()(0), //dx, assumes square/cubic cells
      timeStepSize, //dt
      0.001, //tolerance
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints,
      repositories::{{SOLVER_INSTANCE}}.QuadratureWeights,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,
      repositories::{{SOLVER_INSTANCE}}.StiffnessOperator,
      repositories::{{SOLVER_INSTANCE}}.InvertedPredictorLhsOperator,
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          int                                          normal,
          double * __restrict__                        F
          ) -> void {
          repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,normal,F);
        },
      nullptr, //NCP
      nullptr, //Source
      true,   //useFlux
      false,  //NCP
      false   //useSource
    );
      
      
    double* QHull[Dimensions*2];
    bool    atBoundary[Dimensions*2];
    
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
    
    ::exahype2::aderdg::riemann::projectCellDataOntoFaces(
      QHull,
      spaceTimeQ,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesRight,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}}
    );
    
    ::exahype2::aderdg::corrector::computeVolumeIntegral(
      UIn,
      spaceTimeQ,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{ORDER}},
      marker.x(), //x
      timeStamp, //t
      marker.h()(0), //dx, assumes cubic cells
      timeStepSize, //dt
      repositories::{{SOLVER_INSTANCE}}.QuadratureWeights,
      repositories::{{SOLVER_INSTANCE}}.StiffnessOperator,
      [&](
          const double * __restrict__                  Q,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          int                                          normal,
          double * __restrict__                        F
          ) -> void {
          repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,normal,F);
        },
      nullptr, //NCP
      nullptr, //Source
      true, //useFlux
      false, //useNCP
      false //useSource
      );
      
            
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
      """