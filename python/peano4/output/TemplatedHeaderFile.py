# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Helper import Overwrite
from .Helper import writeFile

import os
import re


class TemplatedHeaderFile(object):
  def __init__(self,headerfile_template,classname,namespace,subdirectory,dictionary,default_overwrite=True):
    """
      The template files should be fully qualified
      classname is a string
      namespace is a (possibly empty) list of strings
      
    """
    self.headerfile_template  = headerfile_template
    self.classname            = classname
    self.namespace            = namespace
    self.subdirectory         = subdirectory
    self.default_overwrite    = default_overwrite
    
    self.d = dictionary
    self.d[ "OPEN_NAMESPACE" ]           = ""
    self.d[ "CLOSE_NAMESPACE" ]          = ""
    self.d[ "FULL_NAMESPACE" ]           = ""
    self.d[ "FULL_QUALIFIED_CLASSNAME" ] = ""
    for i in namespace:
      self.d[ "OPEN_NAMESPACE" ]           += "namespace " + i + "{\n"
      self.d[ "CLOSE_NAMESPACE" ]          += "}\n"
      self.d[ "FULL_QUALIFIED_CLASSNAME" ] += i + "::"
      if self.d[ "FULL_NAMESPACE" ] != "":
        self.d[ "FULL_NAMESPACE" ]         += "::"
      self.d[ "FULL_NAMESPACE" ]           += i
    self.d[ "CLASSNAME" ]                 = classname
    self.d[ "FULL_QUALIFIED_CLASSNAME" ] += classname
    self.d[ "INCLUDE_GUARD" ]             = self.d[ "FULL_QUALIFIED_CLASSNAME" ].replace( "::", "_" ).replace( "-", "_" ).upper()
    

  def generate_file(self,overwrite,full_qualified_filename,template_file):
    if template_file!=None and writeFile(overwrite,self.default_overwrite,full_qualified_filename):
      import inspect
      print( "[{}] writes {}".format(inspect.getfile(self.__class__), full_qualified_filename ))
      with open( os.path.realpath(template_file), "r" ) as input:
        template = input.read()
      with open( full_qualified_filename, "w" ) as output:
        output.write( template.format(**self.d) )


  def generate(self,overwrite,directory):
    if not os.path.exists( directory + "/" + self.subdirectory ):
      os.mkdir(directory + "/" + self.subdirectory)
    
    header_filename = directory + "/" + self.subdirectory + "/" + self.classname + ".h";

    self.generate_file(overwrite,header_filename,self.headerfile_template)    
    
      
