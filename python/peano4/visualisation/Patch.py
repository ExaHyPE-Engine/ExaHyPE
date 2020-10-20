# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

class Patch(object):
  """
    Patch class that contains offset, size and value of a Peano patch. 
    
    Attributes:  
    ----------
    offset: Tuple
      Offset of the patch from origin of coordinate system
      
    size: Tuple
      Size of patch
      
    values: List
      List of values in the patch
      
    subdomain: integer
      Integer value which identifies from which subdomain data stems from    
      
    Parameters:
    ----------
    file_path: String
      String of input file path (including filename and extension)
      Not here anymore. @todo update
    
  """
  
  def __init__(self, offset, size, values, subdomain_number=0):
    self.offset    = offset
    self.size      = size
    self.values    = values
    self.subdomain_number = subdomain_number
      
