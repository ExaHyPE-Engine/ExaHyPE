# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau import ButcherTableau

import peano4
import jinja2

class SolveRiemannProblem(AbstractRungeKuttaDGActionSet):
  """
  
  The linear combination of the Runge Kutta trials has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  
  """
  
  TemplateProject = """

  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    const double timeStampOldSolution    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    
    // Set the variable
    // double timeStepSize
    {{COMPUTE_TIME_STEP_SIZE}}
    
    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;
    
    assertion3( tarch::la::greaterEquals( timeStepSize, 0.0 ),  timeStamp, timeStepSize, timeStampOldSolution );
    assertion3( tarch::la::greaterEquals( timeStamp, 0.0 ),     timeStamp, timeStepSize, timeStampOldSolution );
    
    #if Dimensions==2
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}Estimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_2D}} * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});

    #elif Dimensions==3
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}Estimates.value + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_3D}} * ({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #endif
    
    double* QHull[Dimensions*2];
    //bool    atBoundary[Dimensions*2];
    double maxEigenvaluePerFace[Dimensions*2];
    
    #if Dimensions==2
    const int facePosition = 0;
//    const int facePosition = 2*{{PREDICATE_NO}}*{{NUMBER_OF_DOFS_PER_FACE_2D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #elif Dimensions==3
    const int facePosition = 0;
//    const int facePosition = 2*{{PREDICATE_NO}}*{{NUMBER_OF_DOFS_PER_FACE_3D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}});
    #endif
    
    /*
    In Peano the faces are ordered by direction, then dimension, so in 3D it would be the face in negative direction in dimension 0, then 1, then 2
      followed by the the face in positive direction of dimensions 0, then 1, then 2. The kernels expect the faces to be ordered by dimension, then
      direction, so first left face in dimension0, then right face in dimension 0, and so on for the other dimensions.
      Therefore when the Hull is arranged, the order of these faces are shuffled correspondingly.    
    2d: 0,1,2,3 -> 0,2,1,3
    3d: 0,1,2,3,4,5 -> 0,3,1,4,2,5
    */
    
    for(int face=0; face<Dimensions; face++){
      QHull[2*face]        = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(face).value + facePosition;
      //atBoundary[2*face]   = fineGridFaces{{SOLVER_NAME}}FaceLabel(face).getBoundary() and
      //                       not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[face];
      QHull[2*face+1]      = fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates(face+Dimensions).value + facePosition;
      //atBoundary[2*face+1] = fineGridFaces{{SOLVER_NAME}}FaceLabel(face+Dimensions).getBoundary() and
      //                       not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[face+Dimensions];        
    }
    
    ::exahype2::dg::getMaxEigenvaluePerFace(
      [&](
        const double * __restrict__                 Q,
        const tarch::la::Vector<Dimensions,double>& x,
        double                                      t,
        double                                      dt,
        int                                         normal
        ) -> double {
          return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue(Q,x,t,dt,normal);
        },
      QHull,
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.x(),
      timeStamp,
      timeStepSize,
      maxEigenvaluePerFace
    );
    
    #if Dimensions==2
    double QHullForRiemann[2*Dimensions*{{NUMBER_OF_DOFS_PER_FACE_2D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})];
    #elif Dimensions==3
    double QHullForRiemann[2*Dimensions*{{NUMBER_OF_DOFS_PER_FACE_3D}}*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})];
    #endif      
    
    ::exahype2::dg::computeRusanovOnHull(
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,dt,normal,F);
        {% endif %}
      }, //flux
      [&](
          const double * __restrict__                  Q,
          const double * __restrict__                  dQdx,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__                        F
        ) -> void {
          {% if NCP_IMPLEMENTATION!="<none>" %}
          repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q, dQdx, x, t, dt, normal, F);
          {% endif %}          
      }, //ncp
      QHull,
      QHullForRiemann,
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.x(),
      marker.h()(0),
      timeStamp,
      timeStepSize,
      maxEigenvaluePerFace,
      {{ "true" if FLUX_IMPLEMENTATION!="<none>" else "false" }}, //useFlux
      {{ "true" if NCP_IMPLEMENTATION!="<none>" else "false" }} //useNCP
    );
                
    ::exahype2::dg::applyRiemannToPatch(
      QOut,
      QHullForRiemann,
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.h()(0),
      timeStepSize,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft,
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesRight,
      repositories::{{SOLVER_INSTANCE}}.QuadratureWeights
    );

    /*
    ::exahype2::dg::subtractCell(
      QOut,
      fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}}
    );
    */
  }
  {% endfor %}

"""

  def __init__(self,solver):
    """
    
    guard_project: String (C++ code)
      Predicate which controls if the solution is actually projected
      
    guard_safe_old_time_step: String (C++ code)
      Predicate which controls if the projection should be copied into
      the old solution and the time step should also be moved over
    
    """
    super(SolveRiemannProblem,self).__init__(solver)
    self.guards            = []
    self._butcher_tableau     = ButcherTableau(self._solver._rk_order)

  @property
  def guards(self):
    if self._guards==[]:
      raise Exception( "Guards are not initialised" )
    return self._guards
      
    
  @guards.setter
  def guards(self,new_guards):
    if new_guards!=[] and len(new_guards)!=self._solver.number_of_Runge_Kutta_steps():
      raise Exception( "Expect one guard per Runge Kutta step. Have {} steps but got guards {}".format(solver.number_of_Runge_Kutta_steps(),guards) )
    self._guards = new_guards
    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATES" ]            = self.guards
      d[ "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES" ] = self._butcher_tableau.time_step_sizes()
      result = jinja2.Template(self.TemplateProject).render(**d)
      pass 
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
  
  def get_includes(self):
    return super(SolveRiemannProblem,self).get_includes() + """
#include "exahype2/dg/Riemann.h"
    """  
