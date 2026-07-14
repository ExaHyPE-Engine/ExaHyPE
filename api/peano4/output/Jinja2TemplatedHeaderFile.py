# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import jinja2

from pathlib import Path
from copy import deepcopy

from .Helper import get_jinja2_template
from .Helper import render_jinja2_template
from .Helper import write_file

from peano4.Logger import log


class Jinja2TemplatedHeaderFile(object):
  def __init__(self,headerfile_template,classname,namespace,subdirectory,dictionary,apply_iteratively=False):
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
    self.apply_iteratively    = apply_iteratively

    self.d = deepcopy(dictionary)
    self.d[ "CLASSNAME" ] = classname
    self.d[ "NAMESPACE" ] = []
    for i in namespace:
      self.d["NAMESPACE"].append(i)
    # Eliminate the prepending "::" that can lead to compatibility issues across the solvers
    self.d["FULL_QUALIFIED_NAMESPACE"] = "::".join(namespace)

  def __generate_file(self,directory,protected_files,full_qualified_filename,template_file):
    """
      template_file: string
         Usually, this is an absolute file name.
    """
    if template_file!=None and write_file(directory,protected_files,full_qualified_filename):
      log.debug("Write " + full_qualified_filename )

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

  def generate(self,directory,protected_files):
    if not os.path.exists( directory + "/" + self.subdirectory ):
      os.makedirs(directory + "/" + self.subdirectory, exist_ok=True)

    header_filename = directory + "/" + self.subdirectory + "/" + self.classname + Path(self.headerfile_template).suffix

    self.__generate_file(directory,protected_files,header_filename,self.headerfile_template)
