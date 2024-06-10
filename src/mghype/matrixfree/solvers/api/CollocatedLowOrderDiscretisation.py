import peano4
import jinja2
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2
import os

from .Solver                      import Solver
from .actionsets.InitDofs         import InitDofsCollocated
from .actionsets.CollocatedSolver import CollocatedSolver
from .actionsets.PlotVertexDataInPeanoBlockFormat import PlotVertexDataInPeanoBlockFormat

from abc import abstractmethod
from peano4.solversteps.ActionSet import ActionSet


class CollocatedLowOrderDiscretisation(Solver):
  """!

  \page matrixfree_collocated_solver Matrix-free Collocated Solver.

  Mathematical and algorithmic descriptions are to follow later. For now, 
  we describe how to use this solver and a little about how it works under 
  the hood.

  Each vertex stores a certain number of values, given by the parameter 
  unknowns_per_vertex. In essence, if this parameter is more than one, 
  we are solving multiple different equations simultaneously, or we
  can equivalently think of our PDE being vector-valued.

  The user needs to provide an assembly matrix, and a mass matrix. If 
  our vector-valued PDE has \f$ K \f$ components, then each of these
  matrices should be square, with \f$ K * 2^D \f$ rows.

  In particular, they should be structured as block diagonals. Let
  \f$ A_i \f$ be square with \f$ 2^D \f$ rows. Then the mass/assembly
  matrices should be structured as follows:

  \f$ diag( A_1, A_2, \dots A_K ) \f$.

  During the actual solver step, we run through each equation one 
  after another, fetching the appropriate mass and assembly matrices
  for the equation in hand.

  At present, it hasn't been decided how to plot the output of multiple
  equations, since only 1 is supported.

  # Data held on the vertices

  ## Solution (formerly named Value)
  This array holds the actual solution which we later plot. This is updated when we touch a vertex
  for the last time.

  ## Rhs

  This array holds the right-hand side of the equation. We assign this in 
  experiment-specific implementation files.

  ## Diagonal
  This array holds the sum of the diagonals of the local assembly matrix. This is
  set to 0 at the start of every solver step

  ## Residual
  This is used to store \f$ Mb - Ax \f$, i.e. the RHS multiplied by the matrix, and subtract
  the solution multiplied by the assembly matrix.
  
  # Solver Steps

  ## Initialisation 

  After the grid is created, we run around the mesh and assign types to the vertices and cells.
  
  Vertices:
  - "Coarse" if the vertex is due to be refined
  - "Interior" if it's not on the boundary
  - "Boundary" if it is on the boundary

  Cells:
  - "Coarse" if it's due to be refined
  - Otherwise marked as "Interior"

  ## Solve

  There is only one action set in this solver. We do something non-trivial during touchCellFirstTime,
  touchVertexFirstTime and touchVertexLastTime.

  ### touchVertexFirstTime

  We check that the Vertex is not a coarse grid vertex, then we set the residuals and the diagonals to 0.

  ### touchCellFirstTime

  All four vertices are in scope here. We create three vectors, each of length \f$ 2^D \f$ to hold 
  one value per vertex. For each vertex in the cell (regardless of whether it is on the boundary), we :
  - Update the residual to be \f$ Mb - Ax \f$
  - Increment the diagonal 

  ### touchVertexLastTime
  We update the solution value (on all vertices) to be equal to the solution at the previous sweep, and we add on
  \f$ \omega \times r / d \f$
  where \f$ r \f$  and \f$ d \f$ stand for the residual and diagonal respectively.

  # Enforcement of boundary conditions

  to be completed

  """

  def __init__(self,
               name,
               unknowns_per_vertex,
               dimensions,
               min_h,
               max_h,
               local_assembly_matrix,
               local_assembly_matrix_scaling,
               mass_matrix,
               mass_matrix_scaling,
               solver_tolerance,
               smoother_relaxation = 0.6
               ):
    """!
   
    Collocated low-order (d-linear) Finite Elements 
       
    """
    super( CollocatedLowOrderDiscretisation, self ).__init__( name,
                                                              min_h,
                                                              max_h
                                                              )
    
    matrix_dims = (2**dimensions) * unknowns_per_vertex
# @todo Sean please get these assertions back in but consistent with DG
#    assert mass_matrix.shape == (matrix_dims,matrix_dims), f"Wrong shape given for mass matrix - got {mass_matrix.shape}, but we have {2**dimensions} vertices per cell and {unknowns_per_vertex} unknowns per vertex"
#    assert local_assembly_matrix.shape == (matrix_dims,matrix_dims), f"Wrong shape given for mass matrix - got {local_assembly_matrix.shape}, but we have {2**dimensions} vertices per cell and {unknowns_per_vertex} unknowns per vertex"
  
    self._unknowns_per_vertex = unknowns_per_vertex

    # already initialised this in superclass!
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",      str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",        str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "diag",       str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "residual",   str(unknowns_per_vertex) ) )

    self._local_assembly_matrix         = local_assembly_matrix
    self._local_assembly_matrix_scaling = local_assembly_matrix_scaling

    self._mass_matrix                   = mass_matrix
    self._mass_matrix_scaling           = mass_matrix_scaling
    self._solver_tolerance              = solver_tolerance
    
    self._basic_descend_order           = 0
    
    self._smoother_relaxation           = smoother_relaxation
    

  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    super( CollocatedLowOrderDiscretisation, self ).add_to_Peano4_datamodel(datamodel,verbose)

  def add_use_statements(self, observer):
    super( CollocatedLowOrderDiscretisation, self ).add_use_statements(observer)

  def add_to_plot(self, observer):
    """!
    
    Tell the project's observer how to plot the data
    
    Nothing fancy here. We add plotters from Peano's toolbox to visualise
    solution and right-hand side. 
    
    """
    observer.add_action_set( PlotVertexDataInPeanoBlockFormat(self, "value", "getValue") )
    observer.add_action_set( PlotVertexDataInPeanoBlockFormat(self, "rhs", "getRhs") )
    pass

  def add_to_create_grid(self, observer):
    observer.add_action_set(peano4.toolbox.CreateRegularGrid(self.max_h))
    pass

  def add_to_init_solution(self, observer):
    """!
    
    Solution initialisation
    
    """
    observer.add_action_set( InitDofsCollocated(self) ) 
    pass

  def add_to_solve(self, observer):
    new_action_set = CollocatedSolver(self)
    new_action_set.descend_invocation_order = observer.highest_descend_invocation_order() + self._basic_descend_order + 1
    observer.add_action_set( new_action_set )
    pass
  
  def add_implementation_files_to_project(self, namespace, output):
    """
    
    The solver creates two classes: An abstract base class and its 
    implementations. The abstract base class will be overwritten if
    there is one in the directory. I pipe all the Python constants 
    into this base class, so they are available in the C++ code. 
    
    """
    templatefile_prefix  = os.path.dirname( os.path.realpath(__file__) ) + "/templates/"
    templatefile_prefix += self.__class__.__name__

    dictionary = {
        "SOLVER_INCLUDES": "",
        "MIN_H": self.min_h,
        "MAX_H": self.max_h,
        "LOCAL_ASSEMBLY_MATRIX": self._local_assembly_matrix,
        "LOCAL_ASSEMBLY_MATRIX_SCALING": self._local_assembly_matrix_scaling,
        "MASS_MATRIX"         : self._mass_matrix,
        "MASS_MATRIX_SCALING" : self._mass_matrix_scaling,
        "VERTEX_CARDINALITY":            self._unknowns_per_vertex,
        "SOLVER_NAME":                   self.typename(),
        "SOLVER_TOLERANCE"            :  self._solver_tolerance,
        "SMOOTHER_RELAXATION":           self._smoother_relaxation
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

