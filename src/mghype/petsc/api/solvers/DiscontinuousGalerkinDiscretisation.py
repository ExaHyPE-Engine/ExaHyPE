# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import peano4
import jinja2
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2

import os

from .Solver                                              import Solver
from multigrid.petsc.api.actionsets.EnumerateDoFs             import EnumerateDoFs
from multigrid.petsc.api.actionsets.InitCellDoFs              import InitCellDoFs
from multigrid.petsc.api.actionsets.ProjectPETScSolutionBackOntoMesh    import ProjectPETScSolutionOnCellsBackOntoMesh
from multigrid.petsc.api.actionsets.PlotDGDataInPeanoBlockFormat        import PlotDGDataInPeanoBlockFormat
from multigrid.petsc.api.actionsets.PlotExactSolution         import PlotExactSolution
from multigrid.petsc.api.actionsets.ImposeDirichletBoundaryConditionsWithInteriorPenaltyMethod   import ImposeDirichletBoundaryConditionsWithInteriorPenaltyMethod 

from abc import abstractmethod

from peano4.solversteps.ActionSet import ActionSet



class AssemblePetscMatrix(ActionSet):
  """!

  Trigger assembly of PETSc matrix and also assemble rhs vector

  """
  TemplateTouchCellFirstTime = """
  if (fineGridCell{{SOLVER_NAME}}PETScData.getType() == celldata::{{SOLVER_NAME}}PETScData::Type::Interior) {
    std::pair<int,int> localCellIndex = std::make_pair(_spacetreeId, fineGridCell{{SOLVER_NAME}}PETScData.getUnknownBaseNumber());
    int globalCellIndex = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localCellIndex, ::petsc::LocalToGlobalMap::Type::Cell);

    // ================================
    // Left-hand side A_cc contribution
    // ================================
    auto lhsCellMatrix = repositories::{{SOLVER_INSTANCE}}.getLhsMatrix(marker.x(),marker.h());

    logTraceInWith3Arguments( "touchCellFirstTime()", globalCellIndex, lhsCellMatrix.toString(), marker.toString() );

    for (int row=0; row<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; row++) 
    for (int col=0; col<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; col++) {
      repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
        globalCellIndex+row, 
        globalCellIndex+col, 
        lhsCellMatrix(row,col)
      );
    }
    logTraceOut( "touchCellFirstTime()" );
    
    // ================================
    // Right-hand side vector contribution (multiply rhs (mass) matrix with rhs initial values
    // ================================
    auto rhsCellMatrix = repositories::{{SOLVER_INSTANCE}}.getRhsMatrix(marker.x(),marker.h());

    for (int row=0; row<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; row++) {
      double rhs = 0.0;
      for (int col=0; col<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; col++) {
        rhs += rhsCellMatrix(row,col) * fineGridCell{{SOLVER_NAME}}.getRhs(col);
      }
      repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
        globalCellIndex+row, rhs
      );
    }
    
    auto projectionCellToFaceMatrix = repositories::{{SOLVER_INSTANCE}}.getProjectionOfCellDataOntoFace(marker.x(),marker.h());
    auto projectionFaceToCellMatrix = repositories::{{SOLVER_INSTANCE}}.getProjectionOfRiemannSolutionOntoCell(marker.x(),marker.h());
    int dofsPerFace = repositories::{{SOLVER_INSTANCE}}.NodesPerFace*repositories::{{SOLVER_INSTANCE}}.FaceUnknowns;
    
    for (int d=0; d<Dimensions; d++) {
      // =================================
      // Project to left face along axis d
      // =================================
      if ( 
        fineGridFaces{{SOLVER_NAME}}PETScData(d).getType()==facedata::{{SOLVER_NAME}}PETScData::Type::Interior 
        or
        fineGridFaces{{SOLVER_NAME}}PETScData(d).getType()==facedata::{{SOLVER_NAME}}PETScData::Type::Boundary
      ) {
        std::pair<int,int> localFirstDoFIndexOfLeftFace  = std::make_pair(_spacetreeId, fineGridFaces{{SOLVER_NAME}}PETScData(d).getUnknownBaseNumber());
        int globalFirstDoFIndexOfLeftFace  = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localFirstDoFIndexOfLeftFace, ::petsc::LocalToGlobalMap::Type::Face);
      
        assertion1(globalFirstDoFIndexOfLeftFace>=0,  marker.toString());
        logTraceInWith5Arguments( "touchCellFirstTime::LeftProjection", globalFirstDoFIndexOfLeftFace, globalCellIndex , projectionCellToFaceMatrix, projectionFaceToCellMatrix, marker.toString() );

        for (int row=0; row<dofsPerFace; row++ ) {
          for (int col=0; col<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; col++) {
            logTraceIn("CellToFace");
            repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
              globalFirstDoFIndexOfLeftFace + row + dofsPerFace, 
              globalCellIndex+col,
              projectionCellToFaceMatrix(d * dofsPerFace + row,col)
            );
            logTraceOut("CellToFace");

            logTraceInWith1Argument("FaceToCellInsertion", projectionFaceToCellMatrix(col, d * dofsPerFace + row));
            repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
              globalCellIndex+col,
              globalFirstDoFIndexOfLeftFace + row + 2 * dofsPerFace, 
              projectionFaceToCellMatrix(col, d * dofsPerFace + row)
            );
            logTraceOut("FaceToCell");
          }
        }
        logTraceOut( "touchCellFirstTime::LeftProjection" );
      }
      
      if ( 
        fineGridFaces{{SOLVER_NAME}}PETScData(d+Dimensions).getType()==facedata::{{SOLVER_NAME}}PETScData::Type::Interior 
        or
        fineGridFaces{{SOLVER_NAME}}PETScData(d+Dimensions).getType()==facedata::{{SOLVER_NAME}}PETScData::Type::Boundary
      ) {
        std::pair<int,int> localFirstDoFIndexOfRightFace = std::make_pair(_spacetreeId, fineGridFaces{{SOLVER_NAME}}PETScData(d+Dimensions).getUnknownBaseNumber());
        int globalFirstDoFIndexOfRightFace = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localFirstDoFIndexOfRightFace, ::petsc::LocalToGlobalMap::Type::Face);
      
        assertion1(globalFirstDoFIndexOfRightFace>=0, marker.toString());

        logTraceInWith2Arguments( "touchCellFirstTime::RightProjection", globalFirstDoFIndexOfRightFace, marker.toString() );
        
        for (int row=0; row<dofsPerFace; row++ ) {
          for (int col=0; col<repositories::{{SOLVER_INSTANCE}}.DoFsPerCell; col++) {
            repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
              globalFirstDoFIndexOfRightFace + row, 
              globalCellIndex+col,
              projectionCellToFaceMatrix(d * dofsPerFace + row + Dimensions*dofsPerFace,col)
            );
            repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
              globalCellIndex+col,
              globalFirstDoFIndexOfRightFace + row + 2 * dofsPerFace, 
              projectionFaceToCellMatrix(col, d * dofsPerFace + row + Dimensions*dofsPerFace)
            );
          }
        }
      } // end of if interior face

      logTraceOut( "touchCellFirstTime::RightProjection" );
    }
  
  }
  """

  TemplateTouchFaceFirstTime="""
  auto RiemannSolverMatrix = repositories::{{SOLVER_INSTANCE}}.getRiemannSolver(marker.x(),marker.h());
  int dofsPerFace          = repositories::{{SOLVER_INSTANCE}}.NodesPerFace*repositories::{{SOLVER_INSTANCE}}.FaceUnknowns;
  if ( fineGridFace{{SOLVER_NAME}}PETScData.getType() == facedata::{{SOLVER_NAME}}PETScData::Type::Interior ) {
    std::pair<int,int> localFirstDoFIndex = std::make_pair(_spacetreeId, fineGridFace{{SOLVER_NAME}}PETScData.getUnknownBaseNumber());
    int globalFirstDoFIndex  = repositories::{{SOLVER_INSTANCE}}.getLocalToGlobalMap().getGlobalIndex(localFirstDoFIndex, ::petsc::LocalToGlobalMap::Type::Face);

    assertion( globalFirstDoFIndex>=0 );

    logTraceInWith2Arguments("touchFaceFirstTime", globalFirstDoFIndex, RiemannSolverMatrix);
    for (int row=0; row<dofsPerFace; row++)

    // we have col<2*dofsPerFace here since we have left and right projections here.
    // ie, we have a certain number of "true" dofsPerFace and then twice as many
    // helper dofs to hold the projections. 
    // In this loop, we are setting the values of the true face Dofs, using the 
    // left and right projections. This is why we get the global index of the 
    // face and offset by 2*dofsPerFace: this will index the true dofs,
    // which are enumerated AFTER the projections.
    for (int col=0; col<2*dofsPerFace; col++) {
      repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
        globalFirstDoFIndex + 2*dofsPerFace + row,
        globalFirstDoFIndex + col, 
        RiemannSolverMatrix(row, col)
      );
    }

    // 3*dofsPerFace so that each of the -, + and solution variables receive an identity here.
    for (int i=0; i<3*dofsPerFace; i++)
    {
      repositories::{{SOLVER_INSTANCE}}.getLinearEquationSystem().insert(
        globalFirstDoFIndex + i,
        globalFirstDoFIndex + i,
        -1
      );
    }

  logTraceOut("touchFaceFirstTime");

  }
  else if ( fineGridFace{{SOLVER_NAME}}PETScData.getType() == facedata::{{SOLVER_NAME}}PETScData::Type::Boundary ){
    // add some more identities!
  }
  """
  
  def __init__(self,
               solver,
               ):
    """!
    
    Yet to be written
        
    """
    
    super( AssemblePetscMatrix, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]        = solver.instance_name()
    self.d["SOLVER_NAME"]            = solver.typename()


  def get_body_of_operation(self,operation_name):
    """!

Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME  
Provide C++ code snippet for peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME  
  
Only touchVertexFirstTime is an event where this action set actually
does something: It inserts the template TemplateTouchVertexFirstTime and 
replaces it with entries from the dictionary. The latter is befilled
in init().

We actually do something during touchVertexFirstTime and touchCellFirstTime. We insert the 
appropriate template into each.
    
    """
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateTouchCellFirstTime).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.TemplateTouchFaceFirstTime).render(**self.d)
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
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
#include "tarch/la/Matrix.h"
"""

  def get_attributes(self):
    """!
    
    
    """
    return f"""
  int _spacetreeId;    
"""
      
  def get_constructor_body(self):
    """!
    
Define body of constructor

@see get_attributes()
    
    """
    return f"""
    _spacetreeId = treeNumber;
"""



class DiscontinuousGalerkinDiscretisation(Solver):
  """!
   
  \page petsc_dg_solver Discontinuous Galerkin Solver with PETSc

  ## The weak formulation

  To translate a PDE into an equation system, we express the solution 

  @f$ u = \sum _i u_i \phi _i @f$

  as sum over shape functions @f$ \phi _i @f$. 
  The shape functions in this class are locally continuous, but their linear 
  combination is not. We test this setup against test functions @f$ v @f$.
  Each test function is actually any of the shape functions @f$ \phi _i @f$.
  Drawing both shapes and test functions from the same function space means that
  we follow a ***Ritz-Galerkin*** approach.


  Let @f$ \mathcal{L}(u) @f$ be the differential operator on the left-hand side
  of the PDE, i.e. let's assume that we solve

  @f$
    \mathcal{L}(u) = f.
  @f$

  A weak formulation means that we multiple both sides with a test function and 
  then integrate over the whole thing. As we take all these tests from the same 
  function space as the shape functions, we know that they all have local 
  support.Consequently, our test integral "shrinks" down to the support of the 
  test function

  @f$
    \int _\Omega \mathcal{L}(u) \cdot v dx = \int_{\text{cell}_v} \mathcal{L}(u) \cdot v dx
  @f$

  as the product disappears everywhere outside of the cell that hosts the test 
  function of interest.


  Once again, we can integrate by parts to get rid of the highest derivative in
  our partial differential operator @f$ \mathcal{L}(u) @f$, but this time, we will 
  get an expression of the form 


\f{eqnarray*}{
  \int _\Omega \mathcal{L}(u) \cdot v dx 
  &=& 
  - \int _\Omega \left( \tilde{\mathcal{L}}(u), \nabla v \right) dx 
  + \int _{\partial \text{cell}_v} (n,\tilde{\mathcal{M}}(u)) v dS(x)
  \\
  &=& 
  - \int _{\text{cell}_v} \left( \tilde{\mathcal{L}}(u), \nabla v \right) dx 
  + \int _{\partial \text{cell}_v} (n,\tilde{\mathcal{M}}(\hat u )) v dS(x)
  \\
  & = & \int _{\text{cell}_v} f \cdot v dx.
\f}

  We reduced the order of the deriative within @f$ \mathcal{L}(u) @f$ and got a 
  new operator @f$ \tilde{\mathcal{L}} @f$.
  If  @f$ \mathcal{L}(u) @f$ is dominated by a second derivative, @f$ \tilde{\mathcal{L}} @f$ 
  hosts only first-order derivatives.
  But this is a Pyrrhic victory:
  We now get an additional PDE term over the cell faces.


  We conclude that

  1. the jump terms along the faces do not disappear, i.e. they do not cancel out 
     from left and right as they do in a Continuous Galerkin formulation. 
  2. Even worse, they refer to a value @f$ u @f$ which technically does not 
     exist, as there is no (unique) solution along the face. We only have a 
     left @f$ u^- @f$ and a right @f$ u^+ @f$ solution. Therefore, I explicitly
     write @f$ \hat u @f$ in the equation above. It denotes that we haven't yet
     decided what the function looks like along the face.
  3. We now have cell integrals to evaluate, and there are additional face 
     integrals. The latter ones couple neighbouring cells.    

  We end up with a linear equation system 

  @f$ A x = b @f$

  where x has @f$ |\mathbb{C}| \cdot K  \cdot (p+1)^d @f$ entries. @f$ \mathbb{C}  @f$ 
  is set of cells in the mesh.


  ## An operator language and temporary variables
    
  Before we dive into examples or discuss how to map this formalism onto code, we 
  introduce an operator language. That is, we break down the overall computation
  into small tasks. These tasks will be found 1:1 in the code later on. Furthermore,
  we gain some freedom by breaking things down into tiny bits and pieces: We can
  alter individual ones or puzzle them together in various ways.
    
  We introduce a few operators (functions) to break down the overall computational 
  scheme into a sequence/set of computations:
    
  - The operator @f$ A_{cc} @f$ takes the dofs within a cell and computes the 
    term @f$ - \int _{\text{cell}_v} \left( \tilde{\mathcal{L}}(u), \nabla v \right) dx @f$.
    It takes the @f$ K (p+1)^d @f$ values per cell and yields @f$ K (p+1)^d @f$
    equations/updates from the cell. In classes like petsc.solvers.DiscontinuousGalerkinDiscretisation
    this matrix is called cell_cell_lhs_matrix.
  - Let's introduce temporary variables on the face. Those are the 
    @f$ u^+ @f$ and @f$ u^- @f$ values, i.e. we store @f$ 2K (p+1)^{d-1} @f$
    degrees of freedom on the face. Those are not really degrees of freedom.
    They will be mere projection, i.e. simply encode what the solution along
    the face is.
  - The operator @f$ A_{cf} @f$ takes the solution from a cell and projects it 
    onto a face. Let @f$ u^+ @f$ and @f$ u^- @f$ denote the solution left or 
    right of a face (from the face's point of view). For each of the @f$ 2d @f$ 
    faces of a cell, the operator @f$ P_{cf} @f$ yields either the 
    @f$ u^+ @f$ or @f$ u^- @f$.
  - The operator @f$ A_{ff} @f$ operates on a face. It takes 
    @f$ u^+ @f$ and @f$ u^- @f$ and yields something
    that "reconstructs" the solution and applies the flux operator on it. 
    This is the Riemann solver and hence linear algebraises the 
    @f$ \tilde{\mathcal{M}}(\hat u ) @f$ from above.
  - The operator @f$ A_{fc} @f$ takes the @f$ \hat u @f$ on the face and yields
    the updates for the cell. It evaluates @f$ \int _{\partial \text{cell}_v} (n,\tilde{\mathcal{M}}(\hat u )) v dS(x) @f$.
  
  
  <div style="background-color: #cfc ; padding: 10px; border: 1px solid green;"> 
    Within Peano, it is important to distinguish data on cell and faces carefully 
    from each other. Furthermore
    (i) we can read data associated with faces within cells;
    (ii) we can never read data from neighbouring cells within a cell;
    (iii) we can never access adjacent cell data from a face.
  </div>
    
    
    
  The overall scheme in operator notation thus reads
    
  @f$
    A_{cc} + A_{fc} \circ A_{ff} \circ A_{cf}
  @f$
  
    
  i.e. we evaluate the cell contributions and add the face contributions 
  which in turn results from a concatenation of three operators: 
  Project solution onto face, solve a Riemann problem there, and then 
  take the result and project it back onto the cell. The projection onto
  the face is a purely geoemtric thing: Once we know the polynomials 
  chosen within the cell, we can evaluate what the solution looks like 
  depending on the weights (unknowns within the degree of freedom). 
  With that knowledge, we can determine what the solution on the face 
  looks like. We use the same polynomial space there (restricted to the 
  submanifold) and therefore can express the face representation through
  polynomial weights (left and right of the face), too. For each pair of
  the  polynomial weights, we can solve the Riemann problem and store the
  outcome in further weights. That is, we represent the outcome of the
  Riemann problem again in the polynomial space on the faces. Finally,
  we take this representation, test it against the test functions, 
  integrate over the face and write the contributions back into the cell.
    
    
    
    
  Effectively, our approach blows up the equation system to solve into 
    
  @f$ 
   Ax = 
   \begin{bmatrix}
   A_{cc}  & A_{fc}  & 0 & 0 \\
    0       & id      & -A_{ff}|_+ & -A_{ff}|_- \\
   -id      & 0       & A_{cf}|_+ & 0      \\
   -id      & 0       & 0 & A_{cf}|_-      
   \end{bmatrix}
   \begin{bmatrix}
   x_{c} \\ x_f \\ x^+ \\ x^-
   \end{bmatrix}
   = 
   \begin{bmatrix}
   b_{c} \\ 0 \\ 0 \\ 0 
   \end{bmatrix}
  @f$ 


  @todo We need a nice illustration here of all the function spaces 

  where we have to specify the involved matrices further. 
  The @f$ x_c @f$ are the real degrees of freedom, i.e. the weights within
  the cells which effectively span the solution.
  The @f$ x_\pm @f$ encode the projection of these polynomials onto the faces.
  These are not real degrees of freedom, as they always result directly from
  the @f$ x_c @f$.
  The @f$ x_f @f$ encode the reconstructed solution on the faces and hold the
  average of the @f$ x_\pm @f$ values.
  This setup means our total matrix is no longer square. 
  @f$ A_{cc} @f$ is square, with @f$ |\mathbb{C}| \cdot (p+1)^d @f$ rows, 
  where @f$ |\mathbb{C}| @f$ is the number of cells in the mesh. 
  @f$ A_{fc} @f$ has the same number of rows, but @f$ |\mathbb{F}| \cdot (p+1)^{d-1} @f$
  columns, since this part has to "hit" @f$ x_f @f$. 
  @f$ A_{ff} @f$ has @f$ |\mathbb{F}| \cdot (p+1)^{d-1} @f$ rows, and twice as many
  columns (note that @f$ | x_\pm| \ = \ 2 |x_f| @f$ ).
  Finally, @f$ A_{cf} @f$ has @f$ |\mathbb{C}| \cdot (p+1)^d @f$ rows and 
  @f$ 2|\mathbb{F}| \cdot (p+1)^{d-1} @f$ columns, since this part "hits" 
  @f$ x_\pm @f$.

  Our global matrix is no longer square. We have 
  @f$ 2 |\mathbb{C}| \cdot (p+1)^d + |\mathbb{F}| \cdot (p+1)^{d-1} @f$ rows
  and 
  @f$ |\mathbb{C}| \cdot (p+1)^d + 3|\mathbb{F}| \cdot (p+1)^{d-1} @f$ columns.




  <div style="background-color: #cfc ; padding: 10px; border: 1px solid green;"> 
  The write-down as one big matrix as above works if and only if the 
  chosen Riemann solver yield a linear combination of the @f$ u^-/u^+ @f$
  or @f$ x^-/x^+ @f$ values respectively. In many sophisticated solvers,
  it will not be linear. In this case, the system becomes a non-linear 
  equation system.
  </div>

  ## Data storage
  
  The data storage within a cell is not prescribed by Peano, i.e. you can, 
  in principle decide if you want to reorder the @ref page_multigrid_terminology "nodes within the cell"
  or you can decide to order the degrees of freedom overall as SoA as opposed
  to AoS. At the end of the day, Peano maintains an array of doubles per
  cell and does not really care about the semantics of those. However, we
  strongly recommend that you stick to the default configuration where 
  possible:

  @image html degree_of_freedom_semantics.png

  - Nodes are enumerated following our @ref page_multigrid_terminology "dimension-generic conventions".
  - Data are stored as an Array of Structs (AoS), i.e. first all the unknowns
    of the first node, then all of the second, and so forth.
    
  If you change any ordering, you have to take this into account for all 
  associated operators (such as projections onto the faces), but you also 
  will have to amend the plotters which are tied to these conventions.
  
  Data on the faces is 
  
  - stored as AoS by default;
  - per face, we first store the left projection @f$ x^- @f$, then the right projection @f$ x^+ @f$
    and then the outcome of the flux or associated reconstruction. That is, all the @f$ x^- @f$
    are held en bloc first before we hold @f$ x^+ @f$.
  - Per @f$ x^\pm @f$, the storage of the nodes and unknowns
    follows again the @ref page_multigrid_terminology "dimension-generic conventions".
    
  It is important to recognise that we store three types of quantities 
  per unknown: Its left projection, its right projection, and its 
  outcome which can be either the flux or a reconstructed
  income. The semantics of the outcome depend upon your operators, i.e. 
  they are not baked into the formalism. Furthermore, there is no 
  assumption about the spacing along the faces. Naturally, you might
  assume that a Gauss-Lobatto layout within the cell implies a 
  Gauss-Lobatto distribution on the faces. However, no such thing
  is encoded within the code, as we solely work with operators. They 
  have to encode the correct spacing. 
  
  Another assumptions is baked into the realisation as a default:
  The polynomial degree on the facet result @f$ x_f @f$ equals the 
  polynomial degree within the cell.

  @todo Different polynomial degrees are simple to realise. This is
        something to do in the future. Note that +/- are to be 
        taken from the same space as the cell, as they are projections,
        but maybe we want to alter this, and even provide completely 
        different degrees for the result of the Riemann solve.
        
  
  
  ## Examples
  
  Multiple examples are shipped with Peano which demonstrate how to use the 
  plain Discontiuous Galerking solver:
  
  - benchmarks/multigrid/petsc/poisson/dg.py Hosts a simple solver for the 
    Poisson equation. All the rationale are discussed in @ref benchmarks_documentation_multigrid_petsc_poisson.
    


  """
  def __init__(self,
               name,
               polynomial_degree,
               dimensions,
               cell_unknowns,
               face_unknowns,
               min_h,
               max_h,
               cell_cell_lhs_matrix,
               cell_cell_rhs_matrix,
               cell_cell_lhs_matrix_scaling,
               cell_cell_rhs_matrix_scaling,
               cell_to_face_matrix,
               face_to_cell_matrix,
               cell_to_face_matrix_scaling,
               face_to_cell_matrix_scaling,
               face_face_Riemann_problem_matrix,
               quadrature_points_in_unit_interval,
               gaussian_integration_points,
               gaussian_integration_weights
               ):
    """!
   
 Constructor of DG class
    
 
## Cell to face projection

Consult @ref page_multigrid_terminology "Peano's enumeration for multigrid" page
for info on the ordering. For 2d with a layout as below
   
@image documentation/Doxygen/Multigrid/face-ordering.png
   
the matrix is applied to
   
@f$
   P_{cf} 
   \begin{bmatrix}
   x_0^+ \\ x_1^+ \\ x_2^- \\ x_3^- 
   \end{bmatrix}
   = 
   x_{c}
@f$
 

## Attributes


@param cell_cell_lhs_matrix: [Float] or []                           
  Pass in [] if you prefer to assemble the matrix per cell yourself. This
  is the @f$ \tilde{\mathcal{L}} @f$ part above. The ordering of the 
  degrees of freedom follows the discussion above. The matrix is from
  @f$ K(p+1) \times K(p+1) @f$.
  
@param cell_cell_rhs_matrix: [Float] or []
  Pass in [] if you prefer to assemble the matrix per cell yourself. This 
  matrix arises from @f$ \int f \cdot v \ dx @f$ and is applied to the nodal
  values of the PDE within the quadrature nodes of a cell. The matrix is 
  from @f$ K(p+1) \times K(p+1) @f$. 

@param cell_cell_lhs_matrix_scaling: Positive integer
  The lhs matrix is scaled with @f$ h^x @f$ where the x is defined by this 
  parameter. If you don't specify an lhs matrix, i.e. you prefer to inject
  this matrix manually, you can leave this parameter None, as it is not 
  used.
  
@param cell_cell_rhs_matrix_scaling: Positive integer
  The rhs matrix is scaled with @f$ h^x @f$ where the x is defined by this 
  parameter. If you don't specify a rhs matrix, i.e. you prefer to inject
  this matrix manually, you can leave this parameter None, as it is not 
  used.

@param cell_to_face_matrix: [Float]
  If you study the global equation system above, you recognise that
  this matrix is applied to the left or right solutions of a cell's
  faces. 

@param face_face_Riemann_problem_matrix: [Float] or []
  Pass in [] if you prefer to assemble the matrix per cell yourself. This 
  matrix represents the actual Riemann solver. It accepts the left projection
  @f$ u^- @f$ and @f$ u^+ @f$ and yields a "reconstructed" value at this 
  point passed into the differential operator. Therefore, this is a band
  matrix.

   
    """
    super( DiscontinuousGalerkinDiscretisation, self ).__init__( name,
                                                                 min_h,
                                                                 max_h
                                                                 )

    self.polynomial_degree = polynomial_degree
    self.dimensions        = dimensions
    self.cell_unknowns     = cell_unknowns
    self.face_unknowns     = face_unknowns

    self.cell_cell_lhs_matrix               = cell_cell_lhs_matrix
    self.cell_cell_rhs_matrix               = cell_cell_rhs_matrix
    self.cell_to_face_matrix                = cell_to_face_matrix
    self.face_to_cell_matrix                = face_to_cell_matrix
    self.face_face_Riemann_problem_matrix   = face_face_Riemann_problem_matrix

    self.cell_cell_lhs_matrix_scaling = cell_cell_lhs_matrix_scaling
    self.cell_cell_rhs_matrix_scaling = cell_cell_rhs_matrix_scaling
    self.cell_to_face_matrix_scaling  = cell_to_face_matrix_scaling
    self.face_to_cell_matrix_scaling  = face_to_cell_matrix_scaling
    
    self.quadrature_points_in_unit_interval = quadrature_points_in_unit_interval
    self.gaussian_integration_points        = gaussian_integration_points
    self.gaussian_integration_weights       = gaussian_integration_weights

    assert( len( quadrature_points_in_unit_interval ) == len( gaussian_integration_points ) == len( gaussian_integration_weights ), "Something has gone wrong. These should all be same length for fixed polynomial degree" )
    
    self._cell_pde_data   = peano4.datamodel.DaStGen2( name )
    self._cell_pde_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",    str(self.number_of_matrix_entries_per_cell) ) )
    self._cell_pde_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",      str(self.number_of_matrix_entries_per_cell) ) )
    self._cell_pde_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "exactSol", str(self.number_of_matrix_entries_per_cell) ) )
    
    pass


  @property
  def nodes_per_cell(self):
    return (self.polynomial_degree + 1) ** self.dimensions


  @property
  def nodes_per_face(self):
    return (self.polynomial_degree + 1) ** (self.dimensions-1)


  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    """!
    
    Initialise index model 
    
    There is an index data model, where all the vertices, faces and cells
    are properly enumerated. This one is taken care of by the superclass.
    The thing we have to add our our PDE-specific unknowns.
    
    """
    super( DiscontinuousGalerkinDiscretisation, self ).add_to_Peano4_datamodel(datamodel,verbose)
    datamodel.add_cell(self._cell_pde_data)


  def add_use_statements(self, observer):
    super( DiscontinuousGalerkinDiscretisation, self ).add_use_statements(observer)
    observer.use_cell(self._cell_pde_data)
    pass


  def add_to_plot(self, observer):
    """!
    
    Tell the project's observer how to plot the data
    
    Nothing fancy here. We add plotters from Peano's toolbox to visualise
    solution and right-hand side. 
    
    """
    observer.add_action_set( PlotDGDataInPeanoBlockFormat(self) )
    pass


  def add_to_create_grid(self, observer):
    observer.add_action_set(peano4.toolbox.CreateRegularGrid(self.max_h))
    pass

  
  def add_to_enumerate_and_init_solution(self, observer):
    """!
    
    Solution initialisation
    
    Close to trivial: Just add the action set petsc.actionsets.InitVertexDoFs
    to the observer.
    
    """
    observer.add_action_set( EnumerateDoFs(self,
                                           boundary_vertices_hold_data = False,
                                           boundary_faces_hold_data = True,
                                           ))
    
    
    observer.add_action_set( InitCellDoFs(self) ) 
    pass
  
  def add_to_assemble(self, observer):
    observer.add_action_set( AssemblePetscMatrix(self) )
    observer.add_action_set( ImposeDirichletBoundaryConditionsWithInteriorPenaltyMethod(self) )
    pass

  def add_to_init_petsc(self, observer):
    pass

  def add_to_map_solution_onto_mesh(self, observer):
    observer.add_action_set( ProjectPETScSolutionOnCellsBackOntoMesh(self) )
    observer.add_action_set( PlotExactSolution(self) )
    pass
    
  def add_implementation_files_to_project(self, namespace, output):
    """
    
    The solver creates two classes: An abstract base class and its 
    implementations. The abstract base class will be overwritten if
    there is one in the directory. I pipe all the Python constants 
    into this base class, so they are available in the C++ code. 
    
    The implementation file will not be overwritten, as I assume that
    the users will write their own domain-specific stuff in there. If
    it does not exist yet, I'll create an empty stub which can be 
    befilled with something meaningful.
    
    Besides the creation of these two files, I also add the files to the
    project artifacts and the makefile, so they are captured by the build
    system.
    
    """
    templatefile_prefix  = os.path.dirname( os.path.realpath(__file__) ) + "/"
    templatefile_prefix += self.__class__.__name__

    dictionary = {
        "SOLVER_INCLUDES": "",
        "MIN_H"             : self.min_h,
        "MAX_H"             : self.max_h,
        "POLYNOMIAL_DEGREE":  self.polynomial_degree,
        "CELL_UNKNOWNS":      self.cell_unknowns,
        "FACE_UNKNOWNS":      self.face_unknowns,
        "CELL_CELL_LHS_MATRIX":      self.cell_cell_lhs_matrix,
        "CELL_CELL_RHS_MATRIX":      self.cell_cell_rhs_matrix,
        "FACE_FACE_RIEMANN_PROBLEM_MATRIX":      self.face_face_Riemann_problem_matrix,
        "CELL_TO_FACE_MATRIX":       self.cell_to_face_matrix,
        "FACE_TO_CELL_MATRIX":                   self.face_to_cell_matrix,
        "QUADRATURE_POINTS_IN_UNIT_INTERVAL":    self.quadrature_points_in_unit_interval,
        "GAUSSIAN_INTEGRATION_POINTS"       :    self.gaussian_integration_points,
        "GAUSSIAN_INTEGRATION_WEIGHTS"       :    self.gaussian_integration_weights,
        "CELL_CELL_LHS_MATRIX_SCALING": self.cell_cell_lhs_matrix_scaling,
        "CELL_CELL_RHS_MATRIX_SCALING": self.cell_cell_rhs_matrix_scaling,
        "CELL_TO_FACE_MATRIX_SCALING":           self.cell_to_face_matrix_scaling,
        "FACE_TO_CELL_MATRIX_SCALING":           self.face_to_cell_matrix_scaling,
        "SOLVER_NAME":                           self.typename(),
        }

    generated_abstract_header_file = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".Abstract.template.h",
      templatefile_prefix + ".Abstract.template.cpp",
      "Abstract" + self.typename(), 
      namespace,
      ".", 
      dictionary,
      True,
      True)
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".template.h",
      templatefile_prefix + ".template.cpp",
      self.typename(), 
      namespace,
      ".", 
      dictionary,
      False,
      True)

    output.add( generated_abstract_header_file )
    output.add( generated_solver_files )
    output.makefile.add_h_file( "Abstract" + self._name + ".h", generated=True )
    output.makefile.add_h_file( self._name + ".h", generated=True )
    output.makefile.add_cpp_file( "Abstract" + self._name + ".cpp", generated=True )
    output.makefile.add_cpp_file( self._name + ".cpp", generated=True )


  @property
  def number_of_matrix_entries_per_vertex(self):
    """!
    
    Nothing is associated with the vertex. There's nothing to be done here.
    
    """
    return 0
  
  @property
  def number_of_matrix_entries_per_face(self):
    """!
    
    In the DG scheme, we have a projection from the left and we have a 
    projection from the right. These values are proper projections, i.e.
    they do not carry any semantics. Then we have to solve the Riemann
    problem, and need one more unknown to store the outcome of that one.
    
    """
    return 3 * self.face_unknowns * self.nodes_per_face
  
  @property
  def number_of_matrix_entries_per_cell(self):
    """!
    
    This is the actual unknowns per cell
    
    """
    return self.cell_unknowns * self.nodes_per_cell
  