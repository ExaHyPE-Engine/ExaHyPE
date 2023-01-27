# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import os

from .DoF                import DoFAssociation
from .PatchToDoubleArray import FloatTypes
from .PatchToDoubleArray import PatchToDoubleArray 


class PatchToDoubleArrayOnHeap(PatchToDoubleArray):
  """
  
  Very simple converter which maps the patch 1:1 onto a double array. 
  By default, I leave all the MPI merges empty though obviously the 
  merges often are plain copying over along faces (see the discussion 
  on Finite Volumes in the guidebook). If you need any proper MPI 
  handling, use merge_method_definition to inject it. Depending
  on how you use your patch (as cell or face or vertex data structure)
  the thing you pass has the respective semantics. The most popular
  default merge implementation is the one you find in the blockstructured
  toolbox.
  
  """
  def __init__(self,patch, float_type="double"):
    """
    
      includes  Includes to be added to the generated file. This is something
      totally different than get_header_file_include(). You will need to set
      something here if your send/receive conditions, e.g., call other stuff.
      
    """
    super( PatchToDoubleArrayOnHeap, self ).__init__(patch, float_type)


  def get_stack_container(self):
    return "peano4::stacks::STDVectorStack< " + self.data.get_full_qualified_type() + " >";    

    
  def construct_output(self,output):
    d = self._get_dictionary_for_output()
    """
      Pass in a version of output
    """
    output.makefile.add_cpp_file( self.data.namespace[-1] + "/" + self.data.name + ".cpp" )
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix+".template.h",
      templatefile_prefix+".template.cpp",
      self.data.name, 
      self.data.namespace,
      self.data.namespace[-1],
      d, 
      True)
    output.add(generated_files)

