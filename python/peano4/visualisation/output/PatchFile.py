# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Visualiser import Visualiser
      

class PatchFile(Visualiser):
  """
    The visualiser is first and foremost a persistency layer around 
    datasets. It does not work on the command line.
  """
  
  def __init__(self, file_name, output_directory=".", verbose=False ):
    super(PatchFile,self).__init__(file_name, verbose)
    self.output_directory = output_directory
    if output_director==".":
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
        if self._grid==None:
          last_file = True
        else:
          snapshot_file_name = self._file_name + "-" + str(i) + ".peano-patch-file"
          #self.write_patch_file( self.output_directory + "/" + snapshot_file_name )
          link_file_name = snapshot_file_name
          if self._strip_relative_pathes:
            link_file_name = link_file_name.split( "/" )[-1]
          meta_file_content += """
begin dataset
  include \"""" + snapshot_file_name + """\"
end dataset

"""
          i+=1
    except Exception as e:
      print( "Got an exception: " + str(e))
      traceback.print_exc()
      pass
    print( "dumped " + str(i) + " datasets" ) 
    meta_file = open( self.output_directory + "/" + file_name + ".pvd", "w" )
    meta_file.write( meta_file_content )
    
    
  def reload(self):
    """
     
     Invokes superclass' reload, converts data into VTU, and then sets 
     the resulting self._data as output of the client side object of 
     the trivial producer. Does not work if self._tp is set to None.
    
    """
    super(VTU,self).reload()

#    if self._cell_data!=[]:
#      if self._dimensions == 2 and self.display_as_tree:
#        self._grid = prepare2Dpatches(self._cell_data, self._dof, self._unknowns, 1.0, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
#      elif self._dimensions == 2 and not self.display_as_tree:
#        self._grid = prepare2Dpatches(self._cell_data, self._dof, self._unknowns, 0.0, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
#      else: # Tested above that it can only be 2 or 3
#        self._grid = prepare3Dpatches(self._cell_data, self._dof, self._unknowns, self._description, self._is_data_associated_to_cell, self._mapping, self.verbose) 
#    else:
#      self._grid = None
  

#  def write_vtu(self,file_name):
#    if not file_name.endswith( ".vtu"):
#      print( "Append .vtu extension to " + file_name)
#      file_name += ".vtu"#
#    writer = vtk.vtkXMLUn#structuredGridWriter()
#    writer.SetFileName( file_name )
#    writer.SetInputData( self._grid )
#    writer.Write()
    #
    # explicitly destroy object to speed up things
    #
#    print( "Wrote file " + file_name )
#    del writer

