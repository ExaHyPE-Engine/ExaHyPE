# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org
import peano4
import dastgen2

import numpy as np
from numpy.polynomial.legendre  import leggauss
from scipy.interpolate import lagrange

from abc import abstractmethod

class Solver(object):
  """!

Abstract base class for all PETSc solvers

A PETSc solver in Peano is Python class which represents one solver aka one
PDE problem. Various of these solvers might have to be combined in one
project.

It is the project's job to define which mesh traversals (grid
construction, visualisation, ...) are required, how to run through these,
how to load balance. Each solver in return will know what unknows we require
per mesh entity (vertex, face, ...). Most solvers will create a Python
representation of these unknowns in their constructor. Once the user has 
constructed a solver and added it to the underlying
project, is it the project's job to ask each solver

- can you add your data model to the underlying Peano project;
- can you tell me which data entities all of the mesh traversals have to
  use;
- add your particular action sets to the grid construction, initialisation,
  visualisation, ...

For further information how the solvers add their solver-specific data to the 
project's generic mesh traversals, consult also generic documentation in peano4.actionsets.ActionSet.


## Realisation

Most solvers will split the realisation of the solver into different parts.

First and foremost, it will have to manage data associated with the grid vertices,
faces, cells. This is the static data model. As I don't want to write classes
for these unknowns, the solvers typically use DaStGen 2, which simply is a
code generator to create a class per data model. This part is all handled 
within Python, and you should not see them in the output. If you want to study
these models however, you can peek into the generated vertexdata or facedata
repositories.

For the actual code semantics, most solvers introduce a type which represents
the solver. This one defines, for example, what the initial value looks
like. It is actually realised as two classes: An abstract base class and its
implementation. The abstract base class contains all the variables that can
be modified via Python. The realisation itself is then the actual C++ code
which users might want to modify after the Python call has succeeded and set
everything up. That is: the Python PETSc API will give you both an abstract
solver class and its subclass in the working directory. The abstract base 
class will be overwritten every time you run the Python script. The subclass
is only a template that you should befill with your problem-specific knowledge.
It will not be overwritten by any subsequent Python calls.

The actual solver class that you have to implement yourself is instantiated
within a file called repositories/SolverRepository. This one is generated, too, 
but you can peek into it. It instantiates each solver exactly one and this is 
the object instance against which the actual solver algorithm operates from 
hereon. 

The glue between Peano and the solver is the action sets, i.e. the classes that encode how the grid
traversal and observations made throughout the traversal are mapped onto
either Peano's routines, routines from the toolbox, or functions from the
solver objects. These action sets define what happens if the code reads a
vertex for the first time of a mesh traversal, e.g.


## Links to high level concepts

- For the types of mesh entities, please consult @ref documentation_multigrid_boundary_conditions.



  """
  
  
  def __init__(self, 
               name,
               min_h,
               max_h):
    """
    
    name: String
      Every solver has a unique name. 
    
    """
    self._name = name
    self.min_h = min_h
    self.max_h = max_h
    
    self._vertex_petsc_data = peano4.datamodel.DaStGen2( name + "PETScData")
    self._face_petsc_data   = peano4.datamodel.DaStGen2( name + "PETScData" )
    self._cell_petsc_data   = peano4.datamodel.DaStGen2( name + "PETScData" )
    
    self._vertex_petsc_data.data.add_attribute( dastgen2.attributes.Integer( "unknownBaseNumber" ) )
    self._face_petsc_data.data.add_attribute(   dastgen2.attributes.Integer( "unknownBaseNumber" ) )
    self._cell_petsc_data.data.add_attribute(   dastgen2.attributes.Integer( "unknownBaseNumber" ) )

    self._vertex_petsc_data.data.add_attribute( dastgen2.attributes.Enumeration( name="type", variants=["Boundary", "Interior", "Coarse", "Outside", "Undefined"] ) )
    self._face_petsc_data.data.add_attribute(   dastgen2.attributes.Enumeration( name="type", variants=["Boundary", "Interior", "Coarse", "Outside", "Undefined"] ) )
    self._cell_petsc_data.data.add_attribute(   dastgen2.attributes.Enumeration( name="type", variants=["Interior", "Coarse", "Outside", "Undefined"] ) )
    pass
    
    
  @abstractmethod
  def add_to_plot(self, observer):
    """!
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"


  @abstractmethod
  def add_to_create_grid(self, observer):
    """!
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"

  @abstractmethod
  def add_to_enumerate_and_init_solution(self, observer):
    """!
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"

  @abstractmethod
  def add_to_init_petsc(self, observer):
    """!
    
    Add whatever action set you want to use here. I am not sure if there is a 
    real grid traversal tied to this step. So maybe nothing is called. Has to 
    be checked. Anyway, most solvers leave this one blank.
    
    """
    assert False, "should not be called"
  
  
  @abstractmethod
  def add_to_assemble(self, observer):
    """!
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"
  

  @abstractmethod
  def add_to_map_solution_onto_mesh(self, observer):
    """
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"


  @abstractmethod
  def add_to_plot(self, observer):
    """
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"
  
  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    """!
    
    Initialise index model 
    
    We build up a data model, which is really an index model in this case. 
    Every vertex, face and cell can hold a data model which is only one integer.
    This integer encodes the first index of a matrix unknown held by the
    grid entity. For the plain DG code, there are no vertex unknowns. However,
    we have two types of face indices: Those for the projection and those for 
    the solution of the Riemann problem.
    
    You might want to overwrite this routine, but please ensure that you still
    call this superclass implementation, too.
    
    """
    datamodel.add_vertex(self._vertex_petsc_data)
    datamodel.add_face(self._face_petsc_data)
    datamodel.add_cell(self._cell_petsc_data)
    pass

  def add_use_statements(self, observer):
    """!
    
    Register the index numbers to be used in each and every mesh traversal. You
    can overwrite the routine and add your own stuff. However, please ensure 
    this routine still is invoked, too.    
    
    """
    observer.use_vertex(self._vertex_petsc_data)
    observer.use_face(self._face_petsc_data)
    observer.use_cell(self._cell_petsc_data)
    pass


  @abstractmethod    
  def add_implementation_files_to_project(self, namespace, output):
    """!
    
This routine is typically not invoked by a user.

output: peano4.output.Output
  Add artefacts this output in any implementation.
     
    """
    assert False, "should not be called"


  def typename(self):
    return self._name;

      
  def instance_name(self):
    """!
    
Return the name of the object that will be created for this solver.
    
    """
    return "instanceOf" + self.typename()  

  @property
  def number_of_matrix_entries_per_vertex(self):
    raise NotImplementedError
  
  @property
  def number_of_matrix_entries_per_face(self):
    raise NotImplementedError
  
  @property
  def number_of_matrix_entries_per_cell(self):
    raise NotImplementedError

  @abstractmethod
  def create_readme_descriptor(self):
      """!
      
      Should really be abstract and properly be initialised
      
      @todo Has to be implemented properly, i.e. should at least report on the
            type and the name of the solver
            
      """
      return "Yet to be written"
