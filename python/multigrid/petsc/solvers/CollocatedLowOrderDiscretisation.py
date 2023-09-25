# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2

import os

from .Solver                                              import Solver
from multigrid.petsc.actionsets.EnumerateDofs             import EnumerateDofs
from multigrid.petsc.actionsets.InitDofs                  import InitVertexDoFs
from multigrid.petsc.actionsets.ProjectPETScSolutionOnVerticesBackOntoMesh import ProjectPETScSolutionOnVerticesBackOntoMesh
from multigrid.petsc.actionsets.AssemblePETSCMatrix       import AssemblePetscMatrixOnVertices

from abc import abstractmethod


class CollocatedLowOrderDiscretisation(Solver):
  """!
   
  A low-order discretisation with degrees of freedom on the vertices
    
  This is one of the simplest solvers that you can think of. It places
  one degree of freedom (can be scalar or a vector) on each vertex. As
  we support solely element-wise assembly, this means you can implement
  things like a 9-point (2d) or 27-point (3d) stencil, but not really
  anything more sophisticated.
  
  """
  def __init__(self,
               name,
               unknowns,
               dimensions,
               min_h,
               max_h,
               cell_system_matrix,
               cell_mass_matrix,
               ):
    """!
   
    Collocated low-order (d-linear) Finite Elements 
       
    """
    super( CollocatedLowOrderDiscretisation, self ).__init__( name,
                                                              min_h,
                                                              max_h
                                                              )
    self._unknowns  = unknowns
    self._vertex_pde_data = peano4.datamodel.DaStGen2( name )
    self._vertex_pde_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",  str(unknowns) ) )
    self._vertex_pde_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",    str(unknowns) ) )

    self.cell_system_matrix = cell_system_matrix
    self.cell_mass_matrix   = cell_mass_matrix
    pass


  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    super( CollocatedLowOrderDiscretisation, self ).add_to_Peano4_datamodel(datamodel,verbose)
    datamodel.add_vertex(self._vertex_pde_data)
  
  
  def add_use_statements(self, observer):
    super( CollocatedLowOrderDiscretisation, self ).add_use_statements(observer)
    observer.use_vertex( self._vertex_pde_data )


  def add_to_plot(self, observer):
    """!
    
    Tell the project's observer how to plot the data
    
    Nothing fancy here. We add plotters from Peano's toolbox to visualise
    solution and right-hand side. 
    
    """
    observer.add_action_set( peano4.toolbox.PlotVertexDataInPeanoBlockFormat(
      filename       = "solution-" + self._name,
      vertex_unknown = self._vertex_pde_data,
      getter         = "getValue().data()",
      description    = "u",
      time_stamp_evaluation        = peano4.toolbox.PlotVertexDataInPeanoBlockFormat.CountTimeSteps, 
      number_of_unknows_per_vertex = self._unknowns,
      guard_predicate              = "true", 
      ))
    observer.add_action_set( peano4.toolbox.PlotVertexDataInPeanoBlockFormat(
      filename       = "rhs-" + self._name,
      vertex_unknown = self._vertex_pde_data,
      getter         = "getRhs().data()",
      description    = "u",
      time_stamp_evaluation        = peano4.toolbox.PlotVertexDataInPeanoBlockFormat.CountTimeSteps, 
      number_of_unknows_per_vertex = self._unknowns,
      guard_predicate              = "true", 
      ))
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
    observer.add_action_set( EnumerateDofs(self) )
    observer.add_action_set( InitVertexDoFs(self) ) 
    pass
  
  
  def add_to_assemble(self, observer):
    observer.add_action_set( AssemblePetscMatrixOnVertices(self, self.stencil) )
    pass
  

  def add_to_init_petsc(self, observer):
    pass


  def add_to_map_solution_onto_mesh(self, observer):
    observer.add_action_set( ProjectPETScSolutionOnVerticesBackOntoMesh(self) )
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
        "MIN_H": self.min_h,
        "MAX_H": self.max_h,
        "VERTEX_CARDINALITY": self._number_of_vertex_unknowns
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
    return self._unknowns
  
  @property
  def number_of_matrix_entries_per_face(self):
    """!
    
    In the DG scheme, we have a projection from the left and we have a 
    projection from the right. These values are proper projections, i.e.
    they do not carry any semantics. Then we have to solve the Riemann
    problem, and need one more unknown to store the outcome of that one.
    
    """
    return 0
  
  @property
  def number_of_matrix_entries_per_cell(self):
    """!
    
    This is the actual unknowns per cell
    
    """
    return 0

  
  def get_solver_repository_snippets(self):
    # nothing extra needed
    header="""

"""
    cpp = """
void {{NAMESPACE | join("::")}}::repositories::computeLocalToGlobalMapsForAllSolvers() {
  {% for SOLVER in SOLVER_INSTANCES %}
  {{SOLVER[1]}}.getLocalToGlobalMap().computeLocalToGlobalMap();
  {% endfor %}
}

int {{NAMESPACE | join("::")}}::repositories::getTotalNumberOfDegreesOfFreedom() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}
"""
    return header, cpp