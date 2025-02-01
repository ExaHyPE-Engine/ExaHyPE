# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2
import os

from .Solver                                                    import Solver

from .actionsets.dg.InitDofs                                   import InitDofs
from .actionsets.DGPointWiseWithSmartPointerPatches.UpdateResidual                       import UpdateResidual
from .actionsets.DGPointWiseWithSmartPointerPatches.ProjectOntoFaces                     import ProjectOntoFaces
from .actionsets.DGPointWiseWithSmartPointerPatches.UpdateFaceSolution                   import UpdateFaceSolution
from .actionsets.DGPointWiseWithSmartPointerPatches.ProjectIntoCellAndUpdateCellSolution import ProjectIntoCellAndUpdateCellSolution
from .actionsets.PlotDGPatchesInPeanoBlockFormat                                         import PlotDGPatchesInPeanoBlockFormat

class DGPointWiseWithSmartPointerPatches(Solver):
  """!
  We make a DG Solver that stores its data on the heap.

  See the documentation for solvers.api.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver, as
  this solver is almost identical in its usage.
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
              solver_tolerance,
              custom_init_actionset = None,
              custom_plot_actionset = None, # just pass in a list of tuples, one for the variable name and the other for the getter
              custom_init_cell = "",
              custom_init_face = "",
              plot_predicate = "true" # enable custom plotting behaviour
              ):
    super( DGPointWiseWithSmartPointerPatches, self ).__init__(name, min_h, max_h)

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

    self.preprocessing_action_set  = None
    self.postprocessing_action_set = None
    
    self._basic_descend_order           = 0

    self._unknowns_per_cell = unknowns_per_cell_node * ( poly_degree + 1) ** dimensions
    self._unknowns_per_face_solution   = (    solutions_per_face_node) * ( poly_degree + 1) ** (dimensions-1)
    self._unknowns_per_face_projection = (2 * projections_per_face_node) * ( poly_degree + 1) ** (dimensions-1)

    self._cell_patches = []
    self._face_patches = []

    ## 2 integers to store our task numbers


    # cell arrays
    self._cell_data.data.add_attribute(peano4.dastgen2.Peano4IntegerArray( "taskNumber", str(2) ))

    self.add_to_cell_patches( f"{name}Solution", [(poly_degree + 1)]*3, self._unknowns_per_cell_node )
    self.add_to_cell_patches( f"{name}Residual", [(poly_degree + 1)]*3, self._unknowns_per_cell_node )
    self.add_to_cell_patches( f"{name}Rhs"     , [(poly_degree + 1)]*3, self._unknowns_per_cell_node )
    
    self.add_to_face_patches( f"{name}Solution",    [1,(poly_degree + 1),(poly_degree + 1)], self._solutions_per_face_node )
    self.add_to_face_patches( f"{name}LProjection", [1,(poly_degree + 1),(poly_degree + 1)], self._solutions_per_face_node, self.get_face_projection_merge_method() )
    self.add_to_face_patches( f"{name}RProjection", [1,(poly_degree + 1),(poly_degree + 1)], self._solutions_per_face_node, self.get_face_projection_merge_method() )

    self.preprocessing_action_set  = None
    self.postprocessing_action_set = None
    
    self._basic_descend_order           = 0

    self._custom_init_actionset         = custom_init_actionset
    self._custom_plot_actionset         = custom_plot_actionset

    self._custom_init_cell            = custom_init_cell
    self._custom_init_face            = custom_init_face

    self._plot_predicate                = plot_predicate


  def add_to_Peano4_datamodel(self, datamodel, verbose):
    for patch in self._cell_patches:
      datamodel.add_cell( patch )

    for patch in self._face_patches:
      datamodel.add_face( patch )

    super( DGPointWiseWithSmartPointerPatches, self ).add_to_Peano4_datamodel( datamodel, verbose )

  
  def add_use_statements(self, observer):
    for patch in self._cell_patches:
      observer.use_cell(patch)

    for patch in self._face_patches:
      observer.use_face(patch)

    super( DGPointWiseWithSmartPointerPatches, self ).add_use_statements( observer )

  def add_to_plot(self, observer):
    # observer.add_action_set( PlotDGDataInPeanoBlockFormat(self, "solution", "getSolution", plot_predicate=self._plot_predicate) )
    observer.add_action_set( PlotDGPatchesInPeanoBlockFormat(self, f"{self._name}Solution.get", "Solution") )
    pass

  def add_to_create_grid(self, observer):
    observer.add_action_set(peano4.toolbox.CreateRegularGrid(self.max_h))

  def add_to_init_solution(self, observer):
    observer.add_action_set( InitDofs(self) )

  def add_to_solve(self, observer):
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

    dictionary = {
        "SOLVER_INCLUDES": "",
        "MIN_H": self.min_h,
        "MAX_H": self.max_h,
        "SOLVER_NAME":             self.typename(),
        "CELL_CARDINALITY"            : self._unknowns_per_cell,
        "FACE_CARDINALITY_SOLUTION"   : self._unknowns_per_face_solution,
        "VERTEX_CARDINALITY"          : 1,
        "NODES_PER_CELL"              : (self._poly_degree + 1) **  self._dimensions,
        "NODES_PER_FACE"              : (self._poly_degree + 1) ** (self._dimensions-1),
        "UNKNOWNS_PER_CELL_NODE"      : self._unknowns_per_cell_node,
        "FACE_CARDINALITY_PROJECTION" : self._unknowns_per_face_projection,
        "PROJECTIONS_PER_FACE_NODE"   : self._projections_per_face_node,
        "SOLUTIONS_PER_FACE_NODE"     : self._solutions_per_face_node,
        "MASS_MATRIX"                 : self._mass_matrix,
        "MASS_MATRIX_SCALING"         : self._mass_matrix_scaling,
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
        "CUSTOM_INIT_FACE"            : self._custom_init_face,

        }

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

  def add_to_face_patches(self, name, nodes_per_axis, unknowns_per_node, merge_method="", merge_includes=""):
    """!
    Helper function to store patches in the python class
    
    Pass in an array and an int.
    """
    patch           = peano4.datamodel.Patch( nodes_per_axis, unknowns_per_node, name )
    patch.generator = peano4.datamodel.PatchToDoubleArrayWithSmartPointer( patch )
    
    patch.generator.includes                += "#include \"repositories/SolverRepository.h\""
    patch.generator.merge_method_definition  = merge_method
    patch.generator.includes                += merge_includes

    self._face_patches.append(patch)

  def add_to_cell_patches(self, name, nodes_per_axis, unknowns_per_node, merge_method="", merge_includes=""):
    """!
    Helper function to store patches in the python class

    Pass in an array and an int.
    """

    patch           = peano4.datamodel.Patch( nodes_per_axis, unknowns_per_node, name )
    patch.generator = peano4.datamodel.PatchToDoubleArrayWithSmartPointer( patch )
    
    patch.generator.includes                += "#include \"repositories/SolverRepository.h\""
    patch.generator.merge_method_definition  = merge_method
    patch.generator.includes                += merge_includes
    
    self._cell_patches.append(patch)

  def get_face_projection_merge_method(self):
    """!
    This is the way we ensure that projections are merged across spacetrees.

    We do naive += here since:
      - we split up the total projection vector into left and right
      - we don't have a reliable way of determining the offset. During development we encountered
        a bug that meant this could not be calculated reliably (on both spacetrees the selected face number was the same), meaning
        in 50% of cases we would overwrite with junk data
    """
    return """
  if (
    !marker.willBeRefined()
    and
    context == ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange
    and
    value != neighbour.value
  )
    set( get() + neighbour.get() );
  """
