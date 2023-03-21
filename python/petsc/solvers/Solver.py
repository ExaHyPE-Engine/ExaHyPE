# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4


from abc import abstractmethod

class Solver(object):
  """

  Abstract base class for all PETSc solvers

  A PETSc solver in Peano is Python class which represents one solver aka one
  PDE problem. Various of these solvers might have to be combined in one
  project.

  It is the project's job to define which mesh travesals (grid
  construction, visualisation, ...) are required, how to run through these,
  how to load balance. Each solver in return will know what unknows we require
  per mesh entity (vertex, face, ...). Most solvers will create a Python
  representation of these unknowns in their constructor.

  Once the user has constructed a solver and added it to the underlying
  project, is it the project's job to ask each solver

  - can you add your data model to the underlying Peano project;
  - can you tell me which data entities all of the mesh traversals have to
    use;
  - add your particular action sets to the grid construction, initialisation,
    visualisation, ...

  ## Realisation

  Most solvers will split the realisation of the solver into different parts.

  First and foremost, it will have data associated with the grid vertices,
  faces, cells. This is the static data model. As I don't want to write classes
  for these unknowns, the solvers typically use DaStGen 2, which simply is a
  code generator to create a class per data model.

  For the actual code semantics, most solvers create a class which represents
  the solver. This class defines, for example, what the initial value looks
  like. Most solvers will create two classes: An abstract base class and its
  implementation. The abstract base class contains all the variables that can
  be modified via Python. The realisation itself is then the actual C++ code
  which users might want to modify after the Python call has succeeded and set
  everything up.

  The glue is the action sets, i.e. the classes that encode how the grid
  traversal and observations made throughout the traversal are mapped onto
  either Peano's routines, routines from the toolbox, or functions from the
  solver objects. These action sets define what happens if the code reads a
  vertex for the first time of a mesh traversal, e.g.

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
    pass
    
    
  @abstractmethod
  def add_to_plot(self, observer):
    """
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"


  @abstractmethod
  def add_to_create_grid(self, observer):
    """
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"

  
  @abstractmethod
  def add_to_enumerate(self, observer):
    """
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"
  
  
  @abstractmethod
  def add_to_init(self, observer):
    """
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"
  
  
  @abstractmethod
  def add_to_assemble(self, observer):
    """
    
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
  
  
  @abstractmethod
  def add_use_statements(self, observer):
    """
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"


  @abstractmethod
  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    """
    
    Add whatever action set you want to use to observer.
    
    """
    assert False, "should not be called"


  @abstractmethod    
  def add_implementation_files_to_project(self, namespace, output):
    """
    
     The PETSc project will call this operation when it sets
     up the overall environment.
     
     This routine is typically not invoked by a user.

     output: peano4.output.Output
     
    """
    assert False, "should not be called"


  def typename(self):
    return self._name;

      
  def instance_name(self):
    return "instanceOf" + self.typename()  


