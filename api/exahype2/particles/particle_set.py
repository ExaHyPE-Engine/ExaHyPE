# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import exahype2.codegen
import exahype2.datamodel

from exahype2.datamodel.dof import DoF


class AbstractParticleSetGenerator:
    def __init__(self, data):
        self._data = data

    def get_stack_container(self):
        particle_set_type = self._data.get_full_qualified_type()
        return f"::peano4::grid::Stack<{particle_set_type}>"


class ParticleSetGenerator(AbstractParticleSetGenerator):
    def construct_output(self, output):
        dictionary = {
            "PARTICLE_TYPE": self._data.particle_model.name,
            "PARTICLE_FULL_QUALIFIED_TYPE": self._data.particle_model.get_full_qualified_type(),
        }
        template_file_prefix = os.path.join(
            os.path.dirname(os.path.realpath(__file__)), "ParticleSet"
        )

        generated_files = exahype2.codegen.TemplatedFile(
            template_file_prefix + ".h.jinja2",
            template_file_prefix + ".cpp.jinja2",
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
    """Models a vertex-associated container of heap-allocated particles."""

    def __init__(self, particle):
        super().__init__(particle.name + "Set")
        if particle.association != exahype2.datamodel.DoFAssociation.Global:
            raise ValueError("A particle model has to use global association.")
        self.particle_model = particle
        self.generator = ParticleSetGenerator(self)

    def __str__(self):
        return f"{{{{ {self.particle_model.name}, {self.generator.__class__} }}}}"
