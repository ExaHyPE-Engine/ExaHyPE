# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Helper import Overwrite
from .Helper import writeFile

import os
import re

import jinja2
from copy import deepcopy

class Jinja2TemplatedHeaderFile(object):
  def __init__(self,headerfile_template,classname,namespace,subdirectory,dictionary,default_overwrite=True):
    """
      The template files should be fully qualified
      
      classname is a string
      namespace is a (possibly empty) list of strings
      dictionary a series of mappings (cmp. Jinja2 convention)
      
    """
    self.headerfile_template  = headerfile_template
    self.classname            = classname
    self.namespace            = namespace
    self.subdirectory         = subdirectory
    self.default_overwrite    = default_overwrite

    self.d = deepcopy(dictionary)
    self.d[ "CLASSNAME" ] = classname
    self.d[ "NAMESPACE" ] = []
    
    for i in namespace:
      self.d["NAMESPACE"].append(i)
    

  def __generate_file(self,overwrite,full_qualified_filename,template_file):
    if template_file!=None and writeFile(overwrite,self.default_overwrite,full_qualified_filename):
      print( "write " + full_qualified_filename )
      
      template_loader = jinja2.FileSystemLoader(searchpath='/')
      templateEnv = jinja2.Environment(loader=template_loader)
      template = templateEnv.get_template( template_file )
      
      with open( full_qualified_filename, "w" ) as output:
        output.write( template.render(self.d) )


  def generate(self,overwrite,directory):
    if not os.path.exists( directory + "/" + self.subdirectory ):
      os.mkdir(directory + "/" + self.subdirectory)
    
    header_filename = directory + "/" + self.subdirectory + "/" + self.classname + ".h";

    self.__generate_file(overwrite,header_filename,self.headerfile_template)    
    
      
