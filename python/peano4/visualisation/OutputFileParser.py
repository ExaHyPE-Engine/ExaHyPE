# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import sys

from peano4.visualisation.Patch import Patch


class OutputFileParser(object):
  """
    Parser for Peano block file output. 
    
    Attributes:
    ----------
     
      file_path: String
      String of input file path (including filename and extension)

      cell_data: list of patches
        List of Patches with file data
        
      dimensions: Integer
        Dimension of domains
        
      dof: Integer
        Number of degrees of freedom per axis
        
      unknowns: Integer
        Number of unknowns per patch volume    
        
      mapping: series of d-dimensional duples
        Distorts the domain
    
  """
  
  def __init__(self,file_path, set_identifier, subdomain_number ):
    self.file_path = file_path
    self.dof = 0
    self.unknowns = 0
    self.description      = ""
    self.cell_data        = []
    self.is_data_associated_to_cell = None
    self.dimensions       = -1
    self.set_identifier   = set_identifier
    self.subdomain_number = subdomain_number
    self.mapping          = []
    self.parsed           = False    
    self.file_contains_patches = False
    

  def _initialise_default_mapping_if_no_mapping_specified(self):
    vertices_per_axis = self.dof
    if self.is_data_associated_to_cell:
      vertices_per_axis += 1
    if self.mapping==[] and self.dimensions==2:
      for y in range(vertices_per_axis):
        for x in range(vertices_per_axis):
          self.mapping.append(x*1.0/(vertices_per_axis-1))
          self.mapping.append(y*1.0/(vertices_per_axis-1))
    if self.mapping==[] and self.dimensions==3:
      for z in range(vertices_per_axis):
        for y in range(vertices_per_axis):
          for x in range(vertices_per_axis):
            self.mapping.append(x*1.0/(vertices_per_axis-1))
            self.mapping.append(y*1.0/(vertices_per_axis-1))
            self.mapping.append(z*1.0/(vertices_per_axis-1))
    
    
  def __parse_meta_data_line(self, line):
    if "number-of-unknowns" in line:
      self.unknowns = int(line.strip().split()[1])
    if "number-of-dofs-per-axis" in line:
      self.dof = int(line.strip().split()[1])   
    if "description" in line:
      self.description = line.strip().split("description")[1]
      if "\"" in self.description:
        print( "Warning: data description field " + self.description + " holds \" which might lead to issues with VTK's XML export. Remove them" )
        self.description = self.description.replace( "\"", "" )
      pass   
    if "mapping" in line:
      values = line.strip().split("mapping")[1]
      self.mapping = np.fromstring(values, dtype=float, sep=' ')

    
  def parse_file(self):
    """
      Read file and return cell data, dimensions, number of degrees of freedom and number of unknowns
          
      Parameters:
      ----------
      set_identifier: String
        Name of the set of unknowns we want to extract. If this is the empty string
        then I parse all content.
      
        
    """  
    print("Reading "+ self.file_path + " as subdomain " + str(self.subdomain_number) )
    
    try:
     with open(self.file_path, 'r') as data_file:
      for this_line in data_file:
        this_line = this_line.strip()
          
        if this_line.startswith("dimensions"):
          self.dimensions = int(this_line.strip().split()[1])
        
        #Read out meta data  
        if this_line.startswith("begin cell-metadata") and ( self.set_identifier=="" or this_line.endswith('"'+self.set_identifier+'"' )):
          self.is_data_associated_to_cell = True
          line = data_file.readline().strip()
          while not "end cell-metadata" in line:
            self.__parse_meta_data_line( line )
            line = data_file.readline().strip()

        if this_line.startswith("begin vertex-metadata") and ( self.set_identifier=="" or this_line.endswith('"'+self.set_identifier+'"' )):
          self.is_data_associated_to_cell = False
          line = data_file.readline().strip()
          while not "end vertex-metadata" in line:
            self.__parse_meta_data_line( line )
            line = data_file.readline().strip()
          
        elif this_line.startswith("begin patch"):
          self.file_contains_patches = True
          
          #Get patch offset
          this_line = data_file.readline().strip()
          line = this_line.strip().split()           
          if self.dimensions == 2:
            offset = (float(line[1]), float(line[2]))
          elif self.dimensions == 3:
            offset =  (float(line[1]), float(line[2]), float(line[3]))
          else:
            print("Dimension not supported")   
            break
          
          #Get patch size  
          this_line = data_file.readline().strip()
          line = this_line.strip().split() 
          if self.dimensions == 2:
            size = (float(line[1]), float(line[2]))
          elif self.dimensions == 3:
            size =  (float(line[1]), float(line[2]), float(line[3]))      
          else:
            print("Dimension not supported")    
            break
                      
             
          #Get patch data  
          this_line = data_file.readline().strip()
          if this_line.startswith("begin cell-values") and ( self.set_identifier=="" or this_line.endswith('"'+self.set_identifier+'"')):
            this_line = data_file.readline()
            values = np.fromstring(this_line, dtype=float, sep=' ')

          if this_line.startswith("begin vertex-values") and ( self.set_identifier=="" or this_line.endswith('"'+self.set_identifier+'"')):
            this_line = data_file.readline()
            values = np.fromstring(this_line, dtype=float, sep=' ')
            
          #Add patch to list
          self.cell_data.append(Patch(offset, size, values, self.subdomain_number))   
      self._initialise_default_mapping_if_no_mapping_specified()
      self.parsed = True
    except Exception as e:
      print( "Error: was not able to read " + self.file_path + ": " + str(e) )
      self.parsed = False
      

  def get_smallest_patch_size(self):
    smallest = -1
    for patch in self.cell_data:
      if smallest == -1 or smallest > patch.size[0]:
        smallest = patch.size[0]
    return smallest
 
 
  def get_patch_boundaries(self, patch_size):
    patch_boundaries_x = []
    patch_boundaries_y = []
    patch_boundaries_z = []
    for patch in self.cell_data:
      if patch.size[0] == patch_size:
        offset = patch.offset
        if offset[0] not in patch_boundaries_x:
          patch_boundaries_x.append(offset[0])
        if offset[1] not in patch_boundaries_y:
          patch_boundaries_y.append(offset[1])
        if self.dimensions == 3:
          if offset[2] not in patch_boundaries_z:
            patch_boundaries_z.append(offset[2])
    return sorted(patch_boundaries_x), sorted(patch_boundaries_y), sorted(patch_boundaries_z)


  def neighbour_if_gap(self, patch_boundaries, coordinate, patch_size): 
    for i in range(len(patch_boundaries) - 1):
      if (patch_boundaries[i] + patch_size) < coordinate < patch_boundaries[i+1]:
        if (coordinate - (patch_boundaries[i] + patch_size)) <= (patch_boundaries[i+1] - coordinate):
          return patch_boundaries[i] + patch_size
        else:
          return patch_boundaries[i+1]
    return coordinate


  def box_num_within_patch(self, offset, patch_size, x_axis, y_axis, z_axis):
    patch_interval = patch_size / self.dof
    box_number = 0
    x_temp = offset[0]
    for i in range(self.dof):
      if x_temp <= x_axis <= (x_temp + patch_interval):
        break
      else:
        x_temp += patch_interval
        box_number += 1
 
    y_temp = offset[1]
    for i in range(self.dof):
      if y_temp <= y_axis <= (y_temp + patch_interval):
        break
      else:
        y_temp += patch_interval
        box_number += self.dof

    if self.dimensions == 3:
      z_temp = offset[2]
      for i in range(self.dof):
        if z_temp <= z_axis <= (z_temp + patch_interval):
          break
        else:
          z_temp += patch_interval       
          box_number += self.dof * self.dof

    return box_number


  def probe(self, x_axis, y_axis, z_axis=0):
    """
    Calls the parse_file() and returns the unknowns for a given point.
    We probe the smallest patch sizes only.

    :param x_axis:
    Position on x axis where data is to be probed
    :param y_axis:
    Position on y axis where data is to be probed
    :param z_axis:
    Position on z axis where data is to be probed.
    Caller does not need to specify a z axis value if working in 2D
    :return:
    A list of unknowns representing data recorded at point (x,y(,z))
    in the Peano patch file
    """
    if not self.parsed:
      self.parse_file()

    if not self.file_contains_patches:
      print("Empty patch file")
      return []

    patch_size = self.get_smallest_patch_size()

    patch_boundaries_x, patch_boundaries_y, patch_boundaries_z = self.get_patch_boundaries(patch_size)

    errors = []
    if patch_boundaries_x[0] > x_axis or \
        x_axis > (patch_boundaries_x[-1] + patch_size):
      errors.append("Error: X coordinate out of range")
    if patch_boundaries_y[0] > y_axis or \
        y_axis > (patch_boundaries_y[-1] + patch_size):
      errors.append("Error: Y coordinate out of range")
    if self.dimensions == 3:
      if patch_boundaries_z[0] > z_axis or \
          z_axis > (patch_boundaries_z[-1] + patch_size):
        errors.append("Error: Z coordinate out of range")
    if errors:
      print(errors)
      return []

    x_axis = self.neighbour_if_gap(patch_boundaries_x, x_axis, patch_size)
    y_axis = self.neighbour_if_gap(patch_boundaries_y, y_axis, patch_size)
    if self.dimensions == 3:
      z_axis = self.neighbour_if_gap(patch_boundaries_z, z_axis, patch_size)

    for patch in self.cell_data:
      if patch.size[0] == patch_size:
        offset = patch.offset
        if offset[0] <= x_axis <= (offset[0] + patch_size) and \
            offset[1] <= y_axis <= (offset[1] + patch_size) and \
            (self.dimensions != 3 or offset[2] <= z_axis <= (offset[2] + patch_size)):

          box_number = self.box_num_within_patch(offset, patch_size, x_axis, y_axis, z_axis)

          values = []
          for i in range(self.unknowns):
            values.append(patch.values[box_number * self.unknowns + i])

          return values
    
    print("Error probing " + self.file_path)
    return []
