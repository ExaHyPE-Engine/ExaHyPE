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
    
    @see AbstractDGCGCoupling for a description of where the individual values are used.
    
    """
    assert cg_solver._unknowns_per_vertex==dg_solver._unknowns_per_cell_node
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "sumOfInjectedValues", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "oldValue", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "newRestrictedRhs", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "delta", str(cg_solver._unknowns_per_vertex) ) )
    cg_solver._vertex_data.data.add_attribute( dastgen2.attributes.Integer( "numberOfRestrictions" ) )


class AbstractDGCGCoupling(ActionSet):
  """!

  Couple the solution and updates of two different solvers
  
 
  ## Usage
  
  This action set has to be used as a preprocessing step of the CG:
    
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  solver.preprocessing_action_set = mghype.matrixfree.solvers.api.actionsets.AbstractDGCGCoupling(
      solver,                      # dg_solver
      ...
      )
  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  
  I recommend not to use the coupling directly, but one of its subclasses.
  
  
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
  
  This class takes the solution of a DG solver and injects the current solution
  onto the vertices. For this, it simply averages the @f$ 2^d @f$ different 
  values of the DG polynomials into the vertex. We do this only for unrefined
  vertices obviously. 
  
  To realise this behaviour, we rely on functions held within 
  AbstractDGCGCoupling.h.   
  It is important that prepare_and_order_DG_CG_solvers() has been invoked for
  the two solvers whenever we use this action set.
   
  """


  __TemplateTouchVertexFirstTime="""
    mghype::matrixfree::solvers::dgcgcoupling::rollOverAndPrepareCGVertex( 
      fineGridVertex{{CG_SOLVER_NAME}},
      true, // restrictRhsAndInjectSolution
      {{USE_FAS}}
    );
  """


  templateTouchCellFirstTime="""
  // ============================
  // Interpolation
  // ============================
  const double additiveMultigridRelaxation = 0.0;
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

    fineGridCell{{DG_SOLVER_NAME}}.setSolution(
      mghype::matrixfree::solvers::dgcgcoupling::prolongate<{{DG_SOLVER_NAME}}, {{CG_SOLVER_NAME}}>( 
        composedProlongationMatrix,
        fineGridCell{{DG_SOLVER_NAME}}.getSolution(),
        fineGridVertices{{CG_SOLVER_NAME}}
      )
    );
  }
"""



  __TemplateTouchCellLastTime_FAS="""
  if (fineGridCell{{DG_SOLVER_NAME}}.getType() != celldata::{{DG_SOLVER_NAME}}::Type::Coarse) {    
    assertionEquals( {{CG_SOLVER_NAME}}::VertexUnknowns, {{DG_SOLVER_NAME}}::UnknownsPerCellNode );

    // ==========================================
    // compute and restrict hierarchical residual
    // ==========================================
    {
      static std::vector< tarch::la::Matrix< {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, double > > prolongationMatrices = {
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

      tarch::la::Matrix< {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, double > composedProlongationMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( prolongationMatrices, prolongationMatricesScaleFactors, marker.h() );
    
      static std::vector< tarch::la::Matrix< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, double > > restrictionMatrices = {
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
  
      tarch::la::Matrix< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, double > composedRestrictionMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( restrictionMatrices, restrictionMatricesScaleFactors, marker.h() );

      mghype::matrixfree::solvers::dgcgcoupling::computeAndRestrictHierarchicalResidual<{{DG_SOLVER_NAME}}, {{CG_SOLVER_NAME}}>( 
        composedProlongationMatrix,
        composedRestrictionMatrix,
        repositories::{{DG_SOLVER_INSTANCE}}.getMassMatrix( marker.x(), marker.h() ),
        repositories::{{DG_SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()),
        fineGridCell{{DG_SOLVER_NAME}}.getSolution(),
        fineGridCell{{DG_SOLVER_NAME}}.getRhs(),
        fineGridVertices{{CG_SOLVER_NAME}}
      );    
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

      mghype::matrixfree::solvers::dgcgcoupling::injectSolution<{{DG_SOLVER_NAME}}, {{CG_SOLVER_NAME}}>( 
        composedInjectionMatrix,
        fineGridCell{{DG_SOLVER_NAME}}.getSolution(),
        fineGridVertices{{CG_SOLVER_NAME}}
      );    
    }
    
    // ==============
    // Update counter
    // ==============
    mghype::matrixfree::solvers::dgcgcoupling::updateRestrictionCounters( 
      fineGridVertices{{CG_SOLVER_NAME}}
    );    
  }
"""

  
  __TemplateTouchCellLastTime_CorrectionScheme="""
  if (fineGridCell{{DG_SOLVER_NAME}}.getType() != celldata::{{DG_SOLVER_NAME}}::Type::Coarse) {    
    assertionEquals( {{CG_SOLVER_NAME}}::VertexUnknowns, {{DG_SOLVER_NAME}}::UnknownsPerCellNode );

    // =============================
    // compute and restrict residual
    // =============================
    {
      static std::vector< tarch::la::Matrix< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, double > > restrictionMatrices = {
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
  
      tarch::la::Matrix< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, double > composedRestrictionMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( restrictionMatrices, restrictionMatricesScaleFactors, marker.h() );

      mghype::matrixfree::solvers::dgcgcoupling::computeAndRestrictResidual<{{DG_SOLVER_NAME}}, {{CG_SOLVER_NAME}}>( 
        composedRestrictionMatrix,
        repositories::{{DG_SOLVER_INSTANCE}}.getMassMatrix( marker.x(), marker.h() ),
        repositories::{{DG_SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()),
        fineGridCell{{DG_SOLVER_NAME}}.getSolution(),
        fineGridCell{{DG_SOLVER_NAME}}.getRhs(),
        fineGridVertices{{CG_SOLVER_NAME}}
      );    
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

      mghype::matrixfree::solvers::dgcgcoupling::injectSolution<{{DG_SOLVER_NAME}}, {{CG_SOLVER_NAME}}>( 
        composedInjectionMatrix,
        fineGridCell{{DG_SOLVER_NAME}}.getSolution(),
        fineGridVertices{{CG_SOLVER_NAME}}
      );    
    }
    
    // ==============
    // Update counter
    // ==============
    mghype::matrixfree::solvers::dgcgcoupling::updateRestrictionCounters( 
      fineGridVertices{{CG_SOLVER_NAME}}
    );    
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
               injection_matrix_scaling,
               use_fas
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
      
    @param use_fas: Boolean
      If this flag is set, we realise a FAS following the HTMG idea by 
      Griebel et al.
    
    """  
    super( AbstractDGCGCoupling, self ).__init__(
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

    if use_fas:
      self.d["USE_FAS"]                   = "true"
      self._use_fas                       = use_fas
    else:
      self.d["USE_FAS"]                   = "false"
      self._use_fas                       = use_fas
    

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
      result = jinja2.Template(self.__TemplateTouchVertexFirstTime).render(**self.d)
      pass
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass
    if self._use_fas and operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = jinja2.Template(self.__TemplateTouchCellLastTime_FAS).render(**self.d)
      pass
    if not self._use_fas and operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = jinja2.Template(self.__TemplateTouchCellLastTime_CorrectionScheme).render(**self.d)
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
#include "mghype/matrixfree/solvers/DGCGCoupling.h"
"""

        
class AdditiveDGCGCoupling(AbstractDGCGCoupling):
    """!
        
    Introduce additive CC Coupling
    
    The control logic here is simplistic and prescripted. We follow the 
    following steps for the additive solver with one iteration:
    
    1. Let the DGSolver project its solution onto the faces.
    2. Pre step
       - Restrict residual and inject solution (if FAS)
       - Suspend CG solver
       - Let DG solver update solution already (post restriction)
    3. Compute step
       - Switch DG solver's state to project-onto-faces
       - Interpolate delta
    
    The pre-step can be identified by the fact that we run into the first
    
    """
    def __init__(self,
                 dg_solver,
                 cg_solver,
                 prolongation_matrix,
                 prolongation_matrix_scaling,
                 restriction_matrix,
                 restriction_matrix_scaling,
                 injection_matrix,
                 injection_matrix_scaling,
                 use_fas
                 ):        
        super( AdditiveDGCGCoupling, self ).__init__(
            dg_solver,
            cg_solver,
            prolongation_matrix,
            prolongation_matrix_scaling,
            restriction_matrix,
            restriction_matrix_scaling,
            injection_matrix,
            injection_matrix_scaling,
            use_fas
            )
        