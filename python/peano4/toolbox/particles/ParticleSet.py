# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.datamodel.DaStGen2

from peano4.datamodel.DoF      import DoF
from peano4.datamodel.DaStGen2 import DaStGen2Generator

from .Particle import Particle

import dastgen2
import os


class ParticleSetGenerator(object):
  def __init__(self,data):
    self._data = data


  def get_stack_container(self):
    return "peano4::stacks::STDVectorStack< " + self._data.get_full_qualified_type() + " >";

    
  def get_header_file_include(self):
    return """
#include "peano4/stacks/STDVectorStack.h"
#include "../globaldata/""" + self._data.particle_model.name + """.h"
#include \"""" + self._data.namespace[-1] + """/""" + self._data.name + """.h"
"""


  def construct_output(self,output):
    d = { 
      "PARTICLE_TYPE": self._data.particle_model.name
    } 

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
    


class ParticleSet(DoF):
  """
  
    :Attributes:
    
    name: String
      Name of the particle. Has to be a valid C++ class name.

    particle: Particle
      Link to particle definition
  
  """
  def __init__(self,particle):
    DoF.__init__(self,particle.name + "Set")
    
    if particle.association != peano4.datamodel.DoFAssociation.Global:
      print( "Warning: particle " + particle.name + " is not (yet) added to project via add_global_object() and thus has invalid DoF association" )
    
    self.particle_model   = particle
    self.generator        = ParticleSetGenerator(self)



  #def get_full_qualified_type(self):
  #  result = "std::vector< "
  #  for i in self.namespace:
  #    result += i
  #    result += "::"
  #  result += self.name
  #  result += ">"
  #  return result
    