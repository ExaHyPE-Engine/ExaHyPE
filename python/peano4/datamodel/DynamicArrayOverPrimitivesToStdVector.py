# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.output.TemplatedHeaderImplementationFilePair
import os



class DynamicArrayOverPrimitivesToStdVector(object):
  """
  Very simple converter which maps the patch 1:1 onto a double array
  """
  def __init__(self,patch):
    self.data = patch


  def get_stack_container(self):
    return "peano4::stacks::STDVectorStack< " + self._data.get_full_qualified_type() + " >";

    
  def get_header_file_include(self):
    return "#include \"peano4/stacks/STDVectorStack.h\" \n \
            #include \"" + self.data.namespace[-1] + "/" + self.data.name + ".h\""

    
  def construct_output(self,output):
    """
      Pass in a version of output
    """
    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    generated_files = peano4.output.TemplatedHeaderImplementationFilePair(
      templatefile_prefix+".h.template",
      None,
      self.data.name, 
      self.data.namespace,
      self.data.namespace[-1],
      {
        "TYPE" : self.data.primitive_type
      }, 
      #{ 
      #  "CARDINALITY_2D": str(self.data.no_of_unknowns*self.data.dim[0]*self.data.dim[1]), 
      #  "CARDINALITY_3D": str(self.data.no_of_unknowns*self.data.dim[0]*self.data.dim[1]*self.data.dim[2]) 
      #},
      True)
    output.add(generated_files)
    pass

