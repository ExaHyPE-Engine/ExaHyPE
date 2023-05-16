# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import peano4.output.InvokeExternalTool



class DaStGenToLegacyTool(object):

  """
  Full qualified filename of jar file.
  """
  DaStGenJarFile = "~/git/DaStGen/DaStGen.jar"
  __DaStGenArguments = "--plugin PeanoSnippetGenerator --naming Peano4NameTranslator "
    
    
  """
  This converter requires my good old DaStGen Java tool, writes a spec file for 
  DaStGen and finally invokes the tool.
  """
  def __init__(self,data):
    self.data = data


  def get_stack_container(self):
    return "peano4::stacks::STDVectorStack< " + self.data.get_full_qualified_type() + " >";

    
  def get_header_file_include(self):
    return "#include \"peano4/stacks/STDVectorStack.h\" \n \
            #include \"" + self.data.namespace[-1] + "/" + self.data.name + ".h\""

  __Template_Prologue = """  
#include "peano4/utils/Globals.h"

// have to know whether we are using MPI
#include "config.h"

Constant: Dimensions;
Packed-Type: short int;
"""

  def __get_file_name(self):
    return self.data.name + ".def"

  def __get_full_qualified_file_name(self):
    return self.data.namespace[-1] + "/" + self.__get_file_name()


  __Template_DebugFlags = """  
  #ifdef PeanoDebug
  persistent parallelise expose double debugX[Dimensions];
  persistent parallelise expose double debugH[Dimensions];
  #endif
  
  
"""


  def __generate_dastgen_input_file(self):
    directory = self.data.namespace[-1]
    if not os.path.exists( directory ):
      os.mkdir(directory)
      print( "created directory " + directory + " for DaStGen legacy file" )
    
    file = open( self.__get_full_qualified_file_name(), "w" )
    file.write( self.__Template_Prologue )
    file.write( "class " )
    for i in self.data.namespace:
      file.write( i )
      file.write( "::" )
    file.write( self.data.name )
    file.write( " {\n" )

    file.write( self.__Template_DebugFlags );

    for i in self.data.attributes_double:
      file.write( "  persistent parallelise double " )
      file.write( i )
      file.write( ";\n" )

    for i in self.data.attributes_integer:
      file.write( "  persistent parallelise int " )
      file.write( i )
      file.write( ";\n" )

    for i in self.data.attributes_enum:
      file.write( "  enum " + i[0] + " {\n" )
      file.write( "    " )
      for j in i[1]:
        file.write( j )
        if j!=i[1][-1]:
          file.write( ", " )
        else:
          file.write( "\n" )
      file.write( "  };\n" )
      file.write( "  persistent packed parallelise " + i[0] + "  " + i[0] + ";\n" )

    file.write( "};\n\n\n" )


  def construct_output(self,output):
    """
      Pass in a version of output. It is important that external tools are used
      first before we start any compile or so. So I add them first. 
    """
    self.__generate_dastgen_input_file()
    output.makefile.add_cpp_file( self.data.namespace[-1] + "/" + self.data.name + ".cpp", generated=True )
    output.add( peano4.output.InvokeExternalTool( "java -jar " + self.DaStGenJarFile + " " + self.__DaStGenArguments + " " + self.__get_file_name() + " .", self.data.namespace[-1] ), False )

