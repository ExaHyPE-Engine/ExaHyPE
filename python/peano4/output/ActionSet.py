# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Helper import Overwrite
from .Helper import writeFile

import os
import re
import peano4.solversteps.ActionSet


class ActionSet(object):
 
  def __init__(self,classname,namespace,subdirectory,implementation = None):
    """
     implementation Should be of type peano4.solversteps.Mapping or None. If 
                    it is None, then the generated stuff will be a sole 
                    interface.
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

  def __generate_includes(self,outputfile):
    for i in self.include_files:
      outputfile.write( "#include \"" )
      outputfile.write( i )
      outputfile.write( "\"\n")

      
  def __get_operation_arguments(self,operation):
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
    if self.implementation == None:
      outputfile.write( "    virtual " )
    else:
      outputfile.write( "    " )
      outputfile.write( operation[1] )
      outputfile.write( " ")
      outputfile.write( operation[0] )
      outputfile.write( self.__get_operation_arguments(operation) )
      if self.implementation == None:
        outputfile.write( " = 0")
      outputfile.write( ";\n\n")


  def __get_full_qualified_class_name(self):
    full_qualified_classname = ""
    for i in self.namespace:
      full_qualified_classname += i
      full_qualified_classname += "::"
    full_qualified_classname += self.classname
    return full_qualified_classname
  
  
  def __generate_header(self,overwrite,directory):
    filename = directory + "/" + self.subdirectory + "/" + self.classname + ".h";
    default_overwrite = True
    if self.implementation.user_should_modify_template():
      default_overwrite = False

    if writeFile(overwrite,default_overwrite,filename):
      print( "write " + filename )
      outputfile = open( filename, "w" )

      full_qualified_classname = self.__get_full_qualified_class_name()
      include_guard = "_" + full_qualified_classname.replace( "::", "_" ).upper() + "_H_"

      outputfile.write( "#ifndef " + include_guard + "\n" )
      outputfile.write( "#define " + include_guard + "\n\n\n" )
      
      outputfile.write( """
#include "peano4/utils/Globals.h"
#include "peano4/datamanagement/VertexEnumerator.h"
#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/datamanagement/FaceEnumerator.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/grid/GridControlEvent.h"
#include "tarch/la/Vector.h"

#include <vector>


""" )

      outputfile.write( self.implementation.get_includes() )
      outputfile.write( "\n\n" )

      self.__generate_includes(outputfile)
      
      for i in self.namespace:
        outputfile.write( "namespace " + i + " {\n" )

      outputfile.write( "  class " + self.classname + ";\n" )

      for i in self.namespace:
        outputfile.write( "}\n" )

      outputfile.write( "class " + full_qualified_classname + "{\n" )
      outputfile.write( "  private:\n" )
      outputfile.write( "    static tarch::logging::Log  _log;\n" )
      outputfile.write( self.implementation.get_attributes() )

      outputfile.write( "  public:\n" )
      outputfile.write( """
    /**
     * Create action instance for one tree for one grid sweep
     *
     * <h2> Thread safety </h2>
     *
     * The creation of individual trees usually happens through peano4::parallel::SpacetreeSet::createObserverCloneIfRequired().
     * This routine is called lazily when we start to traverse a subtree. 
     * Therefore, the creation of actions is not thread-safe.
     *
     *
     * @param treeNumber Number of the spacetree for which we create the tree instance. Is 
     *                   smaller 0 if this is the prototype action used on a rank from which 
     *                   the real actions are constructed from.
     */      
""" )
      outputfile.write( "    " + self.classname + "(int treeNumber);\n\n" )
      outputfile.write( "    ~" + self.classname + "();\n\n" )
      outputfile.write( "    std::vector< peano4::grid::GridControlEvent > getGridControlEvents() const;\n\n" )
      
      for i in self.operations:
        self.__generate_operation(outputfile,i)       

      outputfile.write( "};\n\n\n" )

      outputfile.write( "#endif\n\n" )



  def get_cpp_file_name(self):
    return self.subdirectory + "/" + self.classname + ".cpp"


  def __generate_implementation(self,overwrite,directory):
    filename = directory + "/" + self.get_cpp_file_name()
    default_overwrite = True
    if self.implementation.user_should_modify_template():
      default_overwrite = False
    if writeFile(overwrite,default_overwrite,filename):
      print( "write " + filename )
      outputfile = open( filename, "w" )

      #full_qualified_classname = self.__get_full_qualified_class_name()

      outputfile.write( "#include \"" + self.classname + ".h\"\n\n\n" )

      outputfile.write( "tarch::logging::Log " + self.__get_full_qualified_class_name() + "::_log( \""+ self.__get_full_qualified_class_name() + "\");\n\n\n" )

      outputfile.write( self.implementation.get_static_initialisations(self.__get_full_qualified_class_name()) )
      outputfile.write( "\n\n\n" )

      outputfile.write( self.__get_full_qualified_class_name() + "::" + self.classname + "(int treeNumber) {\n" )
      outputfile.write( self.implementation.get_constructor_body() )
      outputfile.write( "}\n\n\n" )

      outputfile.write( self.__get_full_qualified_class_name() + "::~" + self.classname + "() {\n" )
      outputfile.write( self.implementation.get_destructor_body() )
      outputfile.write( "}\n\n\n" )

      outputfile.write( "std::vector< peano4::grid::GridControlEvent > " + self.__get_full_qualified_class_name() + "::getGridControlEvents() const {\n" )
      outputfile.write( self.implementation.get_body_of_getGridControlEvents() )
      outputfile.write( "}\n\n\n" )

      
      for operation in self.operations:
        outputfile.write( operation[1] )
        outputfile.write( " " )
        outputfile.write( self.__get_full_qualified_class_name() + "::" + operation[0] )
        outputfile.write( self.__get_operation_arguments(operation) )
        outputfile.write( " {\n" )
        outputfile.write( self.implementation.get_body_of_operation( operation[0] ) )
        outputfile.write( "\n" )
        outputfile.write( "}\n\n\n" )


  def generate(self,overwrite,directory):
    if not os.path.exists( directory + "/" + self.subdirectory ):
      os.mkdir(directory + "/" + self.subdirectory)
    self.__generate_header(overwrite,directory)
    self.__generate_implementation(overwrite,directory)

