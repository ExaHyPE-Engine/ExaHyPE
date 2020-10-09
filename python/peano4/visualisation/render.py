from paraview.simple import *
import OutputFileParser as ofp
import sys

def prepare2Dpatches(cell_data, num_patches, dof, unknowns):
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
    z = cell_data[p].size[0]

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
        data.InsertNextTuple(cell_data[p].values[cell_number*unknowns:cell_number*unknowns+5]) #density, velocity x, y, z, energy per set  
        data.SetName("Cell Data (density, velocity x, y, z, energy)")                  
        count = count+4

  grid.SetPoints(points)
  grid.SetCells(paraview.vtk.VTK_PIXEL, cells)

  return grid

def render3Dpatches(cell_data, num_patches, dof, unknowns):
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
          data.InsertNextTuple(cell_data[p].values[cell_number*unknowns:cell_number*unknowns+5]) #density, velocity x, y, z, energy per set  
          data.SetName("Cell Data (density, velocity x, y, z, energy)")                  
          count = count+8

  grid.SetPoints(points)
  grid.SetCells(paraview.vtk.VTK_VOXEL, cells)

  return grid




def renderFile(filename):
  """
    Parse peano-patch-file and render data 
    
    Parameters:
    ----------
    filename: String
      Path to file to be parsed 
  """
  
  parser = ofp.OutputFileParser(filename)
  cell_data, dimensions, dof, unknowns = parser.parse_file("EulerQ")
  if (dimensions != 2 and dimensions != 3) or dof == 0 or unknowns == 0:
    print("File parsing unsuccessful.")
    return 

  print("Dimensions: ", dimensions)
  print("Number of variables per cell: ", dof)
  print("Number of cells per patch: ", unknowns)

  num_patches = len(cell_data)
  print("Number of patches:", num_patches)

  if dimensions == 2:
    grid = prepare2Dpatches(cell_data, num_patches, dof, unknowns) 
  else: # Tested above that it can only be 2 or 3
    grid = prepare3Dpatches(cell_data, num_patches, dof, unknowns) 
  
  tp = TrivialProducer()
  tp.GetClientSideObject().SetOutput(grid)
  Show(tp)
  #Interact() #Needed if running from command line
  #WriteImage("Output.png")
  return


filename = "./example_output/solution-Euler-tree-0-0.peano-patch-file"
renderFile(filename)

