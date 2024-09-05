from peano4.solversteps.ActionSet import ActionSet

import dastgen2
import peano4
import jinja2
import mghype

class Test4Coupling(ActionSet):
  """!
  Using this action set isn't strictly necessary. But it's the easiest way
  to pipe in the prolongation / restriction matrices.

  During tcft we:
    - Set the auxiliary cell data by prolongating the aux vertex values
    - Set the auxiliary vertex data by restricting the aux cell values
  """

  templateEndTraversal="""
  Counter++;
  """

  templateTouchCellFirstTime="""
  if (
    fineGridCell{{DG_SOLVER_NAME}}.getType() != celldata::{{DG_SOLVER_NAME}}::Type::Coarse
    and
    Counter==0
  ) {
    constexpr int Cols = TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns;
    constexpr int Rows = {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode;

    // gather vertex values to interpolate upwards:
    tarch::la::Vector< Cols, double > vertexValues;
    for (int i=0; i<TwoPowerD; i++) vertexValues[i] = fineGridVertices{{CG_SOLVER_NAME}}(i).getValue( 0 );

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
  
    static tarch::la::Matrix< Rows, Cols, double > composedProlongationMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, marker.h() );

    // interpolate
    auto newDgVector1 = composedProlongationMatrix * vertexValues;
    fineGridCell{{DG_SOLVER_NAME}}.setVector1( newDgVector1 );

    // NEXT - do the same thing for the vertices...
    // forget about solution injection!!

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

    static tarch::la::Matrix< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, double > composedRestrictionMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( restrictionMatrices, restrictionMatricesScaleFactors, marker.h() );

    auto valuesToPutIntoVertex = composedRestrictionMatrix * fineGridCell{{DG_SOLVER_NAME}}.getSolution();
    for (int v=0; v<TwoPowerD; v++)
      fineGridVertices{{CG_SOLVER_NAME}}(v).setVector2(
        0, // assume only one value, so we ask for index 0
        fineGridVertices{{CG_SOLVER_NAME}}(v).getVector2(0) + valuesToPutIntoVertex(v)
      );

  }
  """

  def __init__(
    self,
    dg_solver,
    cg_solver,
    prolongation_matrix,
    prolongation_matrix_scaling,
    restriction_matrix,
    restriction_matrix_scaling,
    injection_matrix,
    injection_matrix_scaling
  ):
    
    super( Test4Coupling, self ).__init__(
      0,     #descend_invocation_order -> will be ignored
      False  # parallel
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
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      return jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      return jinja2.Template(self.templateEndTraversal).render(**self.d)
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
#include <random>
#include "peano4/utils/Loop.h"
#include "mghype/mghype.h"
"""

  def get_attributes(self):
    return """
    static int Counter;
  """

  def get_static_initialisations(self, full_qualified_classname):
    return f"""
  int {full_qualified_classname}::Counter = 0;
    """
  

class Test5Coupling(Test4Coupling):

  templateTouchCellFirstTime="""
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

  static tarch::la::Matrix< Rows, Cols, double > composedProlongationMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( matrices, scaleFactors, marker.h() );

  // restriction matrix
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

  static tarch::la::Matrix< TwoPowerD * {{CG_SOLVER_NAME}}::VertexUnknowns, {{DG_SOLVER_NAME}}::NodesPerCell * {{DG_SOLVER_NAME}}::UnknownsPerCellNode, double > composedRestrictionMatrix = ::mghype::composeMatrixFromHWeightedLinearCombination( restrictionMatrices, restrictionMatricesScaleFactors, marker.h() );

  if (
    fineGridCell{{DG_SOLVER_NAME}}.getType() != celldata::{{DG_SOLVER_NAME}}::Type::Coarse
    and
    Counter==0
  ) {
  // fetch the cg stuff
  tarch::la::Vector<TwoPowerD, double> u_cg_random;

  for (int i=0; i<TwoPowerD; i++) u_cg_random(i) = fineGridVertices{{CG_SOLVER_NAME}}(i).getRand(0);

  auto v_cell = composedProlongationMatrix * u_cg_random;
  
  fineGridCell{{DG_SOLVER_NAME}}.setVcell( 
    v_cell
  );

  // and now project that onto the faces
  auto faceProjections = repositories::{{DG_SOLVER_INSTANCE}}.getFaceFromCellMatrix( marker.x(), marker.h() ) * v_cell;

  for (int f=0; f<TwoTimesD; f++) {
    int startIndexProjection = 
      f < Dimensions ?
      repositories::{{DG_SOLVER_INSTANCE}}.NodesPerFace * repositories::{{DG_SOLVER_INSTANCE}}.ProjectionsPerFaceNode :
      0;
    for (int p=0; p<repositories::{{DG_SOLVER_INSTANCE}}.NodesPerFace * repositories::{{DG_SOLVER_INSTANCE}}.ProjectionsPerFaceNode; p++) {
      fineGridFaces{{DG_SOLVER_NAME}}(f).setProjection(
        p + startIndexProjection,
        faceProjections( f*repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsProjection + p + startIndexProjection )
      );

    }
  }

  }

  else if (
    fineGridCell{{DG_SOLVER_NAME}}.getType() != celldata::{{DG_SOLVER_NAME}}::Type::Coarse
    and
    Counter==1
  ) {

  tarch::la::Vector< repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsSolution*TwoTimesD, double > faceSol;
  for (int f=0; f<TwoTimesD; f++)
  for (int s=0; s<repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsSolution; s++)
    faceSol( f*repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsSolution + s ) = fineGridFaces{{DG_SOLVER_NAME}}(f).getSolutionTest(s);
  
  fineGridCell{{DG_SOLVER_NAME}}.setAdgp(
    repositories::{{DG_SOLVER_INSTANCE}}.getLocalAssemblyMatrix(marker.x(), marker.h()) * fineGridCell{{DG_SOLVER_NAME}}.getVcell()
    +
    repositories::{{DG_SOLVER_INSTANCE}}.getCellFromFaceMatrix(marker.x(), marker.h())  * faceSol
  );

  // now that's done, set v_restrict_ADG
  auto v_restrict_ADG = composedRestrictionMatrix * fineGridCell{{DG_SOLVER_NAME}}.getAdgp();

  for (int v=0; v<TwoPowerD; v++) 
    fineGridVertices{{CG_SOLVER_NAME}}(v).setVrestrictADG( fineGridVertices{{CG_SOLVER_NAME}}(v).getVrestrictADG(0) + v_restrict_ADG(v) );

  // finally, steps 9 and 10
  auto mdg_p = repositories::{{DG_SOLVER_INSTANCE}}.getMassMatrix(marker.x(), marker.h()) * fineGridCell{{DG_SOLVER_NAME}}.getVcell();

  fineGridCell{{DG_SOLVER_NAME}}.setMdgp(
    mdg_p
  );

  auto v_restrict_mdg = composedRestrictionMatrix * mdg_p;
  for (int v=0; v<TwoPowerD; v++)
    fineGridVertices{{CG_SOLVER_NAME}}(v).setVrestrictMDG( 0, fineGridVertices{{CG_SOLVER_NAME}}(v).getVrestrictMDG(0) + v_restrict_mdg(v) );

  }
  """
  
  templateTouchFaceLastTime="""
  // perform averaging on the faces
  if (
    fineGridFace{{DG_SOLVER_NAME}}.getType() == facedata::{{DG_SOLVER_NAME}}::Type::Interior
    and
    Counter == 0
  ) {
    // project the u^\pm into a temporary vector
    tarch::la::Vector< repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsSolution, double > projection = 
      repositories::{{DG_SOLVER_INSTANCE}}.getRiemannMatrix() * fineGridFace{{DG_SOLVER_NAME}}.getProjectionTest();

    for (int i=0; i<repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsSolution; i++)
      fineGridFace{{DG_SOLVER_NAME}}.setSolutionTest(
        i,
        projection(i)
      );
  }

  else if (
    fineGridFace{{DG_SOLVER_NAME}}.getType() == facedata::{{DG_SOLVER_NAME}}::Type::Boundary
    and
    Counter==0
  ) {
    // Essentially what we do here is fix the solution on the boundary to be 
    // the inner-side projection.

    // skip halfway along the projection vector if we are on face 0 or 1
    int startIndexProjection = 
      marker.getSelectedFaceNumber() < Dimensions ?
      repositories::{{DG_SOLVER_INSTANCE}}.NodesPerFace * repositories::{{DG_SOLVER_INSTANCE}}.ProjectionsPerFaceNode :
      0;

    auto boundaryMatrix = repositories::{{DG_SOLVER_INSTANCE}}.getBoundaryConditionMatrix();

    /*
    Here we do slightly messy matrix multiplication. We would just multiply the boundary matrix
    by the projection vector, and place that into the solution, but here we only want to capture
    half of the projection vector; the other half lies outside the computational domain and
    should be fixed to 0.
    */
    for (int row=0; row<boundaryMatrix.rows(); row++)
    {
      double rowSolution = 0;
      for (int col=0; col<boundaryMatrix.cols(); col++)
      {
        rowSolution += boundaryMatrix( row, col ) * fineGridFace{{DG_SOLVER_NAME}}.getProjection( col + startIndexProjection );
      }
      // place this solution into the face
      fineGridFace{{DG_SOLVER_NAME}}.setSolution( row, rowSolution );
    }

  }

  """

  def __init__(
    self,
    dg_solver,
    cg_solver,
    prolongation_matrix,
    prolongation_matrix_scaling,
    restriction_matrix,
    restriction_matrix_scaling,
    injection_matrix,
    injection_matrix_scaling
  ):
    
    super( Test5Coupling, self ).__init__(
      dg_solver,
      cg_solver,
      prolongation_matrix,
      prolongation_matrix_scaling,
      restriction_matrix,
      restriction_matrix_scaling,
      injection_matrix,
      injection_matrix_scaling
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
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      return jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME:
      return jinja2.Template(self.templateTouchFaceLastTime).render(**self.d)
    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      return jinja2.Template(self.templateEndTraversal).render(**self.d)
    return result
  

class Test7Coupling(mghype.matrixfree.solvers.api.actionsets.MultiplicativeDGCGCoupling):
  templateTouchCellFirstTime="""
  if (
    firstCycleCounter==0
    and
    fineGridCell{{DG_SOLVER_NAME}}.getType() != celldata::{{DG_SOLVER_NAME}}::Type::Coarse
  ){
    // finish updating rhs
    tarch::la::Vector< repositories::{{DG_SOLVER_INSTANCE}}.CellUnknowns, double > rhs;

    // fetch the faces
    tarch::la::Vector< repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsSolution*TwoTimesD, double > faceSol;
    for (int f=0; f<TwoTimesD; f++)
    for (int s=0; s<repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsSolution; s++)
      faceSol( f*repositories::{{DG_SOLVER_INSTANCE}}.FaceUnknownsSolution + s ) = fineGridFaces{{DG_SOLVER_NAME}}(f).getSolution(s);
    
    fineGridCell{{DG_SOLVER_NAME}}.setRhs(
      fineGridCell{{DG_SOLVER_NAME}}.getRhs()
      +
      repositories::{{DG_SOLVER_INSTANCE}}.getCellFromFaceMatrix(marker.x(), marker.h())  * faceSol
    );
  }
  """ + mghype.matrixfree.solvers.api.actionsets.MultiplicativeDGCGCoupling.templateTouchCellFirstTime

  templateEndTraversal = """
  firstCycleCounter++;
  """

  def get_includes(self):
    """!
   
    We need the solver repository in this action set, as we directly access
    the solver object. We also need access to Peano's d-dimensional loops.
         
    """    
    return """
#include "repositories/SolverRepository.h"
#include <random>
#include "peano4/utils/Loop.h"
#include "mghype/mghype.h"
#include "mghype/matrixfree/solvers/DGCGCoupling.h"
"""

  def get_attributes(self):
    return super().get_attributes() + """
    static int firstCycleCounter;
"""

  def get_static_initialisations(self, full_qualified_classname):
    return super().get_static_initialisations(full_qualified_classname) + f"""
int {full_qualified_classname}::firstCycleCounter = 0;
"""
  def get_body_of_operation(self,operation_name):
    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      return jinja2.Template(self.templateEndTraversal).render(**self.d)
    else:
      return super().get_body_of_operation(operation_name)
