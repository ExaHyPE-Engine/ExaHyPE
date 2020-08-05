# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import os

from .DoF import DoFAssociation


class PatchToDoubleArray():
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
  def __init__(self,patch):
    """
    
      includes  Includes to be added to the generated file. This is something
      totally different than get_header_file_include(). You will need to set
      something here if your send/receive conditions, e.g., call other stuff.
      
    """
    self.data = patch
    self.merge_method_definition     = ""
    self.send_condition              = "true"
    self.receive_and_merge_condition = "true"
    self.store_persistent_condition  = "true"
    self.load_persistent_condition   = "true"
    self.includes                    = ""


  def get_stack_container(self):
    return "peano4::stacks::STDVectorStack< " + self.data.get_full_qualified_type() + " >";

    
  def get_header_file_include(self):
    """
    
     This is the include statement for the data container. It is not something
     directly generated but how to use the generated code. 
     
    """
    return """
#include "peano4/stacks/STDVectorStack.h"    
#include \"""" + self.data.namespace[-1] + "/" + self.data.name + """.h"
"""

    
  def construct_output(self,output):
    d = { 
        "CARDINALITY_2D": str( int( self.data.no_of_unknowns*self.data.dim[0]*self.data.dim[1] )), 
        "CARDINALITY_3D": str( int( self.data.no_of_unknowns*self.data.dim[0]*self.data.dim[1]*self.data.dim[2] )),
        "MERGE_METHOD_DECLARATIONS": "",
        "MERGE_METHOD_DEFINITIONS": "",
        "SEND_CONDITION":              self.send_condition,
        "RECEIVE_AND_MERGE_CONDITION": self.receive_and_merge_condition,
        "INCLUDES":                    self.includes,
        "DATA_ASSOCIATION":            int(self.data.association),
        "MERGE_METHOD_DEFINITION":     self.merge_method_definition,
        "STORE_PERSISTENT_CONDITION":  self.store_persistent_condition,
        "LOAD_PERSISTENT_CONDITION":   self.load_persistent_condition
      }

    """
      Pass in a version of output
    """
    output.makefile.add_cpp_file( self.data.namespace[-1] + "/" + self.data.name + ".cpp" )
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix+".h.template",
      templatefile_prefix+".cpp.template",
      self.data.name, 
      self.data.namespace,
      self.data.namespace[-1],
      d, 
      True)
    output.add(generated_files)

