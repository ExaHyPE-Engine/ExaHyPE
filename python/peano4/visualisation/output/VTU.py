# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Visualiser import Visualiser

try:
    from paraview.simple import *
except ImportError:
    print("Unable to import paraview")

try:
    import vtk
except ImportError:
    print("Unable to import vtk")

import traceback




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



#  if dimensions == 2 and display_as_tree:
#    grid = prepare2Dpatches(cell_data, dof, unknowns, 1.0, description, is_data_associated_to_cell, mapping, verbose ) 
#  elif dimensions == 2 and not display_as_tree:
#    grid = prepare2Dpatches(cell_data, dof, unknowns, 0.0, description, is_data_associated_to_cell, mapping, verbose) 
#  else: # Tested above that it can only be 2 or 3
#    grid = prepare3Dpatches(cell_data, dof, unknowns, description, is_data_associated_to_cell, mapping, verbose) 
#
#  return grid
      

class VTU(Visualiser):
  """
    The visualiser is first and foremost a persistency layer around 
    datasets. It does not work on the command line.
  """
  
  def __init__(self, file_name, output_directory=".", strip_relative_pathes=True, verbose=False ):
    super(VTU,self).__init__(file_name, verbose)
    self._tp   = None
    self._grid = None
    self.output_directory = output_directory
    self.display_as_tree = False
    self._strip_relative_pathes = strip_relative_pathes
    
    
  def display(self):
    """
    
      Should be called only once
      
    """
    pvd_file = """<?xml version="1.0"?>
<VTKFile type="Collection" version="0.1"
         byte_order="LittleEndian"
         compressor="vtkZLibDataCompressor">
  <Collection>
"""    
    file_name = self._file_name.replace( ".peano-patch-file", "" )
    i = 0;
    try:
      last_file = False
      while not last_file:
        print( "==================================")
        print( "Process snapshot " + str(i) )
        print( "==================================")
        self.select_dataset(i)
        if self._grid==None:
          last_file = True
        else:
          snapshot_file_name = self._file_name + "-" + str(i) + ".vtu"
          self.write_vtu( self.output_directory + "/" + snapshot_file_name )
          link_file_name = snapshot_file_name
          if self._strip_relative_pathes:
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
    meta_file = open( self.output_directory + "/" + file_name + ".pvd", "w" )
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

    
  def reload(self):
    """
     
     Invokes superclass' reload, converts data into VTU, and then sets 
     the resulting self._data as output of the client side object of 
     the trivial producer. Does not work if self._tp is set to None.
    
    """
    super(VTU,self).reload()

    if self._cell_data!=[]:
      if self._dimensions == 2 and self.display_as_tree:
        self._grid = prepare2Dpatches(self._cell_data, self._dof, self._unknowns, 1.0, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
      elif self._dimensions == 2 and not self.display_as_tree:
        self._grid = prepare2Dpatches(self._cell_data, self._dof, self._unknowns, 0.0, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
      else: # Tested above that it can only be 2 or 3
        self._grid = prepare3Dpatches(self._cell_data, self._dof, self._unknowns, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
    else:
      self._grid = None
  

  def write_vtu(self,file_name):
    if not file_name.endswith( ".vtu"):
      print( "Append .vtu extension to " + file_name)
      file_name += ".vtu"
    writer = vtk.vtkXMLUnstructuredGridWriter()
    writer.SetFileName( file_name )
    writer.SetInputData( self._grid )
    writer.Write()
    #
    # explicitly destroy object to speed up things
    #
    print( "Wrote file " + file_name )
    del writer

