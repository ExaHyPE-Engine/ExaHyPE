# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DaStGenToLegacyTool import DaStGenToLegacyTool
from .DoF                 import DoF

import dastgen2

import peano4.dastgen2.MPI


class DaStGen2Generator(object):
  def __init__(self,data):
    self._data = data
    

  def get_stack_container(self):
    return "peano4::stacks::STDVectorStack< " + self._data.get_full_qualified_type() + " >";

    
  def get_header_file_include(self):
    return "#include \"peano4/stacks/STDVectorStack.h\" \n \
            #include \"" + self._data.namespace[-1] + "/" + self._data.name + ".h\""


  def construct_output(self,output):
    """
      Pass in a version of output. It is important that external tools are used
      first before we start any compile or so. So I add them first. 
    """
    full_qualified_name = ""
    for i in self._data.namespace:
      full_qualified_name += i
      full_qualified_name += "::"
    full_qualified_name += self._data.name
    self._data.data.set_full_qualified_name(full_qualified_name)

    self._data.data.write_header_file( self._data.namespace[-1] + "/" + self._data.name + ".h" )
    self._data.data.write_implementation_file(  self._data.namespace[-1] + "/" + self._data.name + ".cpp" )
    #self.__generate_dastgen_input_file()
    output.makefile.add_cpp_file( self._data.namespace[-1] + "/" + self._data.name + ".cpp" )
    #output.add( peano4.output.InvokeExternalTool( "java -jar " + self.DaStGenJarFile + " " + self.__DaStGenArguments + " " + self.__get_file_name() + " .", self.data.namespace[-1] ), False )



class DaStGen2(DoF):
  """
  
    A DaStGen2 data type generator. To add fields to this object, just
    use the DaStGen2 instance data of this field, i.e. data.add_attribute().
    
    :Attributes:
    
    data: dastgen2.DataModel
      Add elements to this guy to enrich your data model.
    
    :Arguments:
    
    name: String
      Name (unqualified)
      
    
      
  """  
  def __init__(self, name):
    super(DaStGen2, self).__init__(name)
    self.generator        = DaStGen2Generator(self)
    
    self.data             = dastgen2.DataModel(name)
    

    
    #@todo ifdef PeanoDebug
    self.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "debugX", "Dimensions" ) )
    self.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "debugH", "Dimensions" ) )


  def configure(self,namespace,association):
    """
    
      I always need the MPI aspect, but I can't add the right one before I don't 
      know whether this DaStGen model is used for vertices, faces or cells. To I 
      hook into this routine. In theory, I could add the DaStGen2 MPI aspect 
      straightaway (in the constructor), but I decided to do so in one place. 
      
    """
    super(DaStGen2, self).configure(namespace,association)
    self.data.add_aspect( dastgen2.aspects.MPI() )
    self.data.add_aspect( peano4.dastgen2.MPI(association) )
    

