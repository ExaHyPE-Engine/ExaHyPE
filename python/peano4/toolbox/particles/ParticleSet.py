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
    return "peano4::stacks::STDVectorOverVectorOfPointersStack< " + self._data.get_full_qualified_type() + " >";

    
  def get_header_file_include(self):
    return """
#include "peano4/stacks/STDVectorOverVectorOfPointersStack.h"
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


  @property
  def readme_descriptor(self):
    return """
### Particle """ + self.particle_model.name + """

There's no additional information available about this particle
data structure.

"""


  @property
  def readme_package_descriptor(self):
    return """

### Particle handling 
                                                                            
The particle handling is based upon the idea of particles in dual trees (PIDT)
as published in the paper below:                                              
                                                                            
       @article{Weinzierl:2016:PIDT, 
         title = {Two particle-in-grid realisations on spacetrees}, 
         journal = {Parallel Computing}, 
         volume = {52}, 
         pages = {42-64}, 
         year = {2016}, 
         issn = {0167-8191}, 
         doi = {https://doi.org/10.1016/j.parco.2015.12.007}, 
         url = {https://www.sciencedirect.com/science/article/pii/S0167819115001635}, 
         author = {T. Weinzierl and B. Verleye and P. Henri and D. Roose}, 
         keywords = {Particle-in-cell, Spacetree, Particle sorting, AMR, Lagrangian–Eulerian methods, Communication-avoiding}, 
         abstract = {The present paper studies two particle management strategies for dynamically adaptive Cartesian grids at hands of a particle-in-cell code. One holds the particles within the grid cells, the other within the grid vertices. The fundamental challenge for the algorithmic strategies results from the fact that particles may run through the grid without velocity constraints. To facilitate this, we rely on multiscale grid representations. They allow us to lift and drop particles between different spatial resolutions. We call this cell-based strategy particle in tree (PIT). Our second approach assigns particles to vertices describing a dual grid (PIDT) and augments the lifts and drops with multiscale linked cells. Our experiments validate the two schemes at hands of an electrostatic particle-in-cell code by retrieving the dispersion relation of Langmuir waves in a thermal plasma. They reveal that different particle and grid characteristics favour different realisations. The possibility that particles can tunnel through an arbitrary number of grid cells implies that most data is exchanged between neighbouring ranks, while very few data is transferred non-locally. This constraints the scalability as the code potentially has to realise global communication. We show that the merger of an analysed tree grammar with PIDT allows us to predict particle movements among several levels and to skip parts of this global communication a priori. It is capable to outperform several established implementations based upon trees and/or space-filling curves.}
       }

"""
    