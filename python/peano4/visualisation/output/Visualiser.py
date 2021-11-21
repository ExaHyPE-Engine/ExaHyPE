# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.visualisation.PatchFileParser import PatchFileParser

import multiprocessing


def validate(parsers):
  """
  
   All the parsers in a data set have to yield the same type of data, i.e.
   with the same number of unknowns and dofs/patch. I return the crucial
   data afterwards, i.e. this routine validates and returns the quantities 
   of interest.
  
  """
  dimensions  = -1
  unknowns    = -1
  dof         = -1
  description = ""
  is_data_associated_to_cell = True
  mapping     = ""
  for parser in parsers:
    snapshot_cell_data, snapshot_dof, snapshot_dimensions, snapshot_unknowns, description, is_data_associated_to_cell, mapping = parser.cell_data, parser.dof, parser.dimensions, parser.unknowns, parser.description, parser.is_data_associated_to_cell, parser.mapping

    if snapshot_dimensions != 2 and snapshot_dimensions != 3:
      print("File parsing unsuccessful. Supported dimensions are d=2 and d=3")
      return 
    if snapshot_dof == 0:
      print("File parsing unsuccessful. No dof specified")
      return 
    if snapshot_unknowns == 0:
      print("File parsing unsuccessful. No unknowns specified")
      return
       
    if snapshot_dimensions!=dimensions and dimensions>0:
      print( "Dimensions not compatible with dimensions from previous files in the snapshot" )
      return
    dimensions = snapshot_dimensions
    if snapshot_dof!=dof and dof>0:
      print("DoF not compatible with dof from previous files in the snapshot")
      return 
    dof=snapshot_dof
     
    if snapshot_unknowns!=unknowns and unknowns>0:
      print("Unknowns not compatible with unknowns from previous files in the snapshot")
      return
    unknowns=snapshot_unknowns
  return dof, dimensions, unknowns, description, is_data_associated_to_cell, mapping



class Visualiser(object):
  """
  
    The visualiser is first and foremost a persistency layer around 
    datasets. It serves as abstract base class for real output formats,
    i.e. it can parse files and hold them, but it does not do anything
    with them.

  """
  
  def __init__(self, file_name, verbose=False ):
    """
    
     file_name: String
       Name of a Peano patch file. This has to be a meta file, i.e. a file which 
       does not hold actual data but hosts data sets that in turn link to actual
       data. 
       
      :: Attributes
      
      self._data: Patch
        
    """
    self._file_name       = file_name
    self._dataset_number = 0
    self._filter          = []
    self.identifier      = ""
    self.verbose         = verbose

    self._cell_data           = None
    
    self._dof            = 0
    self._dimensions     = 0
    self._unknowns       = 0
    self._description    = ""
    self._is_data_associated_to_cell = False
    self._mapping        = None
    self._timestamp      = -1.0

    
  def display(self):
    """
    
    This is the one routine the visualisers typically do overwrite.
    
    """
    if self.verbose:
      print( "empty display() invoked")
    pass


  def select_dataset(self, number):
    """
      number: int
    """
    self._dataset_number = number
    self.reload()
  
  
  def append_filter(self, filter, reload=True):
    """
     
    """
    self._filter.append(filter)
    if reload:
      self.reload()
    
    
  def remove_filters(self):
    """
     
    """
    self._filter = []
    self.reload()
    
    
  def reload(self):
    """
     
     Invokes render_dataset() and then sets the resulting self._data
     as output of the client side object of the trivial producer. Does
     not work if self._tp is set to None
    
    """
    input_file = open( self._file_name, "r" )
    lines = input_file.readlines()
  
    #
    # Will be decreased by first hit
    #
    dataset_number = self._dataset_number+1
    include_file_counter = 0

    parsers = []  
    for line in lines:
      if "begin dataset" in line:
        dataset_number = dataset_number-1
        include_file_counter = 0
      if "timestamp" in line and dataset_number==0:
        self._timestamp = float(list(line.split( "  " ))[2][:-1])
      if "include" in line and dataset_number==0:
        snapshot_file_name = line.split( "\"" )[1]
        if self.verbose:        
          print( "parse file ", snapshot_file_name )
  
        parser = PatchFileParser(snapshot_file_name, self.identifier, include_file_counter)
        parsers.append( parser )
        include_file_counter += 1

    if self.verbose:        
      print( "Will invoke " + str( len(parsers) ) + " parsers in parallel" )
    
    if len(parsers)==0:
      if self.verbose:        
        print( "Snapshot is empty. Return" )
   
      self._cell_data  = []
      self._dof            = 0
      self._dimensions     = 0
      self._unknowns       = 0
      self._description    = ""
      self._mapping        = None
      return None
      
    #
    # This can be done in parallel
    #
    pool = multiprocessing.Pool( len(parsers) ) 
    for parser in parsers:
      pool.apply_async( parser.parse_file() )
      #parser.parse_file()
    pool.close()
    pool.join()
    
    if self.verbose:        
      print( "All individual files are parsed" )
      print( "Apply filter(s) to individual files (where appropriate)" )
    
    for parser in parsers:
      for p in self._filter:
        if p.run_on_individual_pieces_of_data:
          print( "apply filter " + str(p) + " to snapshot")
          parser.cell_data, parser.dof, parser.dimensions, parser.unknowns, parser.description, parser.mapping = p.render(parser.cell_data, parser.dof, parser.dimensions, parser.unknowns, parser.description, parser.mapping)
  
    self._cell_data  = []
    self._dof, self._dimensions, self._unknowns, self._description, self._is_data_associated_to_cell, self._mapping = validate(parsers)
       
    if self.verbose:        
      print( "Concatenate snapshots" )

    for parser in parsers:
      self._cell_data =  self._cell_data + parser.cell_data

    num_patches = len(self._cell_data)
    print("Total number of patches:", num_patches)


    self._dof, self._dimension, self._unknowns, self._description, self._is_data_associated_to_cell, self._mapping = validate(parsers)
    print( "Parsing complete. Convert set of " + str(len(self._cell_data)) + " patches with " + str(self._unknowns) + " unknowns per " + str(self._dof) + "^" + str(self._dimensions) + " patch/cell into VTK data structures" )
  
    if filter!=None:
      if self.verbose:        
        print( "Apply " + str(len(self._filter)) + " filter(s) to concatenated data set" )
      for p in self._filter:
       if p.run_on_concatenated_data:
         print( "apply filter " + str(p) )
         self._cell_data, self._dof, self._dimensions, self._unknowns, self._description, self._mapping = p.render(self._cell_data, self._dof, self._dimensions, self._unknowns, self._description, self._mapping)
     
