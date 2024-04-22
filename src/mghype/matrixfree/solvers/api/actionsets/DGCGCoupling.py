from peano4.solversteps.ActionSet import ActionSet

import dastgen2
import peano4
import jinja2


def prepare_CG_correction_solvers( dg_solver,
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
    
    @see DGCGCoupling for a description of where the individual values are used.
    
    """
    assert cg_solver._unknowns_per_vertex==dg_solver._unknowns_per_cell_node
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "sumOfInjectedValues", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "oldValue", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "newRestrictedRhs", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "delta", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( dastgen2.attributes.Integer( "numberOfRestrictions" ) )


class DGCGCoupling(ActionSet):
  """!

  Couple the solution and updates of two different solvers
  
  This action set has to be used as a preprocessing step of the CG.
  
  This class takes the solution of a DG solver and injects the current solution
  onto the vertices. For this, it simply averages the @f$ 2^d @f$ different 
  values of the DG polynomials into the vertex. We do this only for unrefined
  vertices obviously. 
  
  
  ## Behaviour
  
  We have a new variable SumOfInjectedValues. This one is set to zero at the end
  of touchVertexFirstTime(), i.e. at the begin of a mesh sweep. In each fine 
  grid cell, i.e. each cell carrying a DG polynomial, we evaluate this
  polynomial's value in the @f$ 2^d @f$ vertices (corners) of the cell. Of 
  these values, we restrict @f$ 2^{=-d} @f$th of its value to the next coarser
  mesh level. Prior to the start of a new mesh sweep, SumOfInjectedValues holds 
  the average value of the solution in a vertex. This is just before we 
  clear it again, i.e. we can plug into touchVertexFirstTime() just before we
  clear this field to read our a coarsened representation of the solution at 
  this point.
  
  
  
  ## Realisation
  
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


  """!
  
  We have to get boundary vertices right, but we don't know about them. So we 
  use the number of restrictions counter. Even if this one is garbage, it will
  only be garbage in one initial step. After that, we'll update it properly.
  
  """
  templateTouchVertexFirstTime="""
  if ( 
    fineGridVertex{{CG_SOLVER_NAME}}.getNumberOfRestrictions()<=0
    or
    fineGridVertex{{CG_SOLVER_NAME}}.getNumberOfRestrictions()>TwoPowerD
  ) {
    fineGridVertex{{CG_SOLVER_NAME}}.setDelta(0.0);
    fineGridVertex{{CG_SOLVER_NAME}}.setValue(0.0);
    fineGridVertex{{CG_SOLVER_NAME}}.setRhs( 0.0 );
    fineGridVertex{{CG_SOLVER_NAME}}.setOldValue(0.0);
  }
  else {
    fineGridVertex{{CG_SOLVER_NAME}}.setDelta(
      fineGridVertex{{CG_SOLVER_NAME}}.getValue()
      -
      fineGridVertex{{CG_SOLVER_NAME}}.getOldValue()
    );
    fineGridVertex{{CG_SOLVER_NAME}}.setValue(
      1.0 / fineGridVertex{{CG_SOLVER_NAME}}.getNumberOfRestrictions()
      *
      fineGridVertex{{CG_SOLVER_NAME}}.getSumOfInjectedValues()
    );
    fineGridVertex{{CG_SOLVER_NAME}}.setOldValue(
      fineGridVertex{{CG_SOLVER_NAME}}.getSumOfInjectedValues()
    );    
    fineGridVertex{{CG_SOLVER_NAME}}.setRhs( 
      fineGridVertex{{CG_SOLVER_NAME}}.getNewRestrictedRhs() 
     );
  }
  fineGridVertex{{CG_SOLVER_NAME}}.setSumOfInjectedValues( 0.0 );
  fineGridVertex{{CG_SOLVER_NAME}}.setNumberOfRestrictions( 0 );
  fineGridVertex{{CG_SOLVER_NAME}}.setNewRestrictedRhs( 0.0 );
"""


  templateTouchCellFirstTime="""
  // ============================
  // Interpolation
  // ============================
  // @todo remove the false here
  if (false and fineGridCell{{DG_SOLVER_NAME}}.getType() != celldata::{{DG_SOLVER_NAME}}::Type::Coarse) {    
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
    
    tarch::la::Vector< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, double > coarseDeltaValues;
    for (int vertex=0; vertex<TwoPowerD; vertex++)
    for (int unknown=0; unknown<{{CG_SOLVER_NAME}}::VertexUnknowns; unknown++) {
      coarseDeltaValues( vertex*{{CG_SOLVER_NAME}}::VertexUnknowns+unknown ) = fineGridVertices{{CG_SOLVER_NAME}}(vertex).getDelta(unknown);
    }

    tarch::la::Vector< {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, double > prolongatedValues = 
      composedProlongationMatrix * coarseDeltaValues;
      
    fineGridCell{{DG_SOLVER_NAME}}.setSolution(
      fineGridCell{{DG_SOLVER_NAME}}.getSolution() + 
      prolongatedValues
    );
  }
"""



  templateTouchCellLastTime="""
  if (fineGridCell{{DG_SOLVER_NAME}}.getType() != celldata::{{DG_SOLVER_NAME}}::Type::Coarse) {    
    assertionEquals( {{CG_SOLVER_NAME}}::VertexUnknowns, {{DG_SOLVER_NAME}}::UnknownsPerCellNode );

    tarch::la::Vector< {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, double > hierarchicalResidual;

    // ============================
    // compute hierarchical residual
    // ============================
    // @todo false
    if (false)
    {
      constexpr int Cols = TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns;
      constexpr int Rows = {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode;
    
      static std::vector< tarch::la::Matrix< Rows, Cols, double > > prolongationMatrices = {
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
      #error No scaling for injection matrix available.
      {% endif %}

      static std::vector<int> prolongationMatricesScaleFactors = {
        {% for el in PROLONGATION_MATRIX_SCALING %}
         {{el}},
        {% endfor %}
      };
  
      tarch::la::Matrix< Rows, Cols, double > composedProlongationMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( prolongationMatrices, prolongationMatricesScaleFactors, marker.h() );

      tarch::la::Vector< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, double > coarseDeltaValues;
      for (int vertex=0; vertex<TwoPowerD; vertex++)
      for (int unknown=0; unknown<{{CG_SOLVER_NAME}}::VertexUnknowns; unknown++) {
        coarseDeltaValues( vertex*{{CG_SOLVER_NAME}}::VertexUnknowns+unknown ) = fineGridVertices{{CG_SOLVER_NAME}}(vertex).getValue(unknown);
      }

      tarch::la::Vector< {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, double > prolongatedValues =  
        composedProlongationMatrix * coarseDeltaValues;
      
      hierarchicalResidual =
        repositories::{{DG_SOLVER_INSTANCE}}.getMassMatrix( marker.x(), marker.h() ) 
        * 
        fineGridCell{{DG_SOLVER_NAME}}.getRhs()
        -
        repositories::{{DG_SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) 
        * 
        (        
          prolongatedValues
          -
          fineGridCell{{DG_SOLVER_NAME}}.getSolution()
        );

    }
    
    // ============================
    // restrict hierarchical residual
    // ============================
    // @todo false
    if (false)
    {
      constexpr int Rows = TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns;
      constexpr int Cols = {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode;
    
      static std::vector< tarch::la::Matrix< Rows, Cols, double > > restrictionMatrices = {
        {% for MATRIX in RESTRICTION_MATRIX %}
         {
          {{MATRIX[0]| join(", ")}}
            {% for ROW in MATRIX[1:] %}
            ,{{ROW | join(", ")}}
            {% endfor %}
        },
        {% endfor %}
      };

      {% if RESTRICTION_MATRIX_SCALING is not defined %}
      #error No scaling for injection matrix available.
      {% endif %}

      static std::vector<int> restrictionMatricesScaleFactors = {
        {% for el in INJECTION_MATRIX_SCALING %}
         {{el}},
        {% endfor %}
      };
  
      tarch::la::Matrix< Rows, Cols, double > composedRestrictionMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( restrictionMatrices, restrictionMatricesScaleFactors, marker.h() );

      tarch::la::Vector< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, double > restrictedValues = 
        composedRestrictionMatrix * hierarchicalResidual;

      for (int vertex=0; vertex<TwoPowerD; vertex++) {
        fineGridVertices{{CG_SOLVER_NAME}}(vertex).setNewRestrictedRhs(
          fineGridVertices{{CG_SOLVER_NAME}}(vertex).getNewRestrictedRhs()
          + 
          tarch::la::slice<{{CG_SOLVER_NAME}}::VertexUnknowns>(restrictedValues,vertex*{{CG_SOLVER_NAME}}::VertexUnknowns)
        );
      }
    }
      
    // ============================
    // inject solution back (accumulation)
    // ============================
    {
      constexpr int Rows = TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns;
      constexpr int Cols = {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode;
    
      static std::vector< tarch::la::Matrix< Rows, Cols, double > > injectionMatrices = {
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

      static std::vector<int> injectionMatricesScaleFactors = {
        {% for el in INJECTION_MATRIX_SCALING %}
         {{el}},
        {% endfor %}
      };
  
      tarch::la::Matrix< Rows, Cols, double > composedInjectionMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( injectionMatrices, injectionMatricesScaleFactors, marker.h() );

      tarch::la::Vector< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, double > injectedValues = 
        composedInjectionMatrix * fineGridCell{{DG_SOLVER_NAME}}.getSolution();

      for (int vertex=0; vertex<TwoPowerD; vertex++) {
        fineGridVertices{{CG_SOLVER_NAME}}(vertex).setSumOfInjectedValues(
          fineGridVertices{{CG_SOLVER_NAME}}(vertex).getSumOfInjectedValues()
          + 
          tarch::la::slice<{{CG_SOLVER_NAME}}::VertexUnknowns>(injectedValues,vertex*{{CG_SOLVER_NAME}}::VertexUnknowns)
        );
      }
    }
    
    // ==============
    // Update counter
    // ==============
    for (int vertex=0; vertex<TwoPowerD; vertex++) {
      fineGridVertices{{CG_SOLVER_NAME}}(vertex).setNumberOfRestrictions( 
        fineGridVertices{{CG_SOLVER_NAME}}(vertex).getNumberOfRestrictions()+1
      );
    }
  }
"""


  def __init__(self,
               dg_solver,
               cg_solver,
               prolongation_matrix,
               prolongation_matrix_scaling,
               restriction_matrix,
               restriction_matrix_scaling,
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
    super( DGCGCoupling, self ).__init__(
      0,     #descend_invocation_order -> will be ignored
      False  # parallel
    )
    
    prepare_CG_correction_solvers( dg_solver,
                                   cg_solver,
                                   )
    
    dg_solver._basic_descend_order = cg_solver._basic_descend_order - 1

    self.d = {}
    self.d["DG_SOLVER_INSTANCE"]    = dg_solver.instance_name()
    self.d["DG_SOLVER_NAME"]        = dg_solver.typename()
    self.d["CG_SOLVER_INSTANCE"]    = cg_solver.instance_name()
    self.d["CG_SOLVER_NAME"]        = cg_solver.typename()
    
    self.d["INJECTION_MATRIX"]            = injection_matrix
    self.d["INJECTION_MATRIX_SCALING"]    = injection_matrix_scaling
    self.d["PROLONGATION_MATRIX"]         = prolongation_matrix
    self.d["PROLONGATION_MATRIX_SCALING"] = prolongation_matrix_scaling
    self.d["RESTRICTION_MATRIX"]          = restriction_matrix
    self.d["RESTRICTION_MATRIX_SCALING"]  = restriction_matrix_scaling
    

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
