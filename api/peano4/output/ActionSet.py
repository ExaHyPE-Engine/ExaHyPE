# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

from .Helper import write_file

import peano4.solversteps.ActionSet

from peano4.Logger import log


class ActionSet(object):
  def __init__(self,classname,namespace,subdirectory,implementation):
    """!
     implementation Action set of type peano4.solversteps.ActionSet. It supplies
                    the includes, the attributes and the bodies of the generated
                    class.
    """
    self.classname    = classname
    self.namespace    = namespace
    self.subdirectory = subdirectory
    self.operations   = [
      (peano4.solversteps.ActionSet.OPERATION_BEGIN_TRAVERSAL,"void"),
      (peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL,"void")
    ]
    self.include_files  = []
    self.typedefs       = []
    self.implementation = implementation

  def add_operation(self,name,signature):
    """
     signature is a long tuple. The first entry is the name of the routine,
     the second entry is the result type. From thereon, one entry gives the
     name of an attribute, the second one the type.
    """
    self.operations.append(signature)

  LicenceHeader = (
    "// This file is part of the Peano project. For conditions of distribution and\n"
    "// use, please see the copyright notice at www.peano-framework.org\n"
  )

  def __generate_includes(self, outputfile):
    for i in self.include_files: outputfile.write( '#include "{}"\n'.format(i) )

  @staticmethod
  def __generate_body(outputfile,body):
    """!
     Writes body between the braces of a definition without a blank line next to
     a brace.
    """
    stripped_body = body.strip("\n")
    if stripped_body:
      outputfile.write( stripped_body + "\n" )

  def __get_operation_arguments(self,operation):
    if len(operation)<=2:
      return "()"
    result = "(\n      "
    i = 2
    while i<len(operation):
      result += operation[i+1]
      result += " "
      result += operation[i]
      i+=2
      if i<len(operation):
        result += ",\n      "

    result += ")"
    return result

  def __generate_operation(self,outputfile,operation):
    """
      outputfile points to file, operation to an operation object as added by the solver step
    """
    outputfile.write( "    " )
    outputfile.write( operation[1] )
    outputfile.write( " ")
    outputfile.write( operation[0] )
    outputfile.write( self.__get_operation_arguments(operation) )
    outputfile.write( ";\n\n")

  def __get_full_qualified_class_name(self):
    full_qualified_classname = ""
    for i in self.namespace:
      full_qualified_classname += i
      full_qualified_classname += "::"
    full_qualified_classname += self.classname
    return full_qualified_classname

  def __generate_header(self,directory,protected_files):
    filename = directory + "/" + self.subdirectory + "/" + self.classname + ".h"
    if write_file(directory,protected_files,filename):
      log.debug("Write {}".format(filename))

      with open( filename, "w" ) as outputfile:
          full_qualified_classname = self.__get_full_qualified_class_name()

          outputfile.write( ActionSet.LicenceHeader )
          outputfile.write( "#pragma once\n" )

          outputfile.write( """
#include "peano4/Constants.h"
#include "peano4/dataview/VertexEnumerator.h"
#include "peano4/dataview/VertexMarker.h"
#include "peano4/dataview/FaceEnumerator.h"
#include "peano4/dataview/FaceMarker.h"
#include "peano4/dataview/CellMarker.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/la/Vector.h"
""" )

          includes = self.implementation.get_includes()
          if includes.strip():
              outputfile.write( includes.rstrip("\n") + "\n" )
          self.__generate_includes(outputfile)

          joined_namespace = "::".join(self.namespace)
          outputfile.write( "\n" )
          outputfile.write( "namespace " + joined_namespace + " {\n" )
          outputfile.write( "  class {};\n".format(self.classname) )
          outputfile.write( "} // namespace " + joined_namespace + "\n" )

          outputfile.write( "\n" )
          outputfile.write( "class " + full_qualified_classname + " {\n" )
          outputfile.write( "  public:\n" )
          outputfile.write( """    /**
     * Creates the action instance of one tree for one grid sweep. The creation
     * of actions is not thread-safe.
     *
     * @param treeNumber Number of the spacetree this instance serves. It is
     *                   smaller than 0 for the prototype action of a rank.
     */
""" )
          outputfile.write( "    " + self.classname + "(int treeNumber);\n" )
          outputfile.write( "    ~" + self.classname + "();\n\n" )
          outputfile.write( """    std::vector<peano4::grid::GridControlEvent> getGridControlEvents() const;
    static void prepareTraversal();
    static void unprepareTraversal();

""" )

          for i in self.operations: self.__generate_operation(outputfile,i)

          attributes = self.implementation.get_attributes()
          if attributes.strip():
            outputfile.write( "  private:\n" )
            outputfile.write( attributes )

          outputfile.write( "};\n" )

  def get_cpp_file_name(self):
    return self.subdirectory + "/" + self.classname + ".cpp"

  def __generate_implementation(self,directory,protected_files):
    filename = directory + "/" + self.get_cpp_file_name()
    if write_file(directory,protected_files,filename):
      log.debug("Write {}".format(filename))
      with open( filename, "w" ) as outputfile:
        full_qualified_classname = self.__get_full_qualified_class_name()

        outputfile.write( ActionSet.LicenceHeader )
        outputfile.write( "#include \"" + self.classname + ".h\"\n" )

        static_initialisations = self.implementation.get_static_initialisations(full_qualified_classname).strip("\n")
        if static_initialisations:
          outputfile.write( "\n" )
          outputfile.write( static_initialisations + "\n" )

        outputfile.write( "\n" )
        outputfile.write( full_qualified_classname + "::" + self.classname + "(int treeNumber) {\n" )
        ActionSet.__generate_body( outputfile, self.implementation.get_constructor_body() )
        outputfile.write( "}\n" )

        outputfile.write( "\n" )
        outputfile.write( full_qualified_classname + "::~" + self.classname + "() {\n" )
        ActionSet.__generate_body( outputfile, self.implementation.get_destructor_body() )
        outputfile.write( "}\n" )

        outputfile.write( "\n" )
        outputfile.write( "std::vector<peano4::grid::GridControlEvent> " + full_qualified_classname + "::getGridControlEvents() const {\n" )
        ActionSet.__generate_body( outputfile, self.implementation.get_body_of_getGridControlEvents() )
        outputfile.write( "}\n" )

        outputfile.write( "\n" )
        outputfile.write( "void " + full_qualified_classname + "::prepareTraversal() {\n" )
        ActionSet.__generate_body( outputfile, self.implementation.get_body_of_prepareTraversal() )
        outputfile.write( "}\n" )

        outputfile.write( "\n" )
        outputfile.write( "void " + full_qualified_classname + "::unprepareTraversal() {\n" )
        ActionSet.__generate_body( outputfile, self.implementation.get_body_of_unprepareTraversal() )
        outputfile.write( "}\n" )

        for operation in self.operations:
          outputfile.write( "\n" )
          outputfile.write( operation[1] )
          outputfile.write( " " )
          outputfile.write( full_qualified_classname + "::" + operation[0] )
          outputfile.write( self.__get_operation_arguments(operation) )
          outputfile.write( " {\n" )
          ActionSet.__generate_body( outputfile, self.implementation.get_body_of_operation( operation[0] ) )
          outputfile.write( "}\n" )

  def generate(self,directory,protected_files):
    if not os.path.exists( directory + "/" + self.subdirectory ):
      os.makedirs(directory + "/" + self.subdirectory, exist_ok=True)
    self.__generate_header(directory,protected_files)
    self.__generate_implementation(directory,protected_files)
