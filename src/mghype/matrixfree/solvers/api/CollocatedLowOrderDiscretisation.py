import peano4
import jinja2
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2
import os

from .Solver                      import Solver
from .actionsets.InitDofs         import InitDofsCollocated
from .actionsets.CollocatedSolver import CollocatedSolver

from abc import abstractmethod
from peano4.solversteps.ActionSet import ActionSet


class CollocatedLowOrderDiscretisation(Solver):
  """!

  Matrix-free remake of the petsc-based collocated solver

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
               solver_tolerance
               ):
    """!
   
    Collocated low-order (d-linear) Finite Elements 
       
    """
    super( CollocatedLowOrderDiscretisation, self ).__init__( name,
                                                              min_h,
                                                              max_h
                                                              )
  
    self._unknowns_per_vertex = unknowns_per_vertex

    # already initialised this in superclass!
    # self._vertex_pde_data = peano4.datamodel.DaStGen2( name )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",      str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",        str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "fieldDiag", str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "residual",   str(unknowns_per_vertex) ) )

    self._local_assembly_matrix         = local_assembly_matrix
    self._local_assembly_matrix_scaling = local_assembly_matrix_scaling

    self._mass_matrix                   = mass_matrix
    self._mass_matrix_scaling           = mass_matrix_scaling
    self._solver_tolerance              = solver_tolerance
    
    self._basic_descend_order           = 0
    

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
    observer.add_action_set( peano4.toolbox.PlotVertexDataInPeanoBlockFormat(
      filename       = "solution-" + self._name,
      vertex_unknown = self._vertex_data,
      getter         = "getValue().data()",
      description    = "u",
      time_stamp_evaluation        = peano4.toolbox.PlotVertexDataInPeanoBlockFormat.CountTimeSteps, 
      number_of_unknows_per_vertex = self._unknowns_per_vertex,
      guard_predicate              = "true", 
      ))
    observer.add_action_set( peano4.toolbox.PlotVertexDataInPeanoBlockFormat(
      filename       = "rhs-" + self._name,
      vertex_unknown = self._vertex_data,
      getter         = "getRhs().data()",
      description    = "u",
      time_stamp_evaluation        = peano4.toolbox.PlotVertexDataInPeanoBlockFormat.CountTimeSteps, 
      number_of_unknows_per_vertex = self._unknowns_per_vertex,
      guard_predicate              = "true", 
      ))
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
        "VERTEX_CARDINALITY": self._unknowns_per_vertex,
        "SOLVER_NAME":             self.typename(),
        "SOLVER_TOLERANCE"            : self._solver_tolerance,
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

