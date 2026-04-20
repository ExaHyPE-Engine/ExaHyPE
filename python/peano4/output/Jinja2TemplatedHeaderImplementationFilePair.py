# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import re
import jinja2

from pathlib import Path
from copy import deepcopy

from .Helper import get_jinja2_template
from .Helper import render_jinja2_template
from .Helper import write_file

from .Overwrite import Overwrite

from peano4.Logger import log

class Jinja2TemplatedHeaderImplementationFilePair(object):
  """!
Represents a pair of C++ header and implementation files

The content of these files is represented by jinja2 templates. While
sorting out these templates, the class defines a couple of default
dictionary entries besides the ones manually handed over by the user.
  """

  def __init__(self,
               headfile_template,
               cppfile_template,
               classname,
               namespace,
               subdirectory,
               dictionary,
               default_overwrite=True,
               apply_iteratively=False):
    """
      The template files should be fully qualified
      classname is a string
      namespace is a (possibly empty) list of strings

      cppfile_template can be None
    """
    self.headerfile_template  = headfile_template
    self.cppfile_template     = cppfile_template
    self.classname            = classname
    self.namespace            = namespace
    self.subdirectory         = subdirectory
    self.default_overwrite    = default_overwrite
    self.apply_iteratively    = apply_iteratively

    self.d = deepcopy(dictionary)
    self.d[ "CLASSNAME" ] = classname
    self.d[ "NAMESPACE" ] = []
    for i in namespace:
      self.d["NAMESPACE"].append(i)
    # Eliminate the prepending "::" that can lead to compatibility issues across the solvers
    self.d["FULL_QUALIFIED_NAMESPACE"] = "::".join(namespace)

  def __generate_file(self,overwrite,full_qualified_filename,template_file):
    if template_file!=None and write_file(overwrite,self.default_overwrite,full_qualified_filename):
      log.debug("Write {}".format(full_qualified_filename))

      template = get_jinja2_template(template_file, jinja2.DebugUndefined)
      rendered_text = template.render(self.d)

      if self.apply_iteratively:
        old_size                = 0
        while "{{" in rendered_text and len(rendered_text)!=old_size:
          old_size      = len(rendered_text)
          rendered_text = render_jinja2_template(
            rendered_text,
            self.d,
            jinja2.DebugUndefined,
          )

      with open( full_qualified_filename, "w" ) as output:
        output.write( rendered_text )

  def generate(self,overwrite,directory):
    if not os.path.exists( directory + "/" + self.subdirectory ):
      os.mkdir(directory + "/" + self.subdirectory)

    header_filename = directory + "/" + self.subdirectory + "/" + self.classname + Path(self.headerfile_template).suffix
    cpp_filename    = directory + "/" + self.subdirectory + "/" + self.classname + Path(self.cppfile_template).suffix

    self.__generate_file(overwrite,header_filename,self.headerfile_template)
    self.__generate_file(overwrite,cpp_filename   ,self.cppfile_template)
