import peano4
import dastgen2
from abc import abstractmethod

class Solver:
  """!

  Abstract base class for matrix free solvers.

  """

  def __init__(self, 
               name,
               min_h,
               max_h):
    self._name = name
    self.min_h = min_h
    self.max_h = max_h
    
    self._vertex_data = peano4.datamodel.DaStGen2( name )
    self._face_data   = peano4.datamodel.DaStGen2( name )
    self._cell_data   = peano4.datamodel.DaStGen2( name )

    self._vertex_data.data.add_attribute( dastgen2.attributes.Enumeration( name="type", variants=["Boundary", "Interior", "Corner", "Coarse", "Outside", "Undefined"] ) )
    self._face_data.data.add_attribute(   dastgen2.attributes.Enumeration( name="type", variants=["Boundary", "Interior", "Coarse", "Outside", "Undefined"] ) )
    self._cell_data.data.add_attribute(   dastgen2.attributes.Enumeration( name="type", variants=["Interior", "Coarse", "Outside", "Undefined"] ) )

    # for use with verification suite
    self._abstract_solver_header_extras         = """ """
    self._abstract_solver_implementation_extras = """ """


  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    datamodel.add_vertex(self._vertex_data)
    datamodel.add_face(self._face_data)
    datamodel.add_cell(self._cell_data)
    
  def add_use_statements(self, observer):
    """!
    
    This routine should still be called even if overwritten
    in child class.
    
    """
    observer.use_vertex(self._vertex_data)
    observer.use_face(  self._face_data)
    observer.use_cell(  self._cell_data)
  
  def typename(self):
    return self._name;

  def instance_name(self):
    """!
    
Return the name of the object that will be created for this solver.
    
    """
    return "instanceOf" + self.typename()  

  @abstractmethod
  def create_readme_descriptor(self):
    return "not written yet"

  def add_to_abstract_solver(self, header, implementation):
    self._abstract_solver_header_extras         = header
    self._abstract_solver_implementation_extras = implementation

  def add_to_vertex_data(self, name, size):
    """!
    Add in extra attributes on the fly
    """
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( name, size ) )

  def add_to_cell_data(self, name, size):
    """!
    Add in extra attributes on the fly
    """
    self._cell_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( name, size ) )

  def add_to_face_data(self, name, size):
    """!
    Add in extra attributes on the fly
    """
    self._face_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( name, size ) )
