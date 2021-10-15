# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Visualiser import Visualiser
from .VTU import VTU
from .VTU import prepare2Dpatches
from .VTU import prepare3Dpatches

try:
    from paraview.simple import *
except ImportError:
    print("Unable to import paraview")

try:
    import vtk
except ImportError:
    print("Unable to import vtk")

import traceback



class Interactive(Visualiser):
  """
    The visualiser is first and foremost a persistency layer around 
    datasets. It does not work on the command line.
  """
  
  def __init__(self, file_name, verbose=False ):
    super(Interactive,self).__init__(file_name, verbose)
    self._tp   = None
    self._grid = None
    self.display_as_tree = False
    
    
  def display(self):
    """
    
      Should be called only once
      
    """
    if self._tp==None:
      self._tp = TrivialProducer()
      self.reload()
    Interact() #Needed if running from command line

    
  def reload(self):
    """
     
     Invokes superclass' reload, converts data into VTU, and then sets 
     the resulting self._data as output of the client side object of 
     the trivial producer. Does not work if self._tp is set to None.
    
    """
    super(Interactive,self).reload()
    #self._grid = render_dataset( 
    #  self.file_name,
    #  self.identifier,
    #  self._dataset_number,
    #  False, # display_as_tree
    #  self.filter,
    #  self.verbose
    #)
    
    if self._dimensions == 2 and self.display_as_tree:
      self._grid = prepare2Dpatches(self._cell_data, self._dof, self._unknowns, 1.0, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
    elif self._dimensions == 2 and not self.display_as_tree:
      self._grid = prepare2Dpatches(self._cell_data, self._dof, self._unknowns, 0.0, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
    else: # Tested above that it can only be 2 or 3
      self._grid = prepare3Dpatches(self._cell_data, self._dof, self._unknowns, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
  
    if self._tp != None and self._grid!=None:
      self._tp.GetClientSideObject().SetOutput(self._grid)
      Show(self._tp)
      print( "Please press the play button to update your pipeline" )
      # 
      #visualiser._tp.GetClientSideObject().Update()
      