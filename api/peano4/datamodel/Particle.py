# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .DoF import DoF
from .ParticleToData import ParticleToData


class Particle(DoF):
    readme_descriptor = """
"""

    readme_package_descriptor = """
"""

    def __init__(self, name):
        super(Particle, self).__init__(name)
        self.attributes = []
        self.includes = ""
        self.generator = ParticleToData(self)

    def add_attribute(self, attribute):
        self.attributes.append(attribute)
