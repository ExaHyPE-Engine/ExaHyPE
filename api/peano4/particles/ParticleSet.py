# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import os

import peano4.datamodel
import peano4.output

from peano4.datamodel.DoF import DoF


class AbstractParticleSetGenerator:
    def __init__(self, data):
        self._data = data

    def get_stack_container(self):
        particle_set_type = self._data.get_full_qualified_type()
        return f"::peano4::grid::Stack<{particle_set_type}>"

    def get_header_file_include(self):
        output_namespace = self._data.namespace[-1]
        return f"""
#include "peano4/grid/Stack.h"
#include "{output_namespace}/{self._data.name}.h"
"""


class ParticleSetGenerator(AbstractParticleSetGenerator):
    def construct_output(self, output):
        dictionary = {
            "PARTICLE_TYPE": self._data.particle_model.name,
            "PARTICLE_FULL_QUALIFIED_TYPE": self._data.particle_model.get_full_qualified_type(),
        }
        template_file_prefix = os.path.splitext(os.path.realpath(__file__))[0]

        generated_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            template_file_prefix + ".template.h",
            template_file_prefix + ".template.cpp",
            self._data.name,
            self._data.namespace,
            self._data.namespace[-1],
            dictionary,
            apply_iteratively=True,
        )
        output.add(generated_files)
        output.makefile.add_cpp_file(
            self._data.namespace[-1] + "/" + self._data.name + ".cpp", generated=True
        )


class ParticleSet(DoF):
    """Models a vertex-associated container of heap-allocated particles.

    The generator emits the C++ container, ownership operations, sieve access,
    and MPI serialisation for the supplied global particle model.
    """

    def __init__(self, particle):
        super().__init__(particle.name + "Set")
        if particle.association != peano4.datamodel.DoFAssociation.Global:
            raise ValueError("A particle model has to use global association.")
        self.particle_model = particle
        self.generator = ParticleSetGenerator(self)

    @property
    def readme_descriptor(self):
        return f"""
### Particle set {self.particle_model.name}

The particle set uses `{self.generator.get_stack_container()}`.

"""

    @property
    def readme_package_descriptor(self):
        return """
### Particle handling

Peano associates particles with vertices of a dual spacetree and moves them
between mesh levels as their position or search radius changes. The algorithm is
described by T. Weinzierl et al., *Two particle-in-grid realisations on
spacetrees*, Parallel Computing 52 (2016), 42–64,
https://doi.org/10.1016/j.parco.2015.12.007.
"""

    def __str__(self):
        return f"{{{{ {self.particle_model.name}, {self.generator.__class__} }}}}"
