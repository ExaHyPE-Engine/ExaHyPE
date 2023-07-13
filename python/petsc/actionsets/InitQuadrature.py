# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class InitQuadrature(ActionSet):
  """!

Initialise the quadrature

We should use a pre-existing python library + jinja for this.
But we can do it manually as proof of concept for now

Initialise an appropriately-arranged set of Gauss-Lobatto or
Gauss-Legendre points in each cell

  """
  
  TemplateInitCellQuadrature="""


  """

  
  def __init__(self,
               solver):
    """!
    
    
    """
    super( InitQuadrature, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()
    self.d["POLYNOMIAL_DEGREE"]      = solver.polynomial_degree


  def get_body_of_operation(self,operation_name):
    """!

    insert our templates into the c++ code.
    
    """
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitCellQuadrature).render(**self.d)
      pass 

    return result


  def get_action_set_name(self):
    """!
    
Configure name of generated C++ action set

This action set will end up in the directory observers with a name that
reflects how the observer (initialisation) is mapped onto this action 
set. The name pattern is ObserverName2ActionSetIdentifier where this
routine co-determines the ActionSetIdentifier. We make is reflect the
Python class name.
     
    """
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    """!
    
    The action set that Peano will generate that corresponds to this class
    should not be modified by users and can safely be overwritten every time
    we run the Python toolkit.
    
    """
    return False


  def get_attributes(self):
    """!
    this ends up in header file
    """
    return f"""
    const int polynomialDegree = {self.d["POLYNOMIAL_DEGREE"]};
"""
      
  def get_constructor_body(self):
    """!
    
Define body of constructor

Consult the superclass' description of the function for results. I 
basically initialise the _localMap with the correct tree number.

@see get_attributes()
    
    """
    return """
"""


  def get_includes(self):
    """!
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
"""
