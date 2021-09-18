# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.visualisation.OutputFileParser import OutputFileParser

try:
    from paraview.simple import *
except ImportError:
    print("Unable to import paraview")

try:
    import vtk
except ImportError:
    print("Unable to import vtk")

import traceback

import multiprocessing



def prepare2Dpatches(cell_data, dof, unknowns, depth_scaling, description, is_data_associated_to_cell, mapping, verbose):
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

  for p in range( len(cell_data) ):
    patch_x_0 = cell_data[p].offset[0]
    patch_y_0 = cell_data[p].offset[1]
    patch_x_1 = cell_data[p].offset[0] + cell_data[p].size[0]
    patch_y_1 = cell_data[p].offset[1] + cell_data[p].size[1]

    z = cell_data[p].size[0] * depth_scaling

    volumes_in_patch_per_axis = dof
    if not is_data_associated_to_cell:
      volumes_in_patch_per_axis -= 1
      
    mapping_count = 0
    for k in range(volumes_in_patch_per_axis+1):
      for l in range(volumes_in_patch_per_axis+1):
        x = patch_x_0 + mapping[mapping_count]*cell_data[p].size[0]
        mapping_count+=1
        y = patch_y_0 + mapping[mapping_count]*cell_data[p].size[1]
        mapping_count+=1
        points.InsertNextPoint([x, y, z])
          
    for k in range(volumes_in_patch_per_axis):
      for l in range(volumes_in_patch_per_axis):
        this_cell = cells.InsertNextCell(4)    
        cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+0)*(volumes_in_patch_per_axis+1)+l+0)
        cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+0)*(volumes_in_patch_per_axis+1)+l+1)
        cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+1)*(volumes_in_patch_per_axis+1)+l+0)
        cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+1)*(volumes_in_patch_per_axis+1)+l+1)
        
    if is_data_associated_to_cell:
      for k in range(volumes_in_patch_per_axis):
        for l in range(volumes_in_patch_per_axis):
          cell_number = k*dof + l
          new_data = [x for x in cell_data[p].values[cell_number*unknowns:cell_number*unknowns+unknowns] ]
          new_data.append(cell_data[p].subdomain_number)
          data.InsertNextTuple(new_data)
    else:
      for k in range(volumes_in_patch_per_axis+1):
        for l in range(volumes_in_patch_per_axis+1):
          vertex_number = k*(volumes_in_patch_per_axis+1) + l
          new_data = [x for x in cell_data[p].values[vertex_number*unknowns:vertex_number*unknowns+unknowns] ]
          new_data.append(cell_data[p].subdomain_number)
          data.InsertNextTuple(new_data)
  
  if is_data_associated_to_cell:
    grid.GetCellData().SetScalars(data)
  else:
    grid.GetPointData().SetScalars(data)
  grid.SetPoints(points)
  grid.SetCells(paraview.vtk.VTK_PIXEL, cells)

  return grid



def prepare3Dpatches(cell_data, dof, unknowns, description, is_data_associated_to_cell, mapping, verbose):
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

  for p in range(len(cell_data)):

    patch_x_0 = cell_data[p].offset[0]
    patch_y_0 = cell_data[p].offset[1]
    patch_z_0 = cell_data[p].offset[2]
    patch_x_1 = cell_data[p].offset[0] + cell_data[p].size[0]
    patch_y_1 = cell_data[p].offset[1] + cell_data[p].size[1]
    patch_z_1 = cell_data[p].offset[2] + cell_data[p].size[2]
    
    volumes_in_patch_per_axis = dof
    if not is_data_associated_to_cell:
      volumes_in_patch_per_axis -= 1

    mapping_count = 0
    for k in range(volumes_in_patch_per_axis+1):
      for l in range(volumes_in_patch_per_axis+1):
        for m in range(volumes_in_patch_per_axis+1):
          x = patch_x_0 + mapping[mapping_count]*cell_data[p].size[0]
          mapping_count+=1
          y = patch_y_0 + mapping[mapping_count]*cell_data[p].size[1]
          mapping_count+=1
          z = patch_z_0 + mapping[mapping_count]*cell_data[p].size[2]
          mapping_count+=1
          points.InsertNextPoint([x, y, z])

    for k in range(volumes_in_patch_per_axis):
      for l in range(volumes_in_patch_per_axis):
        for m in range(volumes_in_patch_per_axis):
          this_cell = cells.InsertNextCell(8)    
          cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+0)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(l+0)*(volumes_in_patch_per_axis+1)+m+0)
          cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+0)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(l+0)*(volumes_in_patch_per_axis+1)+m+1)
          cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+0)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(l+1)*(volumes_in_patch_per_axis+1)+m+0)
          cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+0)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(l+1)*(volumes_in_patch_per_axis+1)+m+1)
          cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(l+0)*(volumes_in_patch_per_axis+1)+m+0)
          cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(l+0)*(volumes_in_patch_per_axis+1)+m+1)
          cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(l+1)*(volumes_in_patch_per_axis+1)+m+0)
          cells.InsertCellPoint(p*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(k+1)*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1)+(l+1)*(volumes_in_patch_per_axis+1)+m+1)
          
          
    if is_data_associated_to_cell:
      for k in range(volumes_in_patch_per_axis):
        for l in range(volumes_in_patch_per_axis):
          for m in range(volumes_in_patch_per_axis):
            cell_number = k*dof*dof + l*dof + m
            new_data = [x for x in cell_data[p].values[cell_number*unknowns:cell_number*unknowns+unknowns] ]
            new_data.append(cell_data[p].subdomain_number)
            data.InsertNextTuple(new_data)
    else: 
      for k in range(volumes_in_patch_per_axis+1):
        for l in range(volumes_in_patch_per_axis+1):
          for m in range(volumes_in_patch_per_axis+1):
            vertex_number = k*(volumes_in_patch_per_axis+1)*(volumes_in_patch_per_axis+1) + l*(volumes_in_patch_per_axis+1) + m
            new_data = [x for x in cell_data[p].values[vertex_number*unknowns:vertex_number*unknowns+unknowns] ]
            new_data.append(cell_data[p].subdomain_number)
            data.InsertNextTuple(new_data)
             

  if is_data_associated_to_cell:
    grid.GetCellData().SetScalars(data)
  else:
    grid.GetPointData().SetScalars(data)
  grid.SetPoints(points)
  grid.SetCells(paraview.vtk.VTK_VOXEL, cells)

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
  cell_data, dof, dimensions, unknowns, description, is_data_associated_to_cell, mapping = parser.cell_data, parser.dof, parser.dimensions, parser.unknowns, parser.description, parser.is_data_associated_to_cell, parser.mapping
  
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
      cell_data, dof, dimensions, unknowns = p.render(cell_data, dof, dimensions, unknowns)

  if dimensions == 2 and display_as_tree:
    grid = prepare2Dpatches(cell_data, dof, unknowns, 1.0, description, is_data_associated_to_cell, mapping, verbose) 
  elif dimensions == 2 and not display_as_tree:
    grid = prepare2Dpatches(cell_data, dof, unknowns, 0.0, description, is_data_associated_to_cell, mapping, verbose) 
  else: # Tested above that it can only be 2 or 3
    grid = prepare3Dpatches(cell_data, dof, unknowns, description, is_data_associated_to_cell, mapping, verbose) 
  
  return grid


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
    

def render_dataset(filename, identifier, dataset_number=0, display_as_tree = True, filter=None, verbose=False):
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
      snapshot_cell_data, snapshot_dof, snapshot_dimensions, snapshot_unknowns, description, mapping = parser.cell_data, parser.dof, parser.dimensions, parser.unknowns, parser.description, parser.mapping
      for p in filter:
        if p.run_on_individual_pieces_of_data:
          snapshot_cell_data, snapshot_dof, snapshot_unknowns, dimensions, mapping = p.render(snapshot_cell_data, snapshot_dof, snapshot_unknowns, dimensions, mapping)
      parser.cell_data, parser.dof, parser.dimensions, parser.unknowns, parser.description, parser.mapping = snapshot_cell_data, snapshot_dof, snapshot_dimensions, snapshot_unknowns, description, mapping


  input_file = open( filename, "r" )
  lines = input_file.readlines()
  
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
      if verbose:        
        print( "parse file ", snapshot_file_name )

      parser = OutputFileParser(snapshot_file_name,identifier, include_file_counter)
      parsers.append( parser )
      include_file_counter += 1

  if verbose:        
    print( "Will invoke " + str( len(parsers) ) + " parsers in parallel" )
    
  if len(parsers)==0:
    if verbose:        
      print( "Snapshot is empty. Return" )
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
    
  if verbose:        
    print( "All individual files are parsed" )
    print( "Apply filter(s) to individual files (where appropriate)" )
    
  for parser in parsers:
    for p in filter:
      if p.run_on_individual_pieces_of_data:
        print( "apply filter " + str(p) + " to snapshot")
        parser.cell_data, parser.dof, parser.dimensions, parser.unknowns, parser.description, parser.mapping = p.render(parser.cell_data, parser.dof, parser.dimensions, parser.unknowns, parser.description, parser.mapping)

  cell_data  = []
  dof, dimensions, unknowns, description, is_data_associated_to_cell, mapping = validate(parsers)
      
  if verbose:        
    print( "Concatenate snapshots" )

  for parser in parsers:
    cell_data =  cell_data + parser.cell_data

  num_patches = len(cell_data)
  print("Total number of patches:", num_patches)


  dof, dimension, unknowns, description, is_data_associated_to_cell, mapping = validate(parsers)
  print( "Parsing complete. Convert set of " + str(len(cell_data)) + " patches with " + str(unknowns) + " unknowns per " + str(dof) + "^" + str(dimensions) + " patch/cell into VTK data structures" )
  
  if filter!=None:
    if verbose:        
      print( "Apply " + str(len(filter)) + " filter(s) to concatenated data set" )
    for p in filter:
     if p.run_on_concatenated_data:
       print( "apply filter " + str(p) )
       cell_data, dof, dimensions, unknowns, description, mapping = p.render(cell_data, dof, dimensions, unknowns, description, mapping)

  if dimensions == 2 and display_as_tree:
    grid = prepare2Dpatches(cell_data, dof, unknowns, 1.0, description, is_data_associated_to_cell, mapping, verbose ) 
  elif dimensions == 2 and not display_as_tree:
    grid = prepare2Dpatches(cell_data, dof, unknowns, 0.0, description, is_data_associated_to_cell, mapping, verbose) 
  else: # Tested above that it can only be 2 or 3
    grid = prepare3Dpatches(cell_data, dof, unknowns, description, is_data_associated_to_cell, mapping, verbose) 

  return grid
      

class Visualiser(object):
  """
    The visualiser is first and foremost a persistency layer around 
    datasets. It does not work on the command line.
  """
  
  def __init__(self, file_name, verbose=False ):
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
    self.verbose        = verbose
    
    
  def display(self, invoked_on_command_line = False):
    """
    
      Should be called only once
      
    """
    if self._tp==None:
      self._tp = TrivialProducer()
      self.reload()


  def select_dataset(self, number):
    """
      number: int
    """
    self.dataset_number = number
    self.reload()
  
  
  def append_filter(self, filter, reload=True):
    """
     
    """
    self.filter.append(filter)
    if reload:
      self.reload()
    
    
  def remove_filters(self):
    """
     
    """
    self.filter = []
    self.reload()
    
    
  def reload(self):
    """
     
     Invokes render_dataset() and then sets the resulting self._data
     as output of the client side object of the trivial producer. Does
     not work if self._tp is set to None
    
    """
    self._data = render_dataset( 
      self.file_name,
      self.identifier,
      self.dataset_number,
      False, # display_as_tree
      self.filter,
      self.verbose
    )
    if self._tp != None and self._data!=None:
      self._tp.GetClientSideObject().SetOutput(self._data)
      Show(self._tp)
      print( "Please press the play button to update your pipeline" )
      # 
      #visualiser._tp.GetClientSideObject().Update()
      

  def write_vtu(self,file_name):
    if not file_name.endswith( ".vtu"):
      print( "Append .vtu extension to " + file_name)
      file_name += ".vtu"
    writer = vtk.vtkXMLUnstructuredGridWriter()
    writer.SetFileName( file_name )
    writer.SetInputData( self._data )
    writer.Write()
    #
    # explicitly destroy object to speed up things
    #
    print( "Wrote file " + file_name )
    del writer


  def write_vtu_time_series(self,strip_relative_pathes):
    pvd_file = """<?xml version="1.0"?>
<VTKFile type="Collection" version="0.1"
         byte_order="LittleEndian"
         compressor="vtkZLibDataCompressor">
  <Collection>
"""    
    file_name = self.file_name.replace( ".peano-patch-file", "" )
    i = 0;
    try:
      last_file = False
      while not last_file:
        print( "==================================")
        print( "Process snapshot " + str(i) )
        print( "==================================")
        self.select_dataset(i)
        if self._data==None:
          last_file = True
        else:
          snapshot_file_name = file_name + "-" + str(i) + ".vtu"
          self.write_vtu( snapshot_file_name )
          link_file_name = snapshot_file_name
          if strip_relative_pathes:
            link_file_name = link_file_name.split( "/" )[-1]
          pvd_file += """
    <DataSet timestep=\"""" + str(i) + """\" group="" part="0" file=\"""" + link_file_name + """\" />
"""
          i+=1
    except Exception as e:
      print( "Got an exception: " + str(e))
      traceback.print_exc()
      pass
    print( "dumped " + str(i) + " datasets" ) 
    pvd_file += """
  </Collection>
</VTKFile>
"""
    meta_file = open( file_name + ".pvd", "w" )
    meta_file.write( pvd_file )
    
    if "/" in file_name and not strip_relative_pathes:
      print( """
WARNING: The conversion tool has processed a dataset stored in a different
directory. If the resulting .pvd file (""" + file_name + """.pvd)
contains relative pathes, you will have to copy it into the present working
directory before you load the outcome into Paraview. Otherwise, Paraview 
will not be able to locate your actual data files.

Alternatively, you can rerun the postprocessing and eliminate relative 
pathes (see documentation/help of your conversion script).
""" )
