# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.datamodel


class Particle(peano4.datamodel.Particle):
    def __init__(self, name):
        super(Particle, self).__init__(name)

        self.name = name
        self.add_attribute(peano4.datamodel.DoubleArray("x", "DIMENSIONS"))
        self.add_attribute(peano4.datamodel.DoubleArray("cellH", "DIMENSIONS"))
        self.add_attribute(peano4.datamodel.Double("searchRadius"))
        self.add_attribute(peano4.datamodel.Integer("partid", initval="-1"))
        self.add_attribute(
            peano4.datamodel.BooleanArray("containedInAdjacentCell", "peano4::CellsPerVertex")
        )
        self.add_attribute(
            peano4.datamodel.Enumeration("parallelState", ["Local", "Virtual"])
        )
