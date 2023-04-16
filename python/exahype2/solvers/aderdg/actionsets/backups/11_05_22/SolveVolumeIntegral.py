# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

from .AbstractAderDGActionSet import AbstractAderDGActionSet

#import peano4
import peano4.solversteps
import jinja2

class SolveVolumeIntegral(AbstractAderDGActionSet):
  """
  
  The linear combination of the Runge Kutta trials has to be projected onto
  the faces, so we can then solve the Riemann problems. So the projection 
  happens in one grid sweep, the corresponding Riemann solve in the next one.
  
  
  """
  
  TemplateProject = """
  if ({{PREDICATE}}) {
  /*  
    ::exahype2::dg::explicitEulerOnCell(
      fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.value,   // const double* __restrict__          cellQin,
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      [&](
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      )->void {
        {% if FLUX_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.flux(Q,x,t,normal,F);
        {% else %}
        #if Dimensions==2
        std::fill_n(F,{{NUMBER_OF_UNKNOWNS}}*{{NUMBER_OF_DOFS_PER_CELL_2D}},0.0);
        #else
        std::fill_n(F,{{NUMBER_OF_UNKNOWNS}}*{{NUMBER_OF_DOFS_PER_CELL_3D}},0.0);
        #endif
        {% endif %}
      },
      [&] (
        const double * __restrict__                  Q,
        const double * __restrict__                  dQdx,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        int                                          normal,
        double * __restrict__                        F
      ) ->  void {
        {% if NCP_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.nonconservativeProduct(Q,dQdx,x,t,normal,F);
        {% else %}
        #if Dimensions==2
        std::fill_n(F,{{NUMBER_OF_UNKNOWNS}}*{{NUMBER_OF_DOFS_PER_CELL_2D}},0.0);
        #else
        std::fill_n(F,{{NUMBER_OF_UNKNOWNS}}*{{NUMBER_OF_DOFS_PER_CELL_3D}},0.0);
        #endif
        {% endif %}
      },
      [&] (
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double * __restrict__                        S
      ) -> void {
        {% if SOURCE_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.sourceTerm(Q,x,t,S);
        {% else %}
        #if Dimensions==2
        std::fill_n(S,{{NUMBER_OF_UNKNOWNS}}*{{NUMBER_OF_DOFS_PER_CELL_2D}},0.0);
        #else
        std::fill_n(S,{{NUMBER_OF_UNKNOWNS}}*{{NUMBER_OF_DOFS_PER_CELL_3D}},0.0);
        #endif
        {% endif %}
      },
      [&] (
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        const tarch::la::Vector<Dimensions,double>&  h,
        const tarch::la::Vector<Dimensions,int>&     index,
        double                                       t,
        double * __restrict__                        S
      ) -> void {
        {% if SOURCE_IMPLEMENTATION!="<none>" %}
        repositories::{{SOLVER_INSTANCE}}.pointSourceTerm(Q,x,h,index,t,S);
        {% else %}
        #if Dimensions==2
        std::fill_n(S,{{NUMBER_OF_UNKNOWNS}}*{{NUMBER_OF_DOFS_PER_CELL_2D}},0.0);
        #else
        std::fill_n(S,{{NUMBER_OF_UNKNOWNS}}*{{NUMBER_OF_DOFS_PER_CELL_3D}},0.0);
        #endif
        {% endif %}
      },
      repositories::{{SOLVER_INSTANCE}}.DerivativeOperator,
      repositories::{{SOLVER_INSTANCE}}.MassMatrix,
      repositories::{{SOLVER_INSTANCE}}.StiffnessMatrix,
      repositories::{{SOLVER_INSTANCE}}.InvertedAndLumpedMassMatrix,
      todo
    );    
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
    super(SolveVolumeIntegral,self).__init__(solver)
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
