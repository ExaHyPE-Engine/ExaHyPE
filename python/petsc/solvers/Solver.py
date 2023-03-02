# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4


from abc import abstractmethod

class Solver(object):
  def __init__(self, 
               name):
    """
    
    name: String
      Every solver has a unique name. 
    
    """
    self._name = name
    pass
    
    
  @abstractmethod
  def add_to_plot(self, observer):
    assert False, "should not be called"


  @abstractmethod
  def add_to_create_grid(self, observer):
    assert False, "should not be called"

  
  @abstractmethod
  def add_to_enumerate(self, observer):
    assert False, "should not be called"
  
  
  @abstractmethod
  def add_to_init(self, observer):
    assert False, "should not be called"
  
  
  @abstractmethod
  def add_to_assemble(self, observer):
    assert False, "should not be called"
  

  @abstractmethod
  def add_to_map_solution_onto_mesh(self, observer):
    assert False, "should not be called"


  @abstractmethod
  def add_to_plot(self, observer):
    assert False, "should not be called"
  
  
  @abstractmethod
  def add_use_statements(self, observer):
    assert False, "should not be called"


  @abstractmethod
  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    assert False, "should not be called"
  