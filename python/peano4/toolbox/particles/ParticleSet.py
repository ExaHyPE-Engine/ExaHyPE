# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.datamodel.DaStGen2

from peano4.datamodel.DoF import DoF
from peano4.datamodel.DaStGen2 import DaStGen2Generator

from .Particle import Particle

import dastgen2
import os


from enum import Enum


class AbstractParticleSetGenerator(object):
    def __init__(
        self,
        data,
        gather_particles,
    ):
        self._data = data
        self._gather_particles = gather_particles

    @property
    def gather_particles(self):
        return self._gather_particles

    def get_stack_container(self):
        return (
            "peano4::stacks::STDVectorOverContainerOfPointers< "
            + self._data.get_full_qualified_type()
            + " >"
        )

    def get_header_file_include(self):
        return (
            """
#include "peano4/stacks/STDVectorOverContainerOfPointers.h"
#include "../globaldata/"""
            + self._data.particle_model.name
            + """.h"
#include \""""
            + self._data.namespace[-1]
            + """/"""
            + self._data.name
            + """.h"
"""
        )


class ParticleSetGenerator_ScatteredOnHeap_IndexByList(AbstractParticleSetGenerator):
    """!

    Map a particle set onto heap objects indexed by a list

    This class is tied to peano4::stacks::STDVectorOverContainerOfPointers, i.e.
    Peano's stacks are realised via std::vector from the C++ standard library.
    Each entry within this vector then resembles a C++ container over pointers
    to the heap. That is, all particles are stored on the heap. They are
    scattered. The C++ pointer per stack entry that's used to store all the
    particle pointers is realised through std::list.

    Please consult @ref page_swift_performance_optimisation "the generic discussion on the impact of storage schemes".

    @image html ParticleSetGenerator_ScatteredOnHeap.png


    """

    def __init__(self, data):
        super(ParticleSetGenerator_ScatteredOnHeap_IndexByList, self).__init__(
            data, False
        )

    def construct_output(self, output):
        d = {"PARTICLE_TYPE": self._data.particle_model.name, "STD_CONTAINER": "list"}

        templatefile_prefix = (
            os.path.realpath(__file__).replace(".pyc", "").replace(".py", "")
        )

        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            templatefile_prefix + "_ScatteredOnHeap_IndexedByStdContainer.template.h",
            templatefile_prefix + "_ScatteredOnHeap_IndexedByStdContainer.template.cpp",
            self._data.name,
            self._data.namespace,
            self._data.namespace[-1],
            d,
            True,
        )
        output.add(generated_files)
        output.makefile.add_cpp_file(
            self._data.namespace[-1] + "/" + self._data.name + ".cpp", generated=True
        )
        pass


class ParticleSetGenerator_ScatteredOnHeap_IndexByVector(AbstractParticleSetGenerator):
    """!

    Map a particle set onto heap objects indexed by an std::vector

    This class is tied to peano4::stacks::STDVectorOverContainerOfPointers, i.e.
    Peano's stacks are realised via std::vector from the C++ standard library.
    Each entry within this vector then resembles a C++ container over pointers
    to the heap. That is, all particles are stored on the heap. They are
    scattered. The C++ pointer per stack entry that's used to store all the
    particle pointers is realised through std::list.

    Please consult @ref page_swift_performance_optimisation "the generic discussion on the impact of storage schemes".

    @image html ParticleSetGenerator_ScatteredOnHeap.png


    """

    def __init__(self, data):
        super(ParticleSetGenerator_ScatteredOnHeap_IndexByVector, self).__init__(
            data, False
        )

    def construct_output(self, output):
        d = {"PARTICLE_TYPE": self._data.particle_model.name, "STD_CONTAINER": "vector"}

        templatefile_prefix = (
            os.path.realpath(__file__).replace(".pyc", "").replace(".py", "")
        )

        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            templatefile_prefix + "_ScatteredOnHeap_IndexedByStdContainer.template.h",
            templatefile_prefix + "_ScatteredOnHeap_IndexedByStdContainer.template.cpp",
            self._data.name,
            self._data.namespace,
            self._data.namespace[-1],
            d,
            True,
        )
        output.add(generated_files)
        output.makefile.add_cpp_file(
            self._data.namespace[-1] + "/" + self._data.name + ".cpp", generated=True
        )
        pass


class ParticleSetGenerator_ContinuousPerVertex(AbstractParticleSetGenerator):
    """!

    Map a particle set onto heap objects indexed by a list

    This class is tied to peano4::stacks::STDVectorOverContainerOfPointers, i.e.
    Peano's stacks are realised via std::vector from the C++ standard library.
    Each entry within this vector then resembles a C++ container over pointers
    to the heap. That is, all particles are stored on the heap. They are
    scattered. The C++ pointer per stack entry that's used to store all the
    particle pointers is realised through std::list.

    Please consult @ref page_swift_performance_optimisation "the generic discussion on the impact of storage schemes".

    @image html ParticleSetGenerator_ContinuousPerVertex.png

    """

    def __init__(self, data):
        super(ParticleSetGenerator_ContinuousPerVertex, self).__init__(data, True)

    def construct_output(self, output):
        d = {
            "PARTICLE_TYPE": self._data.particle_model.name,
            "MEMORY_POOL_TYPE": "toolbox::particles::memorypool::VertexWiseContinuousMemoryPool",
        }

        templatefile_prefix = (
            os.path.realpath(__file__).replace(".pyc", "").replace(".py", "")
        )

        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            templatefile_prefix + "_MemoryPool.template.h",
            templatefile_prefix + "_MemoryPool.template.cpp",
            self._data.name,
            self._data.namespace,
            self._data.namespace[-1],
            d,
            True,
        )
        output.add(generated_files)
        output.makefile.add_cpp_file(
            self._data.namespace[-1] + "/" + self._data.name + ".cpp", generated=True
        )
        pass


class ParticleSetGenerator_GlobalContinuous(AbstractParticleSetGenerator):
    """!

    Map a particle set onto heap objects indexed by a list

    This class is tied to peano4::stacks::STDVectorOverContainerOfPointers, i.e.
    Peano's stacks are realised via std::vector from the C++ standard library.
    Each entry within this vector then resembles a C++ container over pointers
    to the heap. That is, all particles are stored on the heap. They are
    scattered. The C++ pointer per stack entry that's used to store all the
    particle pointers is realised through std::list.

    Please consult @ref page_swift_performance_optimisation "the generic discussion on the impact of storage schemes".

    @image html ParticleSetGenerator_GlobalContinuous.png

    """

    def __init__(self, data):
        super(ParticleSetGenerator_GlobalContinuous, self).__init__(data, True)

    def construct_output(self, output):
        d = {
            "PARTICLE_TYPE": self._data.particle_model.name,
            "MEMORY_POOL_TYPE": "toolbox::particles::memorypool::GlobalContinuousMemoryPool",
        }

        templatefile_prefix = (
            os.path.realpath(__file__).replace(".pyc", "").replace(".py", "")
        )

        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            templatefile_prefix + "_MemoryPool.template.h",
            templatefile_prefix + "_MemoryPool.template.cpp",
            self._data.name,
            self._data.namespace,
            self._data.namespace[-1],
            d,
            True,
        )
        output.add(generated_files)
        output.makefile.add_cpp_file(
            self._data.namespace[-1] + "/" + self._data.name + ".cpp", generated=True
        )
        pass


class ParticleSet(DoF):
    """!

    Represents a particle set

    A particle set is, in principle, a very simple container tied to a vertex.
    It allows each vertex to point to the particles around. While the
    realisation on the Python side is simple - after all, the Python model is
    solely the data model - interesting questions arise once we discuss how the
    data model is mapped onto C++. This information is not directly stored
    within the ParticleSet objects, i.e. this class is solely a presentation
    of the data topology.

    The information how the association is mapped onto Peano's C++ code is held
    within the generator. By default, this generator maps each particle set onto
    a vector which in turn hosts particles on the heap. However, there are
    alternative implementations which you might want to pick by setting the
    generator attribute to a different value.

    - Study ParticleSetGenerator_ScatteredOnHeap_IndexByList to obtain some
      information about Peano's default storage scheme.
    - The generator ParticleSetGenerator_ScatteredOnHeap_IndexByVector is
      a slightly more memory-compact version.


    ## Attributes

    name: String
      Name of the particle. Has to be a valid C++ class name.

    particle: Particle
      Link to particle definition

    """

    def __init__(self, particle):
        DoF.__init__(self, particle.name + "Set")

        if particle.association != peano4.datamodel.DoFAssociation.Global:
            print(
                "Warning: particle "
                + particle.name
                + " is not (yet) added to project via add_global_object() and thus has invalid DoF association"
            )

        self.particle_model = particle
        # self.generator      = ParticleSetGenerator_ScatteredOnHeap_IndexByList(self)
        self.generator = ParticleSetGenerator_ScatteredOnHeap_IndexByVector(self)
        # self.generator      = ParticleSetGenerator_ContinuousPerVertex(self)
        # self.generator      = ParticleSetGenerator_GlobalContinuous(self)

    @property
    def readme_descriptor(self):
        return (
            """
### Particle set """
            + self.particle_model.name
            + """

The particle set is administered through the container """
            + self.generator.get_stack_container()
            + """.

"""
        )

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
         keywords = {Particle-in-cell, Spacetree, Particle sorting, AMR, Lagrangian-Eulerian methods, Communication-avoiding}, 
         abstract = {The present paper studies two particle management strategies for dynamically adaptive Cartesian grids at hands of a particle-in-cell code. One holds the particles within the grid cells, the other within the grid vertices. The fundamental challenge for the algorithmic strategies results from the fact that particles may run through the grid without velocity constraints. To facilitate this, we rely on multiscale grid representations. They allow us to lift and drop particles between different spatial resolutions. We call this cell-based strategy particle in tree (PIT). Our second approach assigns particles to vertices describing a dual grid (PIDT) and augments the lifts and drops with multiscale linked cells. Our experiments validate the two schemes at hands of an electrostatic particle-in-cell code by retrieving the dispersion relation of Langmuir waves in a thermal plasma. They reveal that different particle and grid characteristics favour different realisations. The possibility that particles can tunnel through an arbitrary number of grid cells implies that most data is exchanged between neighbouring ranks, while very few data is transferred non-locally. This constraints the scalability as the code potentially has to realise global communication. We show that the merger of an analysed tree grammar with PIDT allows us to predict particle movements among several levels and to skip parts of this global communication a priori. It is capable to outperform several established implementations based upon trees and/or space-filling curves.}
       }

"""

    def __str__(self):
        return "{{ {}, {} }}".format(self.particle_model.name, self.generator.__class__)
