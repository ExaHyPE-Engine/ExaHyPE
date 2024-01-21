# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2
import os

from .Solver import Solver
from .actionsets.InitDofs         import InitDofsDG
from .actionsets.UpdateCell       import UpdateCell
from .actionsets.ProjectOntoFaces import ProjectOntoFaces

class DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver(Solver):
  """!
  \f{eqnarray*}{\f}

  Discontinuous Galerkin Solver

  This solver holds \f$ K^c (p+1)^d \f$ unknowns (doubles) per cell for the
  solution, \f$  K^c (p+1)^d \f$ per cell for the right-hand side.
  We also hold \f$ ( K^f + 2K^c ) (p+1)^{d-1} \f$ unknowns per face, ie 
  for each node, we hold one solution for each unknown per face node, as 
  well as a projection from the left and right for each unknown per cell.
  \f$ K^c \f$ and \f$ K^f \f$ are different in general.

  The internal functionality relies on the break-down of the per-cell
  operations into 

  \f$ A u|_c - P \mathcal{R}(u^-,u^+) = A u|_c - P \mathcal{R}(u|_f) = A u|_c - P \varphi|_f = Mf \f$  

  and it realises a Block-Jacobi smoother subject to two relaxation 
  parameters \f$ \omega _c, \omega _f \in [0,1] \f$. Conceptually,
  the solver starts from the representation

  \f$
  \left(
    \begin{array}{ccccc}
      A &   &   &   & ... \\
        & A &   &   & ... \\
        &   & A &   & ... \\
        &   &   & A & ... \\
      . & . & . & . & \hat A
    \end{array}
  \right)
  \f$

  of the overall left-hand side above. Here \f$ A \f$ acts on the individual 
  cell, and the block entries with a dot couple the cell to the faces and vice
  versa. We extract the matrix 

  \f$
  \left(
    \begin{array}{ccccc}
      A &   &   &   &  \\
        & A &   &   &  \\
        &   & A &   &  \\
        &   &   & A &  \\
        &   &   &   & diag( \hat A )
    \end{array}
  \right)
  \f$

  from this system matrix, and use this one within an (outer) Jacobi scheme.
  This yields a block-Jacobi solver, where individual cells (matrix blocks)
  are explicitly inverted.

  We end up with an iterative scheme where two phases take turns:

  - Per cell, we solve the equation
    \f$ 
    A u|_c + P q|_f = M b
    \f$ 
    which yields the update rule
    \f{eqnarray*}{ 
    u|_c & \gets & u|_c + \omega _c A^{-1}\left( Mb - A u|_c - P q|_f \right) \\
        &   =   & (1-\omega _c) u|_c + \omega _c A^{-1}\left( Mb - P q|_f \right)
    \f}
    Here, the \f$ q \f$ is the solution of the PDE term on the faces which we
    assume to be fixed. Obviously, \f$ \omega _c > 0 \f$ whilst \f$ \omega _c=1 \f$
    solves the problem per cell exactly (and might still be a bad smoother).
  - Per face, we use a plain Jacobi. We assume that we have an explicit
    expression of the Riemann solution (simple averaging would be an example)
    and hence \f$ \hat A = id \f$. We consequently get
    \f{eqnarray*}{ 
      q|_f & \gets & q|_f + \omega _f \left( \mathcal{R}(u^-, u^+) - q|_f \right) \\
          & = & (1-\omega _f) q|_f + \omega _f \mathcal{R}(u^-, u^+).
    \f}

    

  ## Algorithmical blueprint

  1. Enter cell: Whenever a cell is entered, we create a temporary 
    \f$ N(p+1)^d \f$ array (on the heap). 
    
    1. Into this temporary array
        \f$ r \f$, we write \f$ Mb \f$. This is the mass matrix (right-hand side
        matrix of the class constructor, as we are not restricted to a 
        traditional mass matrix) times the right-hand sides within the 
        cell. So
        \f$
        r \gets Mb.
        \f$
    2. Once complete, we substract \f$ Pq|_f \f$. That is, we take the 
        solution on the face and we project it (cell from face) onto the 
        cell, where we substract it from the previous \f$ r \f$ value:
        \f$
        r \gets Mb + Pq|_f.
        \f$
    3. The third step is where the MFlops come from: We take the cell
        matrix and explicitly invert it. For a linear, invariant matrix
        \f$ A \f$, we could do so once, but the solver itself makes no
        assumption if the matrix is invariant or not. So it inverts it
        every step and uses BLAS routines or this. The result is multiplied
        with the residual as accumulated so far:
        \f$
        r \gets A^{-1} \left( Mb + Pq|_f \right).
        \f$
    4. We run over the solution within the cell and scale each entry 
        with \f$ (1-\omega _c) \f$. If \f$ \omega _c =1 \f$, this means we
        set the entries to zero. In general, the entries however will 
        only be damped. After that, we add the \f$ r \f$ values to each
        point. This gives us a new solution guess.
    5. We project the updated solution \f$ u|_c \f$ onto the faces, i.e.
        obtain a new \f$ u|_f^{\pm } \f$. This is realised within the 
        action set ProjectOntoFaces.
  2. touchFaceFirstTime(): Whenever a face is touched, we solve the Riemann
    problem and update the Riemann guess accordingly. Initially, we assume
    that the \f$ q \f$ entries on the face are all set to zero.
    
    1. We evaluate per node on the face the expression \f$ \mathcal{R}(u^-,u^+) \f$.
        For this, we loop over the \f$ (p+1)^{d-1} \f$ points and call the 
        C++ solver's routine. The user can thus implement the actual 
        Riemann solver themself.
    2. We take the result and update the value on the face according to
        \f$
        q|_f \gets (1-\omega _f) q|_f + \omega _f \mathcal{R}(u^-, u^+).
        \f$

        
    
    All of this functionality can be found in the subpackage actionsets.UpdateCell.

  ## How does it work?

  ### Python stuff
  The benchmarks folder contains a dg.py file. In there, we define the simulation paramters,
  the Peano project, the solver and the various matrices we pass in.

  As always, the number of nodes per face/cell is fixed by the polynomial degree and the 
  dimension of the space.

  Some important parameters:

  \li ```poly_degree``` - The degree of the polynomial that we use. Defaults to 1.

  \li ```unknowns_per_cell``` - The number of unknowns that we solve for in each cell, \f$ K^c \f$

  \li ```unknowns_per_face``` - The number of unknowns that we solve for in each face, \f$ K^f \f$. 
  It's not clear whether it makes sense to have this as a different value from ```unknowns_per_cell```, 
  since our update rule \f$ u|_c \gets u|_c + \omega _c A^{-1}\left( Mb - A u|_c - P q|_f \right) \f$ implies
  that there are the same number of \f$ u|_c \f$ as \f$ q|_f \f$ at coinciding nodal points.

  ### Under the hood

  Inside the cells we store \f$ K^c (p+1)^{d} \f$ values for the solution, and the same number for the rhs.
  On each face we store \f$ K^f (p+1)^{d-1} \f$ (although, at time of writing I fix \f$K^f==K^c\f$) values
  for the solution, and then \f$ 2K^c (p+1)^{d-1} \f$ for the projections in a separate array. Since this is
  a projection of \f$ u|_c \f$ onto the face, we have one for each face nodal point, with the factor of two
  since we have one on the left and one on the right.


  ### Matrices we pass in to the solver class
  See documentation for this class itself...

  \li ```assembly_matrix``` - this is the cell-to-cell matrix which defines how the cell nodes couple to one another.
  This is a square matrix with \f$ K^c * (p+1)^{d} \f$ rows.

  \li ```mass_matrix``` - similar to ```assembly_matrix```.

  \li ```face_from_cell``` - this matrix projects the cell solution onto all of the faces. It has \f$ 2K^c (p+1)^{d-1} * 2D \f$
  rows (note the extra factor of 2D, the number of faces),
   ie, one row for each element of the ```projection``` array, for each face, with the entries for each face listed one 
  after another. The matrix has \f$ K^c * (p+1)^{d} \f$ columns, ie one row for each face-solution. It is the job of the user
  to ensure that this matrix performs the correct ordering in the desired manner - more on this later.

  \li ```cell_from_face``` - this matrix projects the solution on all of the faces onto the cell. It hence has 
  \f$ 2K^f (p+1)^{d-1} * 2D \f$ columns (note the extra factor of 2D) and \f$ K^c * (p+1)^{d} \f$. The user should
  enumerate these correctly.

  \li ```riemann_matrix``` - The matrix that combines the left/right projections on the cell and outputs a value 
  which we use to update the \f$ q\_f \f$. It has \f$ 2K^c (p+1)^{d-1} \f$ columns and \f$ 2K^f (p+1)^{d-1} \f$ rows.
  @todo I think this is another reason why K^c cannot be different from K^f....

  So far we have three non-trivial action sets.

  ###  InitDofs
  Code for this is in actionsets.InitDofsDG. So far nothing happens, other than determining which cells/faces are 
  on the boundary and putting in initial guesses for each cell and face dof.

  ### UpdateCell
  This is called before ProjectOntoFaces. Code for this is in actionsets.UpdateCell.
  @todo - include code snippets here

  ### ProjectOntoFaces
  Code for this is in actionsets.ProjectOntoFaces
  @todo - include code snippets here

  ## Solver class
  For now, the solver class contains the matrices mentioned above. We also include a method that explicitly
  inverts a matrix using BLAS, in two stages:

  \verbatim
#include <cblas.h>
#include <lapacke.h>

...

  // make copy of current Assembly matrix
  tarch::la::Matrix< CellUnknowns, CellUnknowns, double > M = AssemblyMatrix;

  // aux variables
  int returnCode;
  int ipiv[CellUnknowns];

  // step 1 - compute LU factorisation. M -> P * L * U, where P a permutation matrix
  returnCode = LAPACKE_dgetrf( LAPACK_COL_MAJOR, CellUnknowns, CellUnknowns, M.data(), CellUnknowns, ipiv );
  // check return code
  assertion(returnCode>=0);

  // step 2 - use the result of the above to compute the inverse.
  returnCode = LAPACKE_dgetri( LAPACK_COL_MAJOR, CellUnknowns, M.data(), CellUnknowns, ipiv );
 // check return code
  assertion(returnCode>=0);

  return M;

  \endverbatim

  Here we neatly (ab)use the tarch::la::Matrix::data() function to get a pointer to a double array.
  See documentation on these functions [here](https://www.netlib.org/lapack/explore-html-3.6.1/dd/d9a/group__double_g_ecomputational_ga0019443faea08275ca60a734d0593e60.html)
  and [here](https://www.netlib.org/lapack/explore-html-3.6.1/dd/d9a/group__double_g_ecomputational_ga56d9c860ce4ce42ded7f914fdb0683ff.html)

  ## How do I build it on COSMA?

  Modules to load:
    \li module load oneAPI/2022.3.0
    \li module load gnu_comp/10.2.0 gsl
    \li module load intel_mpi
    \li module load openblas

    Make sure LD_LIBRARY_PATH contains the openblas directory from the intel compiler files:
      ```export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:/cosma/local/openblas/intel_2020-update2/0.3.12cosma7/lib```
  
  Stuff to add to CXXFLAGS:
  \li -I/cosma/local/openblas/intel_2020-update2/0.3.12cosma7/include/

  Stuff to add to LDFLAGS:
  \li -L/cosma/local/openblas/intel_2020-update2/0.3.12cosma7/lib/
  \li -lopenblas
    
  # Questions to be answered:
  - What to do on boundary cells/faces?

    
  """
  def __init__(self,
              name,
              dimensions,
              poly_degree,
              unknowns_per_cell_node,
              unknowns_per_face_node,
              min_h,
              max_h,
              assembly_matrix,
              assembly_matrix_scaling,
              mass_matrix,
              mass_matrix_scaling,
              face_from_cell_projection,
              cell_from_face_projection,
              riemann_matrix,
              ):
    super( DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver, self ).__init__(name, min_h, max_h)

    # store everything we pass in anyhow
    self._dimensions                = dimensions
    self._poly_degree               = poly_degree
    self._unknowns_per_cell_node    = unknowns_per_cell_node
    self._unknowns_per_face_node    = unknowns_per_face_node
    self._assembly_matrix           = assembly_matrix
    self._assembly_matrix_scaling   = assembly_matrix_scaling
    self._mass_matrix               = mass_matrix
    self._mass_matrix_scaling       = mass_matrix_scaling
    self._face_from_cell_projection = face_from_cell_projection
    self._cell_from_face_projection = cell_from_face_projection
    self._riemann_matrix            = riemann_matrix

    # K^c * (p+1)^d
    self._unknowns_per_cell = unknowns_per_cell_node * ( poly_degree + 1) ** dimensions
    #(K^f +  2K^c) * (p+1)^(d-1)
    self._unknowns_per_face_solution   = (    unknowns_per_face_node) * ( poly_degree + 1) ** (dimensions-1)

    # to each node we leave one copy of each of the cell unknowns from left and right
    self._unknowns_per_face_projection = (2 * unknowns_per_cell_node) * ( poly_degree + 1) ** (dimensions-1)

    # ignore vertex data

    # cell stuff
    self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "solution", str(self._unknowns_per_cell) ))
    self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "rhs",      str(self._unknowns_per_cell) ))

    # face stuff
    self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "solution",      str(self._unknowns_per_face_solution) ))
    self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "projection",    str(self._unknowns_per_face_projection) ))

    # assertions for matrix shapes
    assert mass_matrix.shape     == (self._unknowns_per_cell, self._unknowns_per_cell)
    assert assembly_matrix.shape == mass_matrix.shape

    # assert  face_from_cell_projection.shape    == (self._unknowns_per_face_projection * 2*self._dimensions, self._unknowns_per_cell)
    # assert (face_from_cell_projection.T).shape ==  self._cell_from_face_projection.shape

  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    super( DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver, self ).add_to_Peano4_datamodel(datamodel,verbose)

  def add_use_statements(self, observer):
    super( DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver, self ).add_use_statements(observer)

  def add_to_plot(self, observer):
    """!
    
    Tell the project's observer how to plot the data
    
    Nothing fancy here. We add plotters from Peano's toolbox to visualise
    solution and right-hand side. 
    
    """
    pass

  def add_to_create_grid(self, observer):
    observer.add_action_set(peano4.toolbox.CreateRegularGrid(self.max_h))
    pass

  def add_to_init_solution(self, observer):
    """!
    
    Solution initialisation
    
    """
    observer.add_action_set( InitDofsDG(self) ) 
    pass

  def add_to_solve(self, observer):
    # 1st priority, run sequentially
    observer.add_action_set( UpdateCell(self, 0, False) ) 
    # 2nd priority, run sequentially
    observer.add_action_set( ProjectOntoFaces(self, 1, False) ) 
    pass
  
  def add_implementation_files_to_project(self, namespace, output):
    """
    
    The solver creates two classes: An abstract base class and its 
    implementations. The abstract base class will be overwritten if
    there is one in the directory. I pipe all the Python constants 
    into this base class, so they are available in the C++ code. 
    
    """
    templatefile_prefix  = os.path.dirname( os.path.realpath(__file__) ) + "/"
    templatefile_prefix += self.__class__.__name__

    dictionary = {
        "SOLVER_INCLUDES": "",
        "MIN_H": self.min_h,
        "MAX_H": self.max_h,
        "SOLVER_NAME":             self.typename(),
        "CELL_CARDINALITY"            : self._unknowns_per_cell,
        "FACE_CARDINALITY_SOLUTION"   : self._unknowns_per_face_solution,
        "FACE_CARDINALITY_PROJECTION" : self._unknowns_per_face_projection,
        "VERTEX_CARDINALITY"          : 1,
        "NODES_PER_CELL"              : (self._poly_degree + 1) **  self._dimensions,
        "NODES_PER_FACE"              : (self._poly_degree + 1) ** (self._dimensions-1),
        "UNKNOWNS_PER_CELL_NODE"      : self._unknowns_per_cell_node,
        "UNKNOWNS_PER_FACE_NODE"      : self._unknowns_per_face_node,
        "MASS_MATRIX"                 : self._mass_matrix,
        "MASS_MATRIX_SCALING"         : self._mass_matrix_scaling,
        "ASSEMBLY_MATRIX"             : self._assembly_matrix,
        "ASSEMBLY_MATRIX_SCALING"     : self._assembly_matrix_scaling,
        "FACE_FROM_CELL_PROJECTION"   : self._face_from_cell_projection,
        "CELL_FROM_FACE_PROJECTION"   : self._cell_from_face_projection,
        "RIEMANN_MATRIX"              : self._riemann_matrix
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




    