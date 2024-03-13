from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


def order_DG_CG_solvers( dg_solver,
                        cg_solver,
                        ):
    dg_solver._basic_descend_order = cg_solver._basic_descend_order + 1


class ComputeAndRestrictBiasedHierarchicalResidual(ActionSet):
  """!
  
  Compute the hierarchical, biased residual over a DG solution and restrict it onto a collocated grid

  If you want to use this action set, add it as a preamble to the DG solver via
  
         solver.preprocessing_action_set = mghype.matrixfree.solvers.api.actionsets.ComputeAndRestrictBiasedHierarchicalResidual(
  
  After that, ensure that you add the CG solver ***before*** you add your DG solver.
  This is important, as the order of the action sets that results is important.
  
  Internally, the action set realises a very simple scheme:
  
  - touchVertexFirstTime():
    1. Set the CG solver's right-hand side to zero.
    2. Set the CG solver's delta to zero.
    3. Take the DG value at the corner, scale it with @f$ 2^-d @f$ and add this
       value to the CG vertex's delta.
  - touchCellFirstTime():
    1. Take the CG vertex's value, interpolate it and compute the hierarchical
       residual subject to A_cc.
    2. Restrict this residual and add the restricted value to the CG solver's
       right-hand side.
  - touchVertexLastTime():
    1. Take delta from the CG solver's vertex, prolong this value and add it 
       to the DG cell solution.
    2. Copy the 
  


  
  
  """


  templateTouchVertexFirstTime="""
  if ( fineGridVertex{{CG_SOLVER_NAME}}.getType() == vertexdata::{{CG_SOLVER_NAME}}::Type::Interior )
  {
  }
"""


  templateTouchCellFirstTime="""
  if ( 
    fineGridCell{{CG_SOLVER_NAME}}.getType() == celldata::{{CG_SOLVER_NAME}}::Type::Interior  
    and
    not marker.hasBeenRefined()
  )
  {
  }
"""


  templateTouchVertexLastTime="""
  if ( fineGridVertex{{CG_SOLVER_NAME}}.getType() == vertexdata::{{CG_SOLVER_NAME}}::Type::Interior )
  {
  }
"""


  def __init__(self,
               dg_solver,
               cg_solver,
               prolongation_matrix,
               prolongation_matrix_scalin,
               restriction_matrix,
               restriction_matrix_scaling,
               ):
    """!

    Construct action set

    @param prolongation_matrix: [ @f$ \mathbb{R}^{ (p+1}^dK  \times 2^dK } @f$ ]
      Counterpart of restriction_matrix. Often the transpose.
      
    @param prolongation_matrix_scaling: [ Integer ]
      See restriction_matrix_scaling.
    
    @param restriction_matrix: [ @f$ \mathbb{R}^{2^dK \times (p+1}^dK } @f$ ]
      Sequence of matrices. Each takes the solution over the cell spanned by 
      @f$ (p+1}^dK @f$ weights and restricts it onto @f$ K @f$ quantities on the 
      vertices. You can specify the total restriction through a 
      sequence of matrices, each one with its own @f$ h^s @f$ scaling.
      
    @param restriction_matrix_scaling: [ Integer ]
      Specifies a sequence of values @f$ s @f$. They define with which 
      @f$ h^s @f$ the corresponding matrices in restriction_matrix are to be 
      scaled. That is, if you pass in a [3,0], then the first matrix in 
      restriction_matrix will be scaled with  @f$ h^3 @f$ and the second one 
      with one.
    
    
    
    """  
    super( ComputeAndRestrictBiasedHierarchicalResidual, self ).__init__(
      0,     #descend_invocation_order,
      False  # parallel
    )
    self.d = {}
    self.d["DG_SOLVER_INSTANCE"]    = dg_solver.instance_name()
    self.d["DG_SOLVER_NAME"]        = dg_solver.typename()
    self.d["CG_SOLVER_INSTANCE"]    = cg_solver.instance_name()
    self.d["CG_SOLVER_NAME"]        = cg_solver.typename()

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.templateTouchVertexFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result = jinja2.Template(self.templateTouchVertexLastTime).render(**self.d)
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
    return __name__.replace(".py", "").replace(".", "_") + "_ComputeAndRestrictBiasedHierarchicalResidual"
    
  def user_should_modify_template(self):
    """!
    
    The action set that Peano will generate that corresponds to this class
    should not be modified by users and can safely be overwritten every time
    we run the Python toolkit.
    
    """
    return False

  def get_includes(self):
    """!
   
    We need the solver repository in this action set, as we directly access
    the solver object. We also need access to Peano's d-dimensional loops.
         
    """    
    return """
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""





class InjectAndProlong(ActionSet):
  """!

  
  """


  templateTouchVertexFirstTime="""
  if ( fineGridVertex{{CG_SOLVER_NAME}}.getType() == vertexdata::{{CG_SOLVER_NAME}}::Type::Interior )
  {
  }
"""


  templateTouchVertexLastTime="""
  if ( fineGridVertex{{CG_SOLVER_NAME}}.getType() == vertexdata::{{CG_SOLVER_NAME}}::Type::Interior )
  {
  }
"""


  def __init__(self,
               dg_solver,
               cg_solver,
               prolongation_matrix,
               prolongation_matrix_scaling,
               injection_matrix,
               injection_matrix_scaling
               ):
    """!
    
    Construct action set
    
    @param prolongation_matrix Please consult the action set ComputeAndRestrictBiasedHierarchicalResidual.

    @param prolongation_matrix_scaling Please consult the action set ComputeAndRestrictBiasedHierarchicalResidual.
    
    @param injection_matrix: [ @f$ \mathbb{R}^{2^dK \times (p+1}^dK } @f$ ]
      Sequence of matrices. Each takes the solution over the cell spanned by 
      @f$ (p+1}^dK @f$ weights and maps them onto @f$ K @f$ quantities on the 
      vertices. These quantities are typically just the value of the polynomial
      in the vertex, but you can pick other schemes, too. Most of the time, it
      will be a single injection (with one scaling, see below). To be in line
      with the other signatures, you can however specify the total injection through a 
      sequence of matrices, each one with its own @f$ h^s @f$ scaling.
       
    @param injection_matrix_scaling: [ Integer ]
      Specifies a sequence of values @f$ s @f$. They define with which 
      @f$ h^s @f$ the corresponding matrices in injection_matrix are to be 
      scaled. That is, if you pass in a [3,0], then the first matrix in 
      injection_matrix will be scaled with  @f$ h^3 @f$ and the second one 
      with one. If you work with a simple function evaluation as injection, 
      then you are fine with one matrix, and its scaline will be [ 0 ].
    
    """  
    super( InjectAndProlong, self ).__init__(
      0,     #descend_invocation_order,
      False  # parallel
    )
    self.d = {}
    self.d["DG_SOLVER_INSTANCE"]    = dg_solver.instance_name()
    self.d["DG_SOLVER_NAME"]        = dg_solver.typename()
    self.d["CG_SOLVER_INSTANCE"]    = cg_solver.instance_name()
    self.d["CG_SOLVER_NAME"]        = cg_solver.typename()

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.templateTouchVertexFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
      result = jinja2.Template(self.templateTouchVertexLastTime).render(**self.d)
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
    return __name__.replace(".py", "").replace(".", "_") + "_InjectAndProlong"
    
  def user_should_modify_template(self):
    """!
    
    The action set that Peano will generate that corresponds to this class
    should not be modified by users and can safely be overwritten every time
    we run the Python toolkit.
    
    """
    return False

  def get_includes(self):
    """!
   
    We need the solver repository in this action set, as we directly access
    the solver object. We also need access to Peano's d-dimensional loops.
         
    """    
    return """
#include "repositories/SolverRepository.h"
#include "peano4/utils/Loop.h"
"""
