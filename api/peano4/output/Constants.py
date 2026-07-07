# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

from .Helper import write_file

from peano4.Logger import log

class Constants(object):
  """!
    Representative of generated Constants.h holding all user-defined constants

    Represents all constants that a Project exports from the Python
    script into C++. I do provide routines to export defines or
    constants (via constexpr). For the latter, I rely on the auto
    type word unless you use a specialised routine for a particular
    type.

    Each project has one instance of Constants, so you can always
    add/export new constants with

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    my_project.constants.export ...
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    Constants also allows you to add defines (precompiler statements).
  """

  def __init__(self,project):
    self.defines = []
    # Constants are keyed by name: re-exporting a name replaces the previous
    # entry instead of appending a duplicate, so the generator may run several
    # times on the same project without producing redefinition errors in
    # Constants.h.
    self._constant_entries = {}
    self._includes = []
    self.d = {}
    self.d[ "ADD_CONSTANTS" ]            = ""
    self.d[ "INCLUDES" ]                 = ""
    self.d[ "OPEN_NAMESPACE" ]           = ""
    self.d[ "CLOSE_NAMESPACE" ]          = ""
    self.d[ "INCLUDE_GUARD" ]            = "_"
    for i in project.namespace:
      self.d[ "OPEN_NAMESPACE" ]        += "namespace " + i + "{\n"
      self.d[ "CLOSE_NAMESPACE" ]       += "}\n"
      self.d[ "INCLUDE_GUARD" ]         += i + "_"
    self.d[ "INCLUDE_GUARD" ]           += "CONSTANTS_"
    self.d[ "INCLUDE_GUARD" ] = self.d[ "INCLUDE_GUARD" ].upper()

  def add_include(self, include_statement ):
    """
     Add a whole include statement. Identical statements are only added once.
    """
    if include_statement not in self._includes:
      self._includes.append( include_statement )
    self.d[ "INCLUDES" ] = "".join( i + "\n" for i in self._includes )

  def _set_entry(self, name, entry):
    """
     Insert or replace the constant entry registered under name and rebuild
     the rendered constants block.
    """
    self._constant_entries[ str(name) ] = entry
    self.d[ "ADD_CONSTANTS" ] = "".join(
      "  " + e + "\n" for e in self._constant_entries.values()
    )

  def export( self, name, value ):
    """
      Tell the C++ code underlying the project that a certain variable with a
      name has a certain value. The passed arguments are mapped onto an
      constexpr. Therefore, name has to be a string, while value can be an
      integer, a float or a string as well. If you want to export booleans
      or just define variants, you have to use the other routines.
    """
    new_entry = "constexpr auto " + str(name) + " = " + str(value) + ";"
    self._set_entry( name, new_entry )

  def export_boolean_sequence( self, name, value ):
    """
      Tell the C++ code underlying the project that a certain variable with a
      name has a certain value. The passed arguments are mapped onto an
      constexpr. Therefore, name has to be a string, while value can be an
      integer, a float or a string as well. If you want to export booleans
      or just define variants, you have to use the other routines.
    """
    new_entry = "const std::bitset<" + str(len(value)) + "> " + str(name) + " = 0";
    base = 1
    for i in range(0,len(value)):
      if value[i]:
        new_entry += "+" + str(base)
      base *= 2
    new_entry += ";"
    self._set_entry( name, new_entry )

  def export_string( self, name, value ):
    """
      Tell the C++ code underlying the project that a certain variable with a
      name has a certain value. The passed arguments are mapped onto an
      constexpr. Therefore, name has to be a string, while value can be an
      integer, a float or a string as well. If you want to export booleans
      or just define variants, you have to use the other routines.
    """
    self.export_const_with_type( name, "\"" + str(value) + "\"", "std::string" )
    pass

  def export_const_with_type( self, name, value, type ):
    """
      Tell the C++ code underlying the project that a certain variable with a
      name has a certain value. The passed arguments are mapped onto an
      const. Therefore, name has to be a string, while value can be an
      integer, a float or a string as well. If you want to export booleans
      or just define variants, you have to use the other routines.
    """
    new_entry = "const " + type + " " + str(name) + " = " + value + ";"
    self._set_entry( name, new_entry )

  def export_constexpr_with_type( self, name, value, type ):
    """
      Tell the C++ code underlying the project that a certain variable with a
      name has a certain value. The passed arguments are mapped onto an
      constexpr. Therefore, name has to be a string, while value can be an
      integer, a float or a string as well. If you want to export booleans
      or just define variants, you have to use the other routines.
    """
    new_entry = "constexpr " + type + " " + str(name) + " = " + value + ";"
    self._set_entry( name, new_entry )

  def clear(self):
    self._constant_entries = {}
    self.d[ "ADD_CONSTANTS" ] = ""

  def export_boolean( self, name, value ):
    new_entry = "constexpr bool " + name + " = "
    if value:
      new_entry += "true"
    else:
      new_entry += "false"
    self._set_entry( name, new_entry + ";" )

  def define( self, name ):
    """!

    Add define pragma to Constants.h

    This routine introduces a

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #defined name
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~

    entry into your Constants.h.

    """
    new_entry = "#define " + name
    self._set_entry( "#define " + name, new_entry )

  def define_value(self, name, value):
    """!

    Add define pragma to Constants.h

    This routine introduces a

    ~~~~~~~~~~~~~~~~~~~~~~~~~~~
    #defined name value
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~

    entry into your Constants.h.

    """
    new_entry = "#define " + name + " " + value
    self._set_entry( "#define " + name, new_entry )

  def readme_entry(self):
    """!

    """
    result = """

## Exported C++ constants in project

""" + self.d[ "ADD_CONSTANTS" ] + """

These constants are available to the application within its namespace.
"""
    return result

  def generate(self, directory, protected_files):
    filename = directory + "/" + "Constants.h"
    if write_file(directory,protected_files,filename):
      log.debug("Write {}".format(filename))

      #for i in self.cppfiles:
      #  self.d[ 'SOURCES' ] += " "
      #  self.d[ 'SOURCES' ] += i

      # We first eliminate the precompiled variant, and then we get rid of the
      # postfix in the case where it is a source file
      with open( os.path.realpath(__file__).replace( ".pyc", ".template.h" ).replace( ".py", ".template.h" ), "r" ) as input:
        template = input.read()
      with open( filename, "w" ) as output:
        output.write( template.format(**self.d) )
