# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import sys

from peano4.visualisation.Patch import Patch

from paraview.simple import *


class OutputFileParser(object):
  """
    Parser for Peano block file output. 
    
    Parameters:
    ----------
    file_path: String
      String of input file path (including filename and extension)
      
    Attributes:  
    ----------
    file_path: String
      String of input file path (including filename and extension)
    
  """
  
  def __init__(self,file_path):
    self.file_path = file_path
    self.dof = 0
    self.unknowns = 0
    self.description = ""
    
    
  def __parse_meta_data_line(self, line):
    if "number-of-unknowns" in line:
      self.unknowns = int(line.strip().split()[1])
    if "number-of-dofs-per-axis" in line:
      self.dof = int(line.strip().split()[1])   
    if "description" in line:
      self.description = line.strip().split("description")[1]
      pass   

    
  def parse_file(self, set_identifier, subdomain_number=0):
    """
      Read file and return cell data, dimensions, number of degrees of freedom and number of unknowns
          
      Parameters:
      ----------
      set_identifier: String
        Name of the set of unknowns we want to extract. If this is the empty string
        then I parse all content.
      
      Returns:
      ---------- 
      cell_data: list of patches
        List of Patches with file data
        
      dimensions: Integer
        Dimension of domains
        
      dof: Integer
        Number of degrees of freedom per axis
        
      unknowns: Integer
        Number of unknowns per patch volume    
        
    """  
    cell_data = []
    dimensions = 0
    
    print("Reading "+ self.file_path + " as subdomain " + str(subdomain_number) )
    
    with open(self.file_path, 'r') as data_file:
            
      for this_line in data_file:
        this_line = this_line.strip()
          
        if this_line.startswith("dimensions"):
          dimensions = int(this_line.strip().split()[1])
          print("Dim:", dimensions)
        
        #Read out meta data  
        if this_line.startswith("begin cell-metadata") and ( set_identifier=="" or this_line.endswith('"'+set_identifier+'"' )):
          line = data_file.readline().strip()
          while not "end cell-metadata" in line:
            self.__parse_meta_data_line( line )
            line = data_file.readline().strip()
          
        elif this_line.startswith("begin patch"):
        
          #Get patch offset
          this_line = data_file.readline().strip()
          line = this_line.strip().split()           
          if dimensions == 2:
            offset = (float(line[1]), float(line[2]))
          elif dimensions == 3:
            offset =  (float(line[1]), float(line[2]), float(line[3]))
          else:
            print("Dimension not supported")   
            break
          
          #Get patch size  
          this_line = data_file.readline().strip()
          line = this_line.strip().split() 
          if dimensions == 2:
            size = (float(line[1]), float(line[2]))
          elif dimensions == 3:
            size =  (float(line[1]), float(line[2]), float(line[3]))      
          else:
            print("Dimension not supported")    
            break
                      
             
          #Get patch data  
          this_line = data_file.readline().strip()
          if this_line.startswith("begin cell-values") and ( set_identifier=="" or this_line.endswith('"'+set_identifier+'"')):
            this_line = data_file.readline()
            values = np.fromstring(this_line, dtype=float, sep=' ')
            
          #Add patch to list
          cell_data.append(Patch(offset, size, values, subdomain_number))  
      
        #else:
          #Do nothing
           
      
      
    return cell_data, dimensions, self.dof, self.unknowns, self.description
 
