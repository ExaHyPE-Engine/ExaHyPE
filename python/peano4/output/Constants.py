# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Helper import Overwrite
from .Helper import writeFile

import os
import re


class Constants(object):
  """
   
    Represents all constants that a Project exports from the Python
    script into C++. I do provide routines to export defines or 
    constants (via constexpr). For the latter, I rely on the auto 
    type word unless you use a specialised routine for a particular 
    type. 
    
    Each project has one instance of Constants, so you can always 
    add/export new constants with
    
    my_project.constants.export ...
    
    Constants also allows you to add defines (precompiler statements).
     
  """
  default_overwrite = True
   
   
  def __init__(self,project):
    self.defines = []
    self.d = {}
    self.d[ "ADD_CONSTANTS" ]            = ""

    self.d[ "OPEN_NAMESPACE" ]           = ""
    self.d[ "CLOSE_NAMESPACE" ]          = ""
    self.d[ "INCLUDE_GUARD" ]            = "_"
    for i in project.namespace:
      self.d[ "OPEN_NAMESPACE" ]        += "namespace " + i + "{\n"
      self.d[ "CLOSE_NAMESPACE" ]       += "}\n"
      self.d[ "INCLUDE_GUARD" ]         += i + "_"
    self.d[ "INCLUDE_GUARD" ]         += "CONSTANTS_"

    self.d[ "INCLUDE_GUARD" ] = self.d[ "INCLUDE_GUARD" ].upper()
    
    
    
  def export( self, name, value ):
    """
      Tell the C++ code underlying the project that a certain variable with a 
      name has a certain value. The passed arguments are mapped onto an 
      constexpr. Therefore, name has to be a string, while value can be an 
      integer, a float or a string as well. If you want to export booleans
      or just define variants, you have to use the other routines.
    """
    new_entry = "constexpr auto " + str(name) + " = " + str(value) + ";"
    self.d[ "ADD_CONSTANTS" ] += "  " + new_entry + "\n"
    pass
  
  
  def clear(self):
    self.d[ "ADD_CONSTANTS" ]            = ""
    pass

  def export_boolean( self, name, value ):
    new_entry = "constexpr bool " + name + " = "
    if value:
      new_entry += "true"
    else:
      new_entry += "false"
    self.d[ "ADD_CONSTANTS" ] += "  " + new_entry + ";\n"
    pass


  def define( self, name ):
    new_entry = "#define " + name
    self.d[ "ADD_CONSTANTS" ] += "  " + new_entry + "\n"
    pass


  def generate(self, overwrite, directory):
    filename = directory + "/Constants.h";
    if writeFile(overwrite,self.default_overwrite,filename):
      print( "write " + filename )
      
      #for i in self.cppfiles:
      #  self.d[ 'SOURCES' ] += " "
      #  self.d[ 'SOURCES' ] += i
      
      # We first eliminate the precompiled variant, and then we get rid of the
      # postfix in the case where it is a source file
      with open( os.path.realpath(__file__).replace( ".pyc", ".h.template" ).replace( ".py", ".h.template" ), "r" ) as input:
        template = input.read()
      with open( filename, "w" ) as output:
        output.write( template.format(**self.d) )
   
