# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import numpy as np
import sys


class TopologyParser(object):
  """
    Parser for Peano block file output. 
    
    Attributes:
    ----------
     
      file_path: String
      String of input file path (including filename and extension)

      cell_data: list of patches
        List of Patches with file data
        

    
  """
  
  def __init__(self,filepath):
    self.filepath = filepath
    self.ncols = 0
    self.nrows = 0
    self.xlowerleft = 0.0
    self.ylowerleft = 0.0
    self.cellsize = 0.0
    self.nodatavalue = 0.0   
    self.data = []
    
   
  def parse_topo(self):
    """
      Read in topology data from file
          
      Parameters:
      ----------
      
        
    """  
    print("Reading "+ self.filepath )
    
    alldata = []
    count = 0
    
    with open(self.filepath, 'r') as data_file:
      for this_line in data_file:
        this_line = this_line.strip()
        #print(this_line)
          
        if this_line.endswith("ncols"):
          self.ncols = int(this_line.split()[0])
          
        elif this_line.endswith("nrows"):
          self.nrows = int(this_line.split()[0])          
        
        elif this_line.endswith("xll"):
          self.xlowerleft = float(this_line.split()[0])          
               
        elif this_line.endswith("yll"):
          self.ylowerleft = float(this_line.split()[0])          
           
        elif this_line.endswith("cellsize"):
          self.cellsize = float(this_line.split()[0])               
           
        elif this_line.endswith("nodata_value"):
          self.nodatavalue = float(this_line.split()[0])          
                 
        elif len(this_line.split()) == 1:
          alldata.append(float(this_line))
          count = count+1
        
        else:
          print( "Warning: Cannot parse line \n" + this_line)    
        
    self.data = np.reshape(alldata, (self.nrows, self.ncols))
