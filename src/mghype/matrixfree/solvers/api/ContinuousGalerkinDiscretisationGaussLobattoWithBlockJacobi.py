# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import jinja2
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2
import os

from .Solver                                       import Solver
from .actionsets.ContinuousGalerkinDiscretisationGaussLobattoWithBlockJacobi.InitDofs         import InitDofs
from .actionsets.CollocatedSolver.CollocatedSolver import CollocatedSolver
from .actionsets.PlotDGDataInPeanoBlockFormat import PlotDGDataInPeanoBlockFormat


class ContinuousGalerkinDiscretisationGaussLobattoWithBlockJacobi(Solver):
  """!

  Matrix-free Collocated Solver for higher order polynomials.

  This continuous Galerkin p-method works with Gauss-Lobatto nodes only, as we
  employ a block smoother. 

  """

  def __init__(self,
               name,
               unknowns_per_node,
               dimensions,
               polynomial_degree,
               min_h,
               max_h,
               assembly_matrix,
               assembly_matrix_scaling,
               mass_matrix,
               mass_matrix_scaling,
               projection_matrix,
               projection_matrix_scaling,
               integration_points,
               solver_tolerance,
               cell_relaxation,
               face_relaxation   = 0.6,
               custom_init_actionset = None,
               custom_plot_actionset = None, # just pass in a list of tuples, one for the variable name and the other for the getter
               custom_init_cell      = "",
               custom_boundary_conditions = "",
               enable_plotting = True, # sometimes we want to disable standard plotting...
               plot_predicate = "true" # enable custom plotting behaviour
               ):
    """!
   
    Construct solver
    
    @param projection_matrix: Matrix
      
    @param integration_points: [Float]
      Sequence of polynomial_degree+1 entries which identify the position of 
      the integration points along the unit interval.
       
    """
    super( ContinuousGalerkinDiscretisationGaussLobattoWithBlockJacobi, self ).__init__( name,
                                                             min_h = min_h,
                                                             max_h = max_h,
                                                             )
    

    self._dimensions                   = dimensions
    self._polynomial_degree            = polynomial_degree
    self._unknowns_per_cell_node       = unknowns_per_node

    self._assembly_matrix           = assembly_matrix
    self._assembly_matrix_scaling   = assembly_matrix_scaling
    self._mass_matrix               = mass_matrix
    self._mass_matrix_scaling       = mass_matrix_scaling
    self._projection_matrix         = projection_matrix
    self._projection_matrix_scaling = projection_matrix_scaling

    self._cell_relaxation           = cell_relaxation
    self._face_relaxation           = face_relaxation

    self._solver_tolerance          = solver_tolerance

    self._custom_init_actionset         = custom_init_actionset
    self._custom_plot_actionset         = custom_plot_actionset

    self._custom_init_cell              = custom_init_cell
    self._custom_boundary_conditions    = custom_boundary_conditions

    self._enable_plotting               = enable_plotting
    self._plot_predicate                = plot_predicate
    
    self._integration_points            = integration_points

    # K^c * (p+1)^d
    self._degrees_of_freedom_per_cell            = unknowns_per_node * ( polynomial_degree + 1) ** dimensions
    self._degrees_of_freedom_per_face            = unknowns_per_node * ( polynomial_degree + 1) ** (dimensions-1)
    self._inner_degrees_of_freedom_per_cell      = unknowns_per_node * ( polynomial_degree - 1) ** dimensions

    self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "solution", str(self._degrees_of_freedom_per_cell) ))
    self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "residual", str(self._degrees_of_freedom_per_cell) ))
    self._cell_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "rhs",      str(self._degrees_of_freedom_per_cell) ))

    self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "residual",      str(self._degrees_of_freedom_per_face) ))
    self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "diagonal",      str(self._degrees_of_freedom_per_face) ))
    self._face_data.data.add_attribute(peano4.dastgen2.Peano4DoubleArray( "deltaU",        str(self._degrees_of_freedom_per_face) ))

    self.preprocessing_action_set  = None
    self.postprocessing_action_set = None
    
    self._basic_descend_order           = 0
    
    
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

    pass

  def add_to_create_grid(self, observer):
    observer.add_action_set(peano4.toolbox.CreateRegularGrid(self.max_h))
    pass

  def add_to_init_solution(self, observer):
    """!
    
    Solution initialisation. We can either use the
    standard initialisation procedure or pass in 
    another one when we instantiate this solver
    class.
    
    """
    if self._custom_init_actionset is None:
      observer.add_action_set( InitDofs(self) )
    else:
      observer.add_action_set( self._custom_init_actionset(self) )
    pass
    
  def _construct_narrow_to_interior_matrix(self):
    """!

    Construct the matrix which basically picks out the interior points.
  
    """
    result = []
    if self._dimensions==2:
      for image_x in range(0,self._polynomial_degree - 1):
        for image_y in range(0,self._polynomial_degree - 1):
          for image_unknown in range(0,self._unknowns_per_cell_node):
            result.append([])
            for preimage_x in range(0,self._polynomial_degree + 1):
              for preimage_y in range(0,self._polynomial_degree + 1):
                for preimage_unknown in range(0,self._unknowns_per_cell_node):
                  if (preimage_x-1==image_x and
                     preimage_y-1==image_y and
                     preimage_unknown==image_unknown):
                    result[-1].append( 1.0 )
                  else:
                    result[-1].append( 0.0 )
    else:
      for image_x in range(0,self._polynomial_degree - 1):
        for image_y in range(0,self._polynomial_degree - 1):
          for image_z in range(0,self._polynomial_degree - 1):
            for preimage_unknown in range(0,self._unknowns_per_cell_node):
              result.append([])
              for preimage_x in range(0,self._polynomial_degree + 1):
                for preimage_y in range(0,self._polynomial_degree + 1):
                  for preimage_z in range(0,self._polynomial_degree + 1):
                    for preimage_unknown in range(0,self._unknowns_per_cell_node):
                      if (preimage_x-1==image_x and
                         preimage_y-1==image_y and
                         preimage_z-1==image_z and
                         preimage_unknown==image_unknown):
                        result[-1].append( 1.0 )
                      else:
                        result[-1].append( 0.0 )
    return result
  
  
  
  def add_to_solve(self, observer):
#    new_action_set = CollocatedSolver(self)
#    new_action_set.descend_invocation_order = observer.highest_descend_invocation_order() + self._basic_descend_order + 1
#    observer.add_action_set( new_action_set )
    pass
  
  def add_implementation_files_to_project(self, namespace, output, subdirectory="", abstract_overwrite=True):
    """
    
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
        "SOLVER_INCLUDES"             : "",
        "MIN_H"                       : self.min_h,
        "MAX_H"                       : self.max_h,
        "SOLVER_NAME"                 : self.typename(),
        "CELL_CARDINALITY"            : self._degrees_of_freedom_per_cell,
        "INNER_CELL_CARDINALITY"      : self._inner_degrees_of_freedom_per_cell,
        "FACE_CARDINALITY"            : self._degrees_of_freedom_per_face,
        "NODES_PER_CELL"              : (self._polynomial_degree + 1) **  self._dimensions,
        "NODES_PER_FACE"              : (self._polynomial_degree + 1) ** (self._dimensions-1),
        "UNKNOWNS_PER_CELL_NODE"      : self._unknowns_per_cell_node,
        "ASSEMBLY_MATRIX"             : self._assembly_matrix,
        "ASSEMBLY_MATRIX_SCALING"     : self._assembly_matrix_scaling,
        "RHS_MATRIX"                  : self._mass_matrix,
        "RHS_MATRIX_SCALING"          : self._mass_matrix_scaling,
        "SOLVER_TOLERANCE"            :  self._solver_tolerance,
        "CELL_RELAXATION"                      : self._cell_relaxation,
        "FACE_RELAXATION"                      : self._face_relaxation,
        "ABSTRACT_SOLVER_HEADER_EXTRAS"        : self._abstract_solver_header_extras,
        "ABSTRACT_SOLVER_IMPLEMENTATION_EXTRAS": self._abstract_solver_implementation_extras,
        "POLY_DEGREE"                 : self._polynomial_degree,
        "SOLVER_TOLERANCE"            : self._solver_tolerance,
        "CUSTOM_INIT_CELL"            : self._custom_init_cell,
        "CUSTOM_BOUNDARY_CONDITIONS"  : self._custom_boundary_conditions,
        "INTEGRATION_POINTS"          : self._integration_points,
        "NARROW_TO_INTERIOR_MATRIX"   : self._construct_narrow_to_interior_matrix(),
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

