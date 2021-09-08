# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Helper import Overwrite
from .Helper import writeFile

import os
import re

try:
  import jinja2
except:
  print( "Warning: jinja2 not available, i.e. you will have very limited API support (such as visualisation only)")
from copy import deepcopy

class Jinja2TemplatedHeaderFile(object):
  def __init__(self,headerfile_template,classname,namespace,subdirectory,dictionary,default_overwrite=True,apply_iteratively=False):
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
    self.apply_iteratively    = apply_iteratively

    self.d = deepcopy(dictionary)
    self.d[ "CLASSNAME" ] = classname
    self.d[ "NAMESPACE" ] = []
    self.d[ "FULL_QUALIFIED_NAMESPACE" ] = ""

    for i in namespace:
      self.d["NAMESPACE"].append(i)
      self.d[ "FULL_QUALIFIED_NAMESPACE" ] += "::" + i


  def __generate_file(self,overwrite,full_qualified_filename,template_file):
    """

      template_file: string
         Usually, this is an absolute file name.

    """
    if template_file!=None and writeFile(overwrite,self.default_overwrite,full_qualified_filename):
      print( "write " + full_qualified_filename )

      template_loader = jinja2.FileSystemLoader(searchpath=os.path.split(template_file)[0])
      templateEnv = jinja2.Environment(loader=template_loader, undefined=jinja2.DebugUndefined)
      template = templateEnv.get_template( os.path.split(template_file)[1] )

      if self.apply_iteratively:
        rendered_text           = template.render(self.d)
        while "{{" in rendered_text and len(rendered_text)!=old_size:
          old_size      = len(rendered_text)
          template      = jinja2.Template(rendered_text, undefined=jinja2.DebugUndefined)
          rendered_text = template.render( self.d )
            
      with open( full_qualified_filename, "w" ) as output:
        output.write( template.render(self.d) )


  def generate(self,overwrite,directory):
    if not os.path.exists( directory + "/" + self.subdirectory ):
      os.mkdir(directory + "/" + self.subdirectory)

    header_filename = directory + "/" + self.subdirectory + "/" + self.classname + ".h";

    self.__generate_file(overwrite,header_filename,self.headerfile_template)


