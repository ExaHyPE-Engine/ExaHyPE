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
    
    
  def _initialise_default_mapping_if_no_mapping_specified(self):
    volumes_per_axis = self.dof
    if self.is_data_associated_to_cell:
      volumes_per_axis += 1
    if self.mapping==[] and self.dimensions==2:
      for y in range(volumes_per_axis):
        for x in range(volumes_per_axis):
          self.mapping.append(x*1.0/volumes_per_axis)
          self.mapping.append(y*1.0/volumes_per_axis)
    if self.mapping==[] and self.dimensions==3:
      for z in range(volumes_per_axis):
        for y in range(volumes_per_axis):
          for x in range(volumes_per_axis):
            self.mapping.append(x*1.0/volumes_per_axis)
            self.mapping.append(y*1.0/volumes_per_axis)
            self.mapping.append(z*1.0/volumes_per_axis)
    
    
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
      