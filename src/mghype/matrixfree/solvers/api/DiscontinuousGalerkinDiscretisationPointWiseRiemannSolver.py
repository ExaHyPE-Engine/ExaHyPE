""" 
 This file is part of the MGHyPE project within Peano 4. For conditions of distribution and 
 use, please see the copyright notice at www.peano-framework.org
"""
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2
import os
import jinja2

from .Solver import Solver

from .actionsets.InitHigherOrderDofs                           import InitHigherOrderDofs
from .actionsets.DGSolver.UpdateResidual                       import UpdateResidual
from .actionsets.DGSolver.ProjectOntoFaces                     import ProjectOntoFaces
from .actionsets.DGSolver.UpdateFaceSolution                   import UpdateFaceSolution
from .actionsets.DGSolver.ProjectIntoCellAndUpdateCellSolution import ProjectIntoCellAndUpdateCellSolution
from .actionsets.PlotDGDataInPeanoBlockFormat                  import PlotDGDataInPeanoBlockFormat


class DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver(Solver):
  """!

  Most efficient solver we have for this type of problem class

  You can find each of the action sets here:
  solvers.api.actionsets.DGSolver.InitHigherOrderDofs.InitHigherOrderDofs

  The text below specifies the semantics and behaviour of this matrix-free
  DG solver. A tutorial running step by step through the steps how to use
  it can be found @ref tutorials_multigrid_matrixfree_discontinuous_galerkin_poisson "in the tutorials section".
  The solver in general tackles a PDE

    @f$ \mathcal{L}(u) = f @f$
    
  resulting in a linear equation system

    @f$ Au = b. @f$
    
  Throughout the discussion, we @ref page_multigrid_unfolding "unfold the compute steps over the compute mesh".

  ## Storage 
  
  This solver holds \f$ K^c (p+1)^d \f$ unknowns (doubles) per cell for the
  solution, \f$  K^c (p+1)^d \f$ per cell for the right-hand side, and the 
  same number for the residual. Holding the residual in the cell allows us
  to separate some algorithm steps into different action sets. In particular,
  we project the face solution back into the cells after updating the cell
  solution (in separate sweeps).
  These are the primary variables, i.e. those directly resulting from the
  PDE formulation. The correspond to the unknowns directly from @f$ u @f$
  and the right-hand side.
  
  Further to that, we hold auxiliary variables on the faces: 
  Our Riemann solver accepts the left and the right solution. Here, each
  degree of freedom has @f$ K^f_{f \gets c} @f$ unknowns. It will return a result 
  (the flux), which has @f$ K^f @f$ unknowns.
  Per face node, we hence hold, in total, \f$ ( K^f + 2K^f_{f \gets c} ) (p+1)^{d-1} \f$ unknowns. 
  That is the solution for each unknown per face node, as 
  well as a projection from the left and right for each unknown per cell.
  \f$ K^c \f$ and \f$ K^f \f$ can be different, but often they are the same.
  The user can choose how many of the \f$ K^c \f$ to project onto the face
  by modifying \f$ K^f_{f \gets c} \f$.

  ## Operators 
  
  With the auxiliary variables, in place, we can write down the DG 
  problem as 
  
  \f$
  \left(
    \begin{array}{cccc}
      A^{cc} &   &   & P_{c \gets f} \\
      P_{f \gets c} & id & & \\
      P_{f \gets c} & & id  \\
      & id & id & A^{ff}
    \end{array}
  \right)
  \left(
    \begin{array}{c}
      u^c \\
      u^+ \\
      u^- \\
      q
    \end{array}
  \right)
  =
  \left(
    \begin{array}{c}
      M b \\
      0 \\
      0 \\
      0
    \end{array}
  \right)
  \f$
  
  in line with @ref xxxx. The @f$ q @f$ is the result of the Riemann solver.
  
  ## Solver 
  
  We now realise a Block-Jacobi smoother subject to two relaxation parameters 
  \f$ \omega _c, \omega _f \in [0,1] \f$. We tackle the first line in the 
  block system with one smoothing step of a stationary iterative scheme, solve two and three exact (as they are
  trivial with the identity on the diagonal) and apply Jacobi to the last
  row again. As the first and last block are updated in parallel by the 
  Jacobi scheme, the resuling algorithm is globally a Jacobi relaxation, too.

  - Per cell, we tackle the equation
    \f$ 
    A^{cc} u^c + P_{c \gets f} q = M b
    \f$ 
    which yields the update rule
    \f{eqnarray*}{ 
    u|_c & \gets & u|_c + \omega _c \tilde {A}^{-1}\left( Mb - A^{cc} u^c - P_{c \gets f} q \right).
    \f}
    Here, the \f$ q \f$ is the solution of the PDE term on the faces which we
    assume to be fixed.  At this point, we are quite generic and dump the 
    responsibility onto the user to give us an approximation of @f$ A_{cc} @f$ 
    that we can invert. Notably, we highlight that the user may give us
     @f$ \tilde{A} = A^{cc} @f$ in a DG sense, as we cannot invert @f$ A^{cc} @f$.
  - Per face, we use a plain Jacobi. We assume that we have an explicit
    expression of the Riemann solution (simple averaging would be an example)
    and hence \f$ \hat A = id \f$. We consequently get
    \f{eqnarray*}{ 
      q|_f & \gets & q|_f + \omega _f \left( \mathcal{R}(u^-, u^+) - q|_f \right) \\
          & = & (1-\omega _f) q|_f + \omega _f \mathcal{R}(u^-, u^+).
    \f}

    

  ## Algorithmical blueprint

  1. Enter cell: Whenever a cell is entered, we create a temporary 
    \f$ K^c(p+1)^d \f$ array (on the heap). 
    
    1. Into this temporary array
        \f$ r \f$, we write \f$ Mb \f$. This is the mass matrix (right-hand side
        matrix of the class constructor, as we are not restricted to a 
        traditional mass matrix) times the right-hand sides within the 
        cell. So
        \f$
        r \gets Mb.
        \f$
    2. Once complete, we substract \f$ P^{c \gets f}  q|_f \f$. That is, we take the 
        solution on the face and we project it (cell from face) onto the 
        cell, where we substract it from the previous \f$ r \f$ value. We also
        do the same with the cell-to-cell couplings. So:
        \f$
        r \gets Mb - Pq|_f -A^{cc}u^c.
        \f$
      We also do the same with the cell couplings
    3. The third step is where the MFlops come from: We take the user's
       approximation of the cell
        matrix and explicitly invert it. For a linear, invariant matrix
        \f$ A \f$, we could do so once, but the solver itself makes no
        assumption if the matrix is invariant or not. So it inverts it
        every step and uses BLAS routines or this. The result is multiplied
        with the residual as accumulated so far:
        \f$
        r \gets \tilde{A}^{-1} \left( Mb - Pq|_f -A^{cc}u^c \right).
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

        
    
    All of this functionality can be found in the subpackage actionsets.UpdateResidual.

  ## How does it work?

  ### Python stuff
  
  There is a @ref tutorials_multigrid_matrixfree_discontinuous_galerkin_poisson "very simple Poisson solver tutorial"
  in the tutorials section which illustrates how to use this solver.

  The user needs to supply a few crucial matrices, referred to in the maths above. These are noted and their
  shapes discussed in the parameters section of the documentation of this class.

  ### Under the hood
  
  Inside the cells we store \f$ K^c (p+1)^{d} \f$ values for the solution, and the same number for the rhs
  and residual. We store the residual in the cell so that we can calculate the update to the cell solution
  separately from the projection of face solution back into the cells, but crucially are able to update
  the global residual before we apply the preconditioner.
  On each face we store \f$ K^f (p+1)^{d-1} \f$ values
  for the solution, and then \f$ 2K^f_{f \gets c} (p+1)^{d-1} \f$ for the projections in a separate array.
  We note that this factor of two comes from the fact that we have projections from cells on both the left 
  and right. The user is free to choose how many variables from the cell they want to project onto the face.
  The value K^f_{f \gets c} is exposed in dg.py as the argument ``` --projections_per_face" ```.

  ### Matrices we pass in to the solver class
  See documentation for this class itself...


  So far we have three non-trivial action sets.

  ###  InitHigherOrderDofs
  Code for this is in actionsets.InitHigherOrderDofs. So far nothing happens, other than determining which cells/faces are 
  on the boundary and putting in initial guesses for each cell and face dof. We pass over to the implementation
  cpp file which should be left in situ in the directory for the application.

  ### UpdateFaceSolution
  Code for this is in actionsets.DGSolver.UpdateFaceSolution. This action set only contains something nontrivial at 
  touchFaceFirstTime. We apply the Riemann matrix that is present in the Abstract Solver class. This action set goes first,
  the idea being that we updated the face solution using the updated face projections from the previous sweep.

  ### ProjectIntoCellAndUpdateCellSolution
  Code for this is in actionsets.DGSolver.ProjectIntoCellAndUpdateCellSolution. This action set works when we touch the cell for the first time.
  We project the newly updated face solution back into the cells. In particular we:
   - Write the updated face solution into the residual stored in the cell. This finishes the cell update from the previous sweep.
   - Update the global residual
   - Update the cell solution, which is projected back onto faces in the next action set.

  ### ProjectOntoFaces
  Code for this is in actionsets.DGSolver.ProjectOntoFaces. This occurs strictly after ProjectIntoCellAndUpdateCellSolution, so that we project
  freshly updated Cell values onto the Faces.

  ### UpdateResidual
  Code for this is in actionsets.DGSolver.UpdateResidual. We do something nontrivial in touchCellFirstTime, namely perform the cell update
  rule as above. We set the residual to be \f$ Mb - A^{cc} u^c \f$.

  
  @param solver_includes C++ code snippet
    Arbitrary C++ code snippet. Usually used to include additional files 
    (therefore the name), though you can use it for preprocessing macros
    or using statements as well. Has to be a valid C++ expression, i.e. 
    something similar to
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
#include "myfile.h"
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    
  """
  def __init__(self,
              name,
              dimensions,
              poly_degree,
              unknowns_per_cell_node,
              solutions_per_face_node,
              projections_per_face_node,
              min_h,
              max_h,
              assembly_matrix,
              assembly_matrix_scaling,
              mass_matrix,
              mass_matrix_scaling,
              face_from_cell_projection,
              face_from_cell_projection_scaling,
              cell_from_face_projection,
              cell_from_face_projection_scaling,
              riemann_matrix,
              boundary_matrix,
              cell_relaxation,
              face_relaxation,
              approximate_system_matrix,
              approximate_system_matrix_scaling,
              integration_points,
              solver_tolerance,
              custom_init_actionset = None,
              custom_plot_actionset = None, # just pass in a list of tuples, one for the variable name and the other for the getter
              custom_init_cell = "",
              enable_plotting = True, # sometimes we want to disable standard plotting...,
              plot_predicate = "true", # enable custom plotting behaviour
              solver_includes = "",
              use_smart_pointer = True
              ):
    """!
    
    Construct the solver
    
    @param poly_degree: Integer
      The degree of the polynomial that we use. Has to be bigger or equal to one.

    @param unknowns_per_cell_node: Positive Integer
      This is the @f$ K^c @f$ from the solver description.
       
    @param solutions_per_face_node: Positive Integer
      This is the @f$ K^f @f$ from the solver description.

    @param projections_per_face_node: Positive Integer
      This is the @f$ K^f_{f \gets c} @f$ from the solver description.

    @param cell_relaxation: Float   
      This is the @f$ \omega _c @f$ from the description above and typically 
      chosen form @f$ ]0,1[ @f$ though some scenarios benefit from a slight 
      overrelaxation.

    @param assembly_matrix: \f$ \mathbb{R}^{K^c * (p+1)^{d} \times K^c * (p+1)^{d}}\f$ 
      This is the cell-to-cell matrix which defines how the cell nodes couple 
      to one another. Is is a square matrix. You can pass in None. In this 
      case, the generated C++ code will not compile, but you will be allowed
      to fill it in manually.

    @param assembly_matrix_scaling: \f$ \mathbb{R}^{K^c * (p+1)^{d} \times K^c * (p+1)^{d}}\f$ 
      This is a matrix of ints that prescribes how we scale each element in the assembly_matrix
      ie for each i,j, we do: assembly_matrix[i,j] *= ( h )^( assembly_matrix_scaling[i,j] )


    @param mass_matrix: \f$ \mathbb{R}^{K^c * (p+1)^{d} \times K^c * (p+1)^{d}}\f$ 
    
    @param mass_matrix_scaling: \f$ \mathbb{R}^{K^c * (p+1)^{d} \times K^c * (p+1)^{d}}\f$ 
      This is a matrix of ints that prescribes how we scale each element in the mass_matrix
      ie for each i,j, we do: mass_matrix[i,j] *= ( h )^( mass_matrix_scaling[i,j] )

    @param face_from_cell: \f$ \mathbb{R}^{2d * 2K^f_{f \gets c} * (p+1)^{d-1} \times K^c * (p+1)^{d}}\f$ 
      This matrix projects the cell solution onto all of the faces. Note the 
      extra factor of @f$ 2d @f$. The structure of this matrix is quite important. The rows
      should be structured in blocks of \f$ 2K^f_{f \gets c} * (p+1)^{d-1} \f$, ie one for each 
      face, in the familiar Peano ordering. We note that this is the same as the number of projections
      we store per face. We note that, since the 0th face has left-side projections which
      should be set outside of the cell, that at least half of the rows of the matrix should
      be empty.
      We also note that each face contains multiple nodes, and they should be ordered sequentially
      in the typical Peano fashion, i.e. greedily along each coordinate axis until full, starting
      from the negative. For each face, with two nodes, with two values (q and u) being projected onto the face,
      the rows should be ordered as follows:
        1. Negative projection of q onto node 0
        2. Negative projection of q onto node 1
        3. Negative projection of u onto node 0
        4. Negative projection of u onto node 1
        5. Positive projection of q onto node 0
        6. Positive projection of q onto node 1
        7. Positive projection of u onto node 0
        8. Positive projection of u onto node 1
      In two dimensions, all of the negative projections should be 0 on faces 0 and 1. Ditto for the 
      positive projections on faces 2 and 3.

    @param cell_from_face: \f$ \mathbb{R}^{K^c * (p+1)^{d} \times 2d K^f * (p+1)^{d-1}}\f$ 
      This matrix projects the face solution onto the cells. Note the 
      extra factor of @f$ 2d @f$, similar to face_from_cell matrix. Some clever indexing
      is required in the backend here. The enumeration of the cols should follow a similar
      pattern to the rows of face_from_cell.

    @param cell_from_face_scaling: \f$ \mathbb{R}^{K^c * (p+1)^{d} \times 2d K^f * (p+1)^{d-1}}\f$ 
      This is a matrix of ints that prescribes how we scale each element in the cell_from_face
      ie for each i,j, we do: cell_from_face[i,j] *= ( h )^( cell_from_face_scaling[i,j] )

    @param Riemann_matrix: \f$ \mathbb{R}^{ K^f (p+1)^{d-1} \times 2K^f_{f \gets c} (p+1)^{d-1}} @f$
      The matrix that combines the left/right projections on the cell and outputs a value 
      which determines @f$ q @f$.
  
    @param face_relaxation: Float
      This is the @f$ \omega _f @f$ from above. Chosen from @f$ ]0,1[ @f$.
      
    @param approximate_system_matrix: @f$ \mathbb{R}^{K^c (p+1)^d \times K^c (p+1)^d} @f$
      Approximation @f$ \tilde{A} @f$ which will be internally inverted, so it 
      has to be non-singular.

    @param approximate_system_matrix_scaling: @f$ \mathbb{R}^{K^c (p+1)^d \times K^c (p+1)^d} @f$
      
    @param integration_points: [Float]
      Sequence of polynomial_degree+1 entries which identify the position of 
      the integration points along the unit interval.
      
    """  
    super( DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver, self ).__init__(name, min_h, max_h)

    # store everything we pass in anyhow
    self._dimensions                = dimensions
    self._poly_degree               = poly_degree
    self._unknowns_per_cell_node    = unknowns_per_cell_node
    self._solutions_per_face_node   = solutions_per_face_node
    self._projections_per_face_node = projections_per_face_node
    self._assembly_matrix           = assembly_matrix
    self._assembly_matrix_scaling   = assembly_matrix_scaling
    self._mass_matrix               = mass_matrix
    self._mass_matrix_scaling       = mass_matrix_scaling
    self._face_from_cell_projection = face_from_cell_projection
    self._face_from_cell_projection_scaling = face_from_cell_projection_scaling
    self._cell_from_face_projection = cell_from_face_projection
    self._cell_from_face_projection_scaling = cell_from_face_projection_scaling
    self._riemann_matrix            = riemann_matrix
    self._boundary_matrix           = boundary_matrix
    self._approximate_system_matrix = approximate_system_matrix
    self._approximate_system_matrix_scaling = approximate_system_matrix_scaling

    self._cell_relaxation           = cell_relaxation
    self._face_relaxation           = face_relaxation

    self._solver_tolerance          = solver_tolerance
    self._solver_includes           = solver_includes

    self._custom_init_actionset         = custom_init_actionset
    self._custom_plot_actionset         = custom_plot_actionset

    self._custom_init_cell              = custom_init_cell

    self._enable_plotting               = enable_plotting
    self._plot_predicate                = plot_predicate

    self._integration_points            = integration_points

    # K^c * (p+1)^d
    self._unknowns_per_cell = unknowns_per_cell_node * ( poly_degree + 1) ** dimensions
    #(K^f +  2K^c) * (p+1)^(d-1)
    self._unknowns_per_face_solution   = (    solutions_per_face_node) * ( poly_degree + 1) ** (dimensions-1)

    # to each node we leave one copy of each of the cell unknowns from left and right
    self._unknowns_per_face_projection = (2 * projections_per_face_node) * ( poly_degree + 1) ** (dimensions-1)

    # ignore vertex data
    
    self._use_smart_pointer = use_smart_pointer
    if use_smart_pointer:
        self._cell_data.data.add_attribute(peano4.dastgen2.Peano4SmartPointerDoubleArray( "solution", str(self._unknowns_per_cell) ))
        self._cell_data.data.add_attribute(peano4.dastgen2.Peano4SmartPointerDoubleArray( "residual", str(self._unknowns_per_cell) ))
        self._cell_data.data.add_attribute(peano4.dastgen2.Peano4SmartPointerDoubleArray( "rhs",      str(self._unknowns_per_cell) ))

        self._face_data.data.add_attribute(peano4.dastgen2.Peano4SmartPointerDoubleArray( "solution",      str(self._unknowns_per_face_solution ) ))
        self._face_data.data.add_attribute(peano4.dastgen2.Peano4SmartPointerDoubleArray( "lProjection",   str(self._unknowns_per_face_solution ) ))
        self._face_data.data.add_attribute(peano4.dastgen2.Peano4SmartPointerDoubleArray( "rProjection",   str(self._unknowns_per_face_solution ) ))
    else:
        self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "solution", str(self._unknowns_per_cell) ))
        self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "residual", str(self._unknowns_per_cell) ))
        self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "rhs",      str(self._unknowns_per_cell) ))

        self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "solution",      str(self._unknowns_per_face_solution ) ))
        self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "lProjection",   str(self._unknowns_per_face_solution ) ))
        self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "rProjection",   str(self._unknowns_per_face_solution ) ))
    
    
    
    self._face_data.peano4_mpi_and_storage_aspect.merge_implementation = self.get_face_merge_method()
    self._face_data.peano4_mpi_and_storage_aspect.implementation_file_includes = """ 
#include "../{}.h"
#include "repositories/SolverRepository.h"
""".format( self.typename() )

    self.preprocessing_action_set  = None
    self.postprocessing_action_set = None
    
    self._basic_descend_order           = 0

    # assertions for matrix shapes
    # assert mass_matrix.shape     == (self._unknowns_per_cell, self._unknowns_per_cell)
    # assert assembly_matrix.shape == mass_matrix.shape

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
    if self._enable_plotting: # true by default
      observer.add_action_set( PlotDGDataInPeanoBlockFormat(self, "solution", "getSolution", plot_predicate=self._plot_predicate) )
      observer.add_action_set( PlotDGDataInPeanoBlockFormat(self, "rhs", "getRhs", plot_predicate=self._plot_predicate) )

    if self._custom_plot_actionset:
      for name, getter in self._custom_plot_actionset:
        observer.add_action_set( PlotDGDataInPeanoBlockFormat(self, name, getter, plot_predicate=self._plot_predicate) )

  def add_to_create_grid(self, observer):
    observer.add_action_set(peano4.toolbox.CreateRegularGrid(self.max_h))
    pass

  def add_to_init_solution(self, observer):
    """!
    
    Solution initialisation
    
    """
    if self._custom_init_actionset is None:
      observer.add_action_set( InitHigherOrderDofs(self) )
    else:
      observer.add_action_set( self._custom_init_actionset(self) )

  def add_to_solve(self, observer):
    """!
    
    I ignore the order of the action sets here and instead derive my 
    own ordering starting from _basic_descend_order.
    
    """
    descend_order = observer.highest_descend_invocation_order()
    if self.preprocessing_action_set != None:
      self.preprocessing_action_set.descend_invocation_order = 0
      observer.add_action_set( self.preprocessing_action_set )
    observer.add_action_set( UpdateFaceSolution(self, descend_order+1, False) )
    observer.add_action_set( ProjectIntoCellAndUpdateCellSolution(self, descend_order+2, False) )
    observer.add_action_set( ProjectOntoFaces(self, descend_order+3, False) )
    observer.add_action_set( UpdateResidual(self, descend_order+4, False) )
    if self.postprocessing_action_set != None:
      self.postprocessing_action_set.descend_invocation_order = descend_order + 5
      observer.add_action_set( self.postprocessing_action_set )

  
  def add_implementation_files_to_project(self, namespace, output, subdirectory="", abstract_overwrite=True):
    """!
    
    The solver creates two classes: An abstract base class and its 
    implementations. The abstract base class will be overwritten if
    there is one in the directory. I pipe all the Python constants 
    into this base class, so they are available in the C++ code. 
    
    """
    templatefile_prefix  = os.path.dirname( os.path.realpath(__file__) ) + "/templates/"
    templatefile_prefix += self.__class__.__name__

    if(subdirectory):
        subdirectory += "/"

    dictionary = self.create_dictionary()

    generated_abstract_header_file = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".Abstract.template.h",
      templatefile_prefix + ".Abstract.template.cpp",
      "Abstract" + self.typename(), 
      namespace,
      subdirectory + ".",
      dictionary,
      abstract_overwrite,
      True)
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".template.h",
      templatefile_prefix + ".template.cpp",
      self.typename(), 
      namespace,
      subdirectory + ".",
      dictionary,
      False,
      True)

    output.add( generated_abstract_header_file )
    output.add( generated_solver_files )
    output.makefile.add_h_file( subdirectory + "Abstract" + self._name + ".h", generated=True )
    output.makefile.add_h_file( subdirectory + self._name + ".h", generated=True )
    output.makefile.add_cpp_file( subdirectory + "Abstract" + self._name + ".cpp", generated=True )
    output.makefile.add_cpp_file( subdirectory + self._name + ".cpp", generated=True )

  def get_face_merge_method(self):
      """! 

      The actual merge of faces is simple, as we compute the numerical flux
      redundantly, i.e. we really only have to copy stuff one way or the 
      other.
  
      """
      return jinja2.Template("""
  if (!marker.willBeRefined()
    and
    context == ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange

  ) { //todo change this to use the face's own enum

    setLProjection(
      getLProjection() + neighbour.getLProjection()
    );
    setRProjection(
      getRProjection() + neighbour.getRProjection()
    );
  }
    """).render(**self.create_dictionary())

  def create_dictionary(self):
    return {
          "SOLVER_INCLUDES"             : self._solver_includes,
          "MIN_H": self.min_h,
          "MAX_H": self.max_h,
          "SOLVER_NAME":             self.typename(),
          "SOLVER_INSTANCE":         self.instance_name(),
          "CELL_CARDINALITY"            : self._unknowns_per_cell,
          "FACE_CARDINALITY_SOLUTION"   : self._unknowns_per_face_solution,
          "FACE_CARDINALITY_PROJECTION" : self._unknowns_per_face_projection,
          "VERTEX_CARDINALITY"          : 1,
          "NODES_PER_CELL"              : (self._poly_degree + 1) **  self._dimensions,
          "NODES_PER_FACE"              : (self._poly_degree + 1) ** (self._dimensions-1),
          "UNKNOWNS_PER_CELL_NODE"      : self._unknowns_per_cell_node,
          "SOLUTIONS_PER_FACE_NODE"     : self._solutions_per_face_node,
          "PROJECTIONS_PER_FACE_NODE"   : self._projections_per_face_node,
          "RHS_MATRIX"                  : self._mass_matrix,
          "RHS_MATRIX_SCALING"          : self._mass_matrix_scaling,
          "ASSEMBLY_MATRIX"             : self._assembly_matrix,
          "ASSEMBLY_MATRIX_SCALING"     : self._assembly_matrix_scaling,
          "FACE_FROM_CELL_PROJECTION"   : self._face_from_cell_projection,
          "FACE_FROM_CELL_PROJECTION_SCALING": self._face_from_cell_projection_scaling,
          "CELL_FROM_FACE_PROJECTION"   : self._cell_from_face_projection,
          "CELL_FROM_FACE_PROJECTION_SCALING"   : self._cell_from_face_projection_scaling,
          "RIEMANN_MATRIX"              : self._riemann_matrix,
          "BOUNDARY_MATRIX"             : self._boundary_matrix,
          "APPROX_SYSTEM_MATRIX"        : self._approximate_system_matrix,
          "APPROX_SYSTEM_MATRIX_SCALING": self._approximate_system_matrix_scaling,
          "CELL_RELAXATION"             : self._cell_relaxation,
          "FACE_RELAXATION"             : self._face_relaxation,
          "POLY_DEGREE"                 : self._poly_degree,
          "SOLVER_TOLERANCE"            : self._solver_tolerance,
          "CUSTOM_INIT_CELL"            : self._custom_init_cell,
          "INTEGRATION_POINTS"          : self._integration_points,
          "USE_SMART_POINTER"           : self._use_smart_pointer
          }
