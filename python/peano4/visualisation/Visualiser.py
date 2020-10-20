# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.visualisation.OutputFileParser import OutputFileParser
from paraview.simple import *


import multiprocessing



def prepare2Dpatches(cell_data, dof, unknowns, depth_scaling, description):
  """
    Prepare 2D patches in grid for rendering 
    
    Parameters:
    ----------
    cell_data: list of patches
      List of Patches with file data 

    dof: Integer
      Number of degrees of freedom per axis

    unknowns: Integer
      Number of unknowns per patch volume  
      
    Returns:  
    ----------
    grid: paraview.vtk.vtkUnstructuredGrid
      Grid cells
    
  """

  points = paraview.vtk.vtkPoints()
  cells  = paraview.vtk.vtkCellArray()
  grid   = paraview.vtk.vtkUnstructuredGrid() 

  data   = paraview.vtk.vtkDoubleArray()
  data.SetNumberOfComponents(unknowns+1)
  if description!="":
    data.SetName("Unknowns (" + description + ", subdomain)" )
  else:                  
    data.SetName("Unknowns (..., subdomain)" )

  count = 0

  for p in range( len(cell_data) ):

    patch_x_0 = cell_data[p].offset[0]
    patch_y_0 = cell_data[p].offset[1]
    patch_x_1 = cell_data[p].offset[0] + cell_data[p].size[0]
    patch_y_1 = cell_data[p].offset[1] + cell_data[p].size[1]

    z = cell_data[p].size[0] * depth_scaling

    cell_length = cell_data[p].size[0]/float(dof)
    cell_height = cell_data[p].size[1]/float(dof) 

    for k in range(dof):
      for l in range(dof):
        cell_number = k*dof + l
    
        x_0 = patch_x_0 + l*cell_length
        y_0 = patch_y_0 + k*cell_height
        x_1 = x_0 + cell_length
        y_1 = y_0 + cell_height
       
        points.InsertNextPoint([x_0, y_0, z])
        points.InsertNextPoint([x_1, y_0, z])
        points.InsertNextPoint([x_0, y_1, z])
        points.InsertNextPoint([x_1, y_1, z])


        this_cell = cells.InsertNextCell(4)    
        cells.InsertCellPoint(count)
        cells.InsertCellPoint(count+1)
        cells.InsertCellPoint(count+2)
        cells.InsertCellPoint(count+3)
          
        new_data = [x for x in cell_data[p].values[cell_number*unknowns:cell_number*unknowns+unknowns] ]
        new_data.append(cell_data[p].subdomain_number)
        data.InsertNextTuple(new_data)
  
        count = count+4

  grid.SetPoints(points)
  grid.SetCells(paraview.vtk.VTK_PIXEL, cells)
  grid.GetCellData().SetScalars(data)

  return grid



def prepare3Dpatches(cell_data, dof, unknowns, description):
  """
    Prepare 3D patches in grid for rendering 
    
    Parameters:
    ----------
    cell_data: list of patches
      List of Patches with file data 

    dof: Integer
      Number of degrees of freedom per axis

    unknowns: Integer
      Number of unknowns per patch volume  
      
    Returns:  
    ----------
    grid: paraview.vtk.vtkUnstructuredGrid
      Grid cells
    
  """

  points = paraview.vtk.vtkPoints()
  cells  = paraview.vtk.vtkCellArray()
  grid   = paraview.vtk.vtkUnstructuredGrid() 

  data   = paraview.vtk.vtkDoubleArray()
  data.SetNumberOfComponents(unknowns+1)
  if description!="":
    data.SetName("Unknowns (" + description + ", subdomain)" )
  else:                  
    data.SetName("Unknowns (..., subdomain)" )

  count = 0

  for p in range(len(cell_data)):

    patch_x_0 = cell_data[p].offset[0]
    patch_y_0 = cell_data[p].offset[1]
    patch_z_0 = cell_data[p].offset[2]
    patch_x_1 = cell_data[p].offset[0] + cell_data[p].size[0]
    patch_y_1 = cell_data[p].offset[1] + cell_data[p].size[1]
    patch_z_1 = cell_data[p].offset[2] + cell_data[p].size[2]
    
    cell_length = cell_data[p].size[0]/float(dof)
    cell_height = cell_data[p].size[1]/float(dof) 
    cell_depth  = cell_data[p].size[2]/float(dof) 

    for k in range(dof):
      for l in range(dof):
        for m in range(dof):
    
          cell_number = k*dof*dof + l*dof + m
    
          x_0 = patch_x_0 + m*cell_length
          y_0 = patch_y_0 + l*cell_height
          z_0 = patch_z_0 + k*cell_depth
          x_1 = x_0 + cell_length
          y_1 = y_0 + cell_height
          z_1 = z_0 + cell_depth
       
          points.InsertNextPoint([x_0, y_0, z_0])
          points.InsertNextPoint([x_1, y_0, z_0])
          points.InsertNextPoint([x_0, y_0, z_1])
          points.InsertNextPoint([x_1, y_0, z_1])
          points.InsertNextPoint([x_0, y_1, z_0])
          points.InsertNextPoint([x_1, y_1, z_0])
          points.InsertNextPoint([x_0, y_1, z_1])
          points.InsertNextPoint([x_1, y_1, z_1])

          this_cell = cells.InsertNextCell(8)    
          cells.InsertCellPoint(count)
          cells.InsertCellPoint(count+1)
          cells.InsertCellPoint(count+2)
          cells.InsertCellPoint(count+3)
          cells.InsertCellPoint(count+4)
          cells.InsertCellPoint(count+5)
          cells.InsertCellPoint(count+6)
          cells.InsertCellPoint(count+7)

          new_data = [x for x in cell_data[p].values[cell_number*unknowns:cell_number*unknowns+unknowns] ]
          new_data.append(cell_data[p].subdomain_number)
          data.InsertNextTuple(new_data)
             
          count = count+8

  grid.SetPoints(points)
  grid.SetCells(paraview.vtk.VTK_VOXEL, cells)
  grid.GetCellData().SetScalars(data)

  return grid

      


def render_single_file(filename, identifier, display_as_tree = True, filter=None):
  """
    Parse a single peano-patch-file and render data 
    
    Parameters:
    ----------
    filename: String
      Path to file to be parsed
 
    identifier: String
      Name of the set of unknowns we want to extract  
      
    display_as_tree: Boolean
      Displays multiscale data as tree, i.e. uses the z-axis for the 
      mesh level. The flag has no meaning for 3d files.
      
    filter: List of filter objects
      If it is None, then we don't apply any filter on the input file.
     
      
      
    Result:
    -------
    
    Returns an instace of TrivialProducer. You can call Interact
    on this object, e.g.
       
       
  """
  
  parser = OutputFileParser(filename,identifier,0)
  parser.parse_file()
  cell_data, dimensions, dof, unknowns, description = parser.cell_data, parser.dimensions, parser.dof, parser.unknowns, parser.description
  
  if dimensions != 2 and dimensions != 3:
    print("File parsing unsuccessful. Supported dimensions are d=2 and d=3")
    return 
  if dof == 0:
    print("File parsing unsuccessful. No dof specified")
    return 
  if unknowns == 0:
    print("File parsing unsuccessful. No unknowns specified")
    return 

  print("Dimensions: ", dimensions)
  print("Number of variables per cell: ", unknowns)
  print("Number of cells per patch per direction: ", dof)
 
  num_patches = len(cell_data)
  print("Number of patches:", num_patches)
  
  if filter!=None:
    for p in filter:
      cell_data, dof, unknowns, dimensions = p.render(cell_data, dof, unknowns, dimensions)

  if dimensions == 2 and display_as_tree:
    grid = prepare2Dpatches(cell_data, dof, unknowns, 1.0, description) 
  elif dimensions == 2 and not display_as_tree:
    grid = prepare2Dpatches(cell_data, dof, unknowns, 0.0, description) 
  else: # Tested above that it can only be 2 or 3
    grid = prepare3Dpatches(cell_data, dof, unknowns, description) 
  
  return grid


def render_dataset(filename, identifier, dataset_number=0, display_as_tree = True, filter=None):
  """
  
    Peano patch files can either hold data (see render_single_file)
    or they can serve as meta file linking to other files. In the 
    latter case, they hold a sequence of data sets and each data
    set holds multiple files written by multiple threads.
    
    This routine picks one dataset (typically, a data set corresponds
    to one snapshot at one point in time), and loads all files 
    associated with this data set. It then fuses these individual 
    files into one big one before it converts the data into vtk.
    The routine therefore is a natural extension of render_single_file
    into something that acts on a set of files.

    Parameters:
    ----------
    filename: String
      Path to file to be parsed
    
    identifier: String
      Name of the set of unknowns we want to extract
    
    dataset_number: int
      Number of dataset within file that is to be read
      
      
    Result:
    -------
    
    Returns an instace of TrivialProducer. You can call Interact
    on this object, e.g.
       
  """
  def __apply_filter(parser,unknowns, dimensions):
    if filter!=None:
      snapshot_cell_data, snapshot_dimensions, snapshot_dof, snapshot_unknowns, description = parser.cell_data, parser.dimensions, parser.dof, parser.unknowns, parser.description
      for p in filter:
        if p.exploit_idempotent:
          snapshot_cell_data, snapshot_dof, snapshot_unknowns, dimensions = p.render(snapshot_cell_data, snapshot_dof, snapshot_unknowns, dimensions)
      parser.cell_data, parser.dimensions, parser.dof, parser.unknowns, parser.description = snapshot_cell_data, snapshot_dimensions, snapshot_dof, snapshot_unknowns, description


  input_file = open( filename, "r" )
  lines = input_file.readlines()
  
  dimensions = -1
  unknowns   = -1
  dof        = -1
  cell_data  = []
  
  #
  # Will be decreased by first hit
  #
  dataset_number = dataset_number+1
  include_file_counter = 0

  parsers = []  
  for line in lines:
    if "begin dataset" in line:
      dataset_number = dataset_number-1
      include_file_counter = 0
    if "include" in line and dataset_number==0:
      snapshot_file_name = line.split( "\"" )[1]
      print( "parse file ", snapshot_file_name )

      parser = OutputFileParser(snapshot_file_name,identifier, include_file_counter)
      parsers.append( parser )
      include_file_counter += 1
      
  print( "Will read " + str( len(parsers) ) + " parsers in parallel" )
      
  #
  # This can be done in parallel
  #
  pool = multiprocessing.Pool( len(parsers) ) 
  for parser in parsers:
    pool.apply_async( parser.parse_file() )
    #parser.parse_file()
  pool.close()
  pool.join()
    
  print( "All individual files are parsed" )
    
  for parser in parsers:
    snapshot_cell_data, snapshot_dimensions, snapshot_dof, snapshot_unknowns, description = parser.cell_data, parser.dimensions, parser.dof, parser.unknowns, parser.description

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
       
  print( "Apply filter to individual snapshots" )

  #
  # Parallel
  #
  pool = multiprocessing.Pool( len(parsers) ) 
  for parser in parsers:
    pool.apply_async( __apply_filter( parser, unknowns, dimensions) )
    #__apply_filter( parser, unknowns, dimensions)
  pool.close()
  pool.join()

  print( "Concatenate snapshots" )

  for parser in parsers:
    cell_data =  cell_data + parser.cell_data

  num_patches = len(cell_data)
  print("Total number of patches:", num_patches)

  
  if filter!=None:
    print( "Apply " + str(len(filter)) + " filter(s) to concatenated data set" )
    for p in filter:
     cell_data, dof, unknowns, dimensions = p.render(cell_data, dof, unknowns, dimensions)

  print( "Convert into VTK data structures" )

  if dimensions == 2 and display_as_tree:
    grid = prepare2Dpatches(cell_data, dof, unknowns, 1.0, description ) 
  elif dimensions == 2 and not display_as_tree:
    grid = prepare2Dpatches(cell_data, dof, unknowns, 0.0, description) 
  else: # Tested above that it can only be 2 or 3
    grid = prepare3Dpatches(cell_data, dof, unknowns, description) 

  print( "Parsing complete" )

  return grid
      

class Visualiser(object):
  """
    The visualiser is first and foremost a persistency layer around 
    datasets. It does not work on the command line.
  """
  
  def __init__(self, file_name ):
    """
    
     file_name: String
       Name of a Peano patch file. This has to be a meta file, i.e. a file which 
       does not hold actual data but hosts data sets that in turn link to actual
       data. 
        
    """
    self.file_name      = file_name
    self._tp            = None
    self.dataset_number = 0
    self.filter         = []
    self.identifier     = ""
    self.data           = None
    
    
  def display(self, invoked_on_command_line = False):
    """
    
      Should be called only once
      
    """
    self._tp = TrivialProducer()
    if self.data!=None:
      #
      # Aber ansonsten sollte man schon auch was sehen oder Feedback bekommen
      #
      self._tp.GetClientSideObject().SetOutput(self._data)
      Show(self._tp)


  def select_dataset(self, number):
    """
      number: int
    """
    self.dataset_number = number
    self.reload()
  
  
  def append_filter(self, filter):
    """
     
    """
    self.filter.append(filter)
    self.reload()
    
    
  def remove_filters(self):
    """
     
    """
    self.filter = []
    self.reload()
    
    
  def reload(self):
    if self._tp != None:
      self._data = render_dataset( 
        self.file_name,
        self.identifier,
        self.dataset_number,
        False, # display_as_tree
        self.filter
      )
      self._tp.GetClientSideObject().SetOutput(self._data)
      Show(self._tp)
      print( "Please press the play button to update your pipeline" )
      # 
      #visualiser._tp.GetClientSideObject().Update()
      
