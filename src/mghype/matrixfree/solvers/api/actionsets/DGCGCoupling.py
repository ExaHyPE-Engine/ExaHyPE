from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


def prepare_and_order_DG_CG_solvers( dg_solver,
                                     cg_solver,
                                     ):
    """!
    
    Prepare CG solver for coupling and ensure that both solvers are invoked in the correct order
    
    Throughtout the mesh traversal steps from coarse to fine grid (descend), the
    DG solver is to be executed before(!) the CG solver. It is the DG solver 
    which we equip with a preprocessing step, so we know that we are on the safe 
    side here.
    
    We also introduce a new helper value for the CG solver: newValue has, 
    after each traversal, the injected value of the solution. We can then roll
    it over if we aim for a FAS-type implementation. Further to that, we have
    the solvers new rhs, which we again use for the next iteration, and its 
    previous (old) value which we use to compute the delta.
    
    """
    assert cg_solver._unknowns_per_vertex==dg_solver._unknowns_per_cell_node
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "newInjectedValue", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "oldValue", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "newRestrictedRhs", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "delta", str(cg_solver._unknowns_per_vertex) ) )
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

  Couple the solution and updates of two different solvers
  
  This class takes the solution of a DG solver and injects the current solution
  onto the vertices. For this, it simply averages the @f$ 2^d @f$ different 
  values of the DG polynomials into the vertex. We do this only for unrefined
  vertices obviously. 
  
  To realise this behaviour, the touchVertexFirstTime() instantiates the 
  template templateTouchVertexFirstTime which clears the current solution in 
  the CG solve, and it also plugs into touchVertexLastTime:
  
  - touchVertexFirstTime:
    - Store the value minus the previous value in delta. We will use delta to
      interpolate.
    - Set the value to the injected value which we know is correct from the 
      previous traversal.
    - Also set the old value to the injected value. We'll need it in the next
      traversal to compute delta.
    - Clear the CG solver's injected value to zero, as we'll accumulate into it
      from hereon.
  - touchVertexLastTime:
    - Nothing to be done. We are all fine. We know that new injected value is 
      valid now, but we'll roll it over in touchVertexFirstTime() of the 
      subsequent mesh sweep.
  
  It is important that prepare_and_order_DG_CG_solvers() has been invoked for
  the two solvers whenever we use this action set.
   
  """


  templateTouchVertexFirstTime="""
  if ( fineGridVertex{{CG_SOLVER_NAME}}.getType() == vertexdata::{{CG_SOLVER_NAME}}::Type::Interior ) {
    fineGridVertex{{CG_SOLVER_NAME}}.setDelta(
      fineGridVertex{{CG_SOLVER_NAME}}.getValue()
      -
      fineGridVertex{{CG_SOLVER_NAME}}.getOldValue()
    );
    fineGridVertex{{CG_SOLVER_NAME}}.setValue(
      fineGridVertex{{CG_SOLVER_NAME}}.getNewInjectedValue()
    );
    fineGridVertex{{CG_SOLVER_NAME}}.setOldValue(
      fineGridVertex{{CG_SOLVER_NAME}}.getNewInjectedValue()
    );
    fineGridVertex{{CG_SOLVER_NAME}}.setNewInjectedValue( 0.0 );
  }
"""


  templateTouchCellFirstTime="""
  if (fineGridCell{{DG_SOLVER_NAME}}.getType() != celldata::{{DG_SOLVER_NAME}}::Type::Coarse) {    
    constexpr int Cols = TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns;
    constexpr int Rows = {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode;
    
    static std::vector< tarch::la::Matrix< Rows, Cols, double > > matrices = {
      {% for MATRIX in PROLONGATION_MATRIX %}
      {
        {{MATRIX[0]| join(", ")}}
          {% for ROW in MATRIX[1:] %}
          ,{{ROW | join(", ")}}
          {% endfor %}
      },
      {% endfor %}
    };

    {% if PROLONGATION_MATRIX_SCALING is not defined %}
    #error No scaling for prolongation matrix available.
    {% endif %}

    static std::vector<int> scaleFactors = {
      {% for el in PROLONGATION_MATRIX_SCALING %}
        {{el}},
      {% endfor %}
    };
  
    tarch::la::Matrix< Rows, Cols, double > composedProlongationMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, marker.h() );

//    tarch::la::Vector< {{DG_SOLVER_NAME}}::NodesPerCell, double > prolongatedsValues = 
//      composedInjectionMatrix * fineGridCell{{CG_SOLVER_NAME}}.getDelta();
      
//    fineGridCell{{DG_SOLVER_NAME}}.setValue(
//      fineGridCell{{DG_SOLVER_NAME}}.getValue() + 
//      prolongatedValues
//    );
//      fineGridVertex{{SOLVER_NAME}}.setDelta(dof, val - fineGridVertex{{SOLVER_NAME}}.getValue(dof));
  }
"""



  templateTouchCellLastTime="""
  if (fineGridCell{{DG_SOLVER_NAME}}.getType() != celldata::{{DG_SOLVER_NAME}}::Type::Coarse) {    
    assertionEquals( {{CG_SOLVER_NAME}}::VertexUnknowns, {{DG_SOLVER_NAME}}::UnknownsPerCellNode );

    constexpr int Rows = TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns;
    constexpr int Cols = {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode;
    
    static std::vector< tarch::la::Matrix< Rows, Cols, double > > matrices = {
      {% for MATRIX in INJECTION_MATRIX %}
      {
        {{MATRIX[0]| join(", ")}}
          {% for ROW in MATRIX[1:] %}
          ,{{ROW | join(", ")}}
          {% endfor %}
      },
      {% endfor %}
    };

    {% if INJECTION_MATRIX_SCALING is not defined %}
    #error No scaling for injection matrix available.
    {% endif %}

    static std::vector<int> scaleFactors = {
      {% for el in INJECTION_MATRIX_SCALING %}
        {{el}},
      {% endfor %}
    };
  
    tarch::la::Matrix< Rows, Cols, double > composedInjectionMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, marker.h() );

    tarch::la::Vector< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, double > injectedValues = 
      composedInjectionMatrix * fineGridCell{{DG_SOLVER_NAME}}.getSolution();

    for (int vertex=0; vertex<TwoPowerD; vertex++) {
      fineGridVertices{{CG_SOLVER_NAME}}(vertex).setInjectedValue(
        fineGridVertices{{CG_SOLVER_NAME}}(vertex).getInjectedValue()
        + 
        1.0 / TwoPowerD 
        * 
        tarch::la::slice<{{CG_SOLVER_NAME}}::VertexUnknowns>(injectedValues,vertex*{{CG_SOLVER_NAME}}::VertexUnknowns)
      );
    }
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
    
    self.d["INJECTION_MATRIX"]            = injection_matrix
    self.d["INJECTION_MATRIX_SCALING"]    = injection_matrix_scaling
    self.d["PROLONGATION_MATRIX"]         = prolongation_matrix
    self.d["PROLONGATION_MATRIX_SCALING"] = prolongation_matrix_scaling
    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.templateTouchVertexFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = jinja2.Template(self.templateTouchCellLastTime).render(**self.d)
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
#include "mghype/mghype.h"
"""
