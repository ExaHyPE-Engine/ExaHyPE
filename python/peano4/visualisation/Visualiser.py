# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.visualisation.OutputFileParser import OutputFileParser
from paraview.simple import *




def prepare2Dpatches(cell_data, num_patches, dof, unknowns, depth_scaling = 1.0):
  """
    Prepare 2D patches in grid for rendering 
    
    Parameters:
    ----------
    cell_data: list of patches
      List of Patches with file data 
    num_patches: Integer
      Number of patches in tree
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
  grid.GetCellData().SetScalars(data)

  count = 0

  for p in range(num_patches):

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
    
        x_0 = patch_x_0 + k*cell_length
        y_0 = patch_y_0 + l*cell_height
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
          
        data.SetNumberOfComponents(unknowns)
        data.InsertNextTuple(cell_data[p].values[cell_number*unknowns:cell_number*unknowns+unknowns]) #density, velocity x, y, z, energy per set  
        data.SetName("Cell Data (density, velocity x, y, z, energy)")                  
        count = count+4

  grid.SetPoints(points)
  grid.SetCells(paraview.vtk.VTK_PIXEL, cells)

  return grid



def prepare3Dpatches(cell_data, num_patches, dof, unknowns):
  """
    Prepare 3D patches in grid for rendering 
    
    Parameters:
    ----------
    cell_data: list of patches
      List of Patches with file data 
    num_patches: Integer
      Number of patches in tree
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
  grid.GetCellData().SetScalars(data)

  count = 0

  for p in range(num_patches):

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
    
          cell_number = k*dof + l
    
          x_0 = patch_x_0 + k*cell_length
          y_0 = patch_y_0 + l*cell_height
          z_0 = patch_z_0 + m*cell_depth
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
          
          data.SetNumberOfComponents(unknowns)
          data.InsertNextTuple(cell_data[p].values[cell_number*unknowns:cell_number*unknowns+unknowns]) #density, velocity x, y, z, energy per set  
          data.SetName("Cell Data (density, velocity x, y, z, energy)")                  
          count = count+8

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
  
  parser = OutputFileParser(filename)
  cell_data, dimensions, dof, unknowns = parser.parse_file(identifier)
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
      cell_data, num_patches, dof, unknowns, dimensions = p.render(cell_data, num_patches, dof, unknowns, dimensions)

  if dimensions == 2 and display_as_tree:
    grid = prepare2Dpatches(cell_data, num_patches, dof, unknowns) 
  elif dimensions == 2 and not display_as_tree:
    grid = prepare2Dpatches(cell_data, num_patches, dof, unknowns, 0.0) 
  else: # Tested above that it can only be 2 or 3
    grid = prepare3Dpatches(cell_data, num_patches, dof, unknowns) 
  
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
  
  for line in lines:
    if "begin dataset" in line:
      dataset_number = dataset_number-1
    if "include" in line and dataset_number==0:
      snapshot_file_name = line.split( "\"" )[1]
      print( "parse file ", snapshot_file_name )

      parser = OutputFileParser(snapshot_file_name)
      snapshot_cell_data, snapshot_dimensions, snapshot_dof, snapshot_unknowns = parser.parse_file(identifier)

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
      
      print("Dimensions: ", dimensions)
      print("Number of variables per cell: ", unknowns)
      print("Number of cells per patch per direction: ", dof)
 
      snapshot_num_patches = len(snapshot_cell_data)
      print("Number of patches:", snapshot_num_patches)
  
      if filter!=None:
        for p in filter:
          snapshot_cell_data, num_patches, dof, unknowns, dimensions = p.render(snapshot_cell_data, snapshot_num_patches, dof, unknowns, dimensions)

      cell_data =  cell_data + snapshot_cell_data

  num_patches = len(cell_data)
  print("Total number of patches:", num_patches)

  if filter!=None:
    for p in filter:
     cell_data, num_patches, dof, unknowns, dimensions = p.render(cell_data, num_patches, dof, unknowns, dimensions)

  if dimensions == 2 and display_as_tree:
    grid = prepare2Dpatches(cell_data, num_patches, dof, unknowns) 
  elif dimensions == 2 and not display_as_tree:
    grid = prepare2Dpatches(cell_data, num_patches, dof, unknowns, 0.0) 
  else: # Tested above that it can only be 2 or 3
    grid = prepare3Dpatches(cell_data, num_patches, dof, unknowns) 
  
  return grid
      
