# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Visualiser import Visualiser

import traceback      

class PatchFile(Visualiser):
  """
    The visualiser is first and foremost a persistency layer around 
    datasets. It does not work on the command line.
  """
  
  def __init__(self, file_name, output_directory=".", verbose=False ):
    super(PatchFile,self).__init__(file_name, verbose)
    self.output_directory = output_directory
    if output_directory==".":
      print( "WARNING: Output directory is current directory, i.e. might overwrite data" )
    
    
  def display(self):
    """
    
      Should be called only once
      
    """
    meta_file_content = """
# 
# Peano patch file 
# Version 0.2 
# 
format ASCII 

"""
    
    i = 0;
    try:
      last_file = False
      while not last_file:
        print( "==================================")
        print( "Process snapshot " + str(i) )
        print( "==================================")
        self.select_dataset(i)
        if self._cell_data==[]:
          last_file = True
        else:
          snapshot_file_name = self._file_name.replace( ".peano-patch-file", "-" + str(i) + ".peano-patch-file" )
          self.write_patch_file( self.output_directory + "/" + snapshot_file_name )
          link_file_name = snapshot_file_name
          meta_file_content += """
begin dataset
  timestamp  \"""" + self._timestamp + """\"

  include \"""" + snapshot_file_name + """\"
end dataset

"""
          i+=1
    except Exception as e:
      print( "Got an exception: " + str(e))
      traceback.print_exc()
      pass
    print( "dumped " + str(i) + " datasets" ) 
    meta_file = open( self.output_directory + "/" + self._file_name, "w" )
    meta_file.write( meta_file_content )
    
    
  #def reload(self):
  #  """
  #   
  #   Invokes superclass' reload, converts data into VTU, and then sets 
  #   the resulting self._data as output of the client side object of 
  #   the trivial producer. Does not work if self._tp is set to None.
  #  
  #  """
 #   super(PatchFile,self).reload()
#
#    if self._cell_data!=[]:
#      if self._dimensions == 2 and self.display_as_tree:
#        self._grid = prepare2Dpatches(self._cell_data, self._dof, self._unknowns, 1.0, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
#      elif self._dimensions == 2 and not self.display_as_tree:
#        self._grid = prepare2Dpatches(self._cell_data, self._dof, self._unknowns, 0.0, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
#      else: # Tested above that it can only be 2 or 3
#        self._grid = prepare3Dpatches(self._cell_data, self._dof, self._unknowns, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
#    else:
#      self._grid = None


  def write_patch_file( self, file_name ):
    out_file = open( file_name, "w" )
    out_file.write( """
# 
# Peano patch file 
# Version 0.2 
# 
format ASCII
dimensions """ + str(self._dimensions) + """


""")

    if self._is_data_associated_to_cell:
      out_file.write( """
begin cell-metadata \"""" + self.identifier + """\"
""" )  
    else:
      out_file.write( """
begin vertex-metadata \"""" + self.identifier + """\"
""" )  
      
    out_file.write( """
  number-of-unknowns """ + str(self._unknowns) + """
  number-of-dofs-per-axis """ + str(self._dof) + """

""")
    
    if self._is_data_associated_to_cell:
      out_file.write( """
end cell-metadata 
""" )  
    else:
      out_file.write( """
end vertex-metadata 
""" )  


    for p in self._cell_data:
      out_file.write( """
begin patch""" )  
      out_file.write( "\n  offset ")
      for i in range(0,self._dimensions):
        out_file.write( " " + str(p.offset[i]))
      out_file.write( "\n  size")
      for i in range(0,self._dimensions):
        out_file.write( " " + str(p.size[i]))
      out_file.write( "\n  " )
      if self._is_data_associated_to_cell:
        out_file.write( "begin cell-values \"" + self.identifier + "\" \n")  
      else:
        out_file.write( "begin vertex-values \"" + self.identifier + "\" \n")  

      total_number_of_cells_per_patch = self._dof * self._dof
      if self._dimensions==3:
        total_number_of_cells_per_patch *= self._dof
       
      for k in range(0,total_number_of_cells_per_patch * self._unknowns):
        out_file.write( " " + str(p.values[k]))
      
      if self._is_data_associated_to_cell:
        out_file.write( "\n  end cell-values \n")  
      else:
        out_file.write( "\n  end vertex-values \n")  
      out_file.write( """
end patch
""" )  
