# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import dastgen2

from .Solver       import Solver

from abc import abstractmethod


class CollocatedLowOrderDiscretisation(Solver):
  def __init__(self,
               name,
               number_of_unknowns):
    super( CollocatedLowOrderDiscretisation, self ).__init__( name )
    
    self._number_of_unknowns = number_of_unknowns
    
    self._dof = peano4.datamodel.DaStGen2( name )
    self._dof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "value",  str(number_of_unknowns) ) )
    self._dof.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",    str(number_of_unknowns) ) )
    self._dof.data.add_attribute( peano4.dastgen2.Peano4IntegerArray( "number", "2" ) )

    #self._patch.generator.store_persistent_condition = self._store_cell_data_default_guard()
    #self._patch.generator.load_persistent_condition  = self._load_cell_data_default_guard()

    pass

  def add_to_plot(self, observer):
    pass


  def add_to_create_grid(self, observer):
    pass

  
  def add_to_enumerate(self, observer):
    pass
  
  
  def add_to_init(self, observer):
    pass
  
  
  def add_to_assemble(self, observer):
    pass
  

  def add_to_map_solution_onto_mesh(self, observer):
    pass


  def add_to_plot(self, observer):
    pass
  
  
  def add_use_statements(self, observer):
    observer.use_vertex( self._dof )
    pass


  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    datamodel.add_vertex(self._dof)
  