# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.datamodel.DaStGen2

from peano4.datamodel.DoF      import DoF
from peano4.datamodel.DaStGen2 import DaStGen2Generator

import dastgen2
import os


class VertexTaskCounterGenerator(object):
  def __init__(self,data):
    self._data = data


  def get_stack_container(self):
    return "peano4::stacks::STDVectorStack< " + self._data.get_full_qualified_type() + " >";


  def get_header_file_include(self):
    return """
#include "peano4/stacks/STDVectorStack.h"
#include \"""" + self._data.namespace[-1] + """/""" + self._data.name + """.h"
"""


  def construct_output(self,output):
    d = {}

    templatefile_prefix = os.path.realpath(__file__).replace( ".pyc", "" ).replace( ".py", "" )
    generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix+".template.h",
      templatefile_prefix+".template.cpp",
      self._data.name,
      self._data.namespace,
      self._data.namespace[-1],
      d,
      True)
    output.add(generated_files)
    output.makefile.add_cpp_file( self._data.namespace[-1] + "/" + self._data.name + ".cpp" )
    pass



class VertexTaskCounter(DoF):
  """

    :Attributes:

    name: String
      Name of the VertexTaskCounter. Has to be a valid C++ class name.

  """
  def __init__(self, name):
    DoF.__init__(self, name)

    self.generator = VertexTaskCounterGenerator(self)
