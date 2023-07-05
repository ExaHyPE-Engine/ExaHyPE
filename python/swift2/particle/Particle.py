# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles

import dastgen2
from abc import abstractmethod


class Particle(peano4.toolbox.particles.Particle):
    """!

    Base class for any particle in the project

    You usually do not initialise this base class, but you pick one of the
    subclasses which define which time stepping scheme is realised by a
    particle. The time stepping scheme defines through which phases or steps
    each particle runs through per time step, while this generic base class
    has no steps at all.

    The type is a specialisation of Peano's particle as it is provided via the
    toolbox. In line with the docu there, you can use this particle and add
    further attributes via

        my_particle.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("v","Dimensions") )



    """

    class SpeciesAspect(object):
        """

        Every generated particle belongs to a species. I originally thought I'd
        use some kind of inheritance to model this, but actually it is easier to
        introduce a static field for the time being that stores the species
        information.

        """

        def __init__(self):
            pass

        def get_include(self):
            return """
#include "swift2/ParticleSpecies.h"
"""

        def set_model(self, data_model):
            self._data_model = data_model

        def get_attributes(self):
            return ""

        def get_method_declarations(self, full_qualified_name):
            return """
  static ::swift2::ParticleSpecies& getSpecies();
"""

        def get_implementation(self, full_qualified_name):
            return (
                """
::swift2::ParticleSpecies& """
                + full_qualified_name
                + """::getSpecies() {
  static ::swift2::ParticleSpecies species;
  return species;
}
"""
            )

    def __init__(
        self,
        name,
        particles_per_cell,
        min_h,
        max_h,
    ):
        """!

        Initialise the particle

        This is the baseclass for a particle, i.e. we only add the absolute minimum
        of information to a particle. As we inherit from the toolbox particle, we
        already have some attributes defined. These are the guys which are not
        explicitly visible from the code snippet below, i.e. they are introduced by
        the superclass constructor.

        Here's an overview of pre-defined attributes that each and every particle
        hosts:

        - A position which is a double vector with Dimension entries.
        - A search radius which describes the range with which other particles a
          particle might theoretically interact. The effective interaction might
          be smaller if a code decides to ignore potential interaction partners,
          i.e. this is an absolute maximum. The search radius will decide on which
          resolution level within the tree a particle is held.
        - A MoveState flag. This flag is essential. We use it later to label those
          particles that have been moved to avoid that we move particles multiple
          times.
        - A flag that indicates if a particle has been updated within a cell.
          Consult our code release papers: In short, a particle does not uniquely
          belong to one cell but can belong to many cells. We nevertheless want to
          update them only once.

        """
        super(Particle, self).__init__(name)

        self.data.add_attribute(
            dastgen2.attributes.Enumeration(
                "MoveState", ["New", "NotMovedYet", "Moved"]
            )
        )
        self.data.add_attribute(dastgen2.attributes.Boolean("CellHasUpdatedParticle"))
        self.data.add_aspect(Particle.SpeciesAspect())

        if particles_per_cell <= 0:
            print(
                "WARNING: Particles per cell set to zero, so code might refine up to maximum depth even if there is only one particle"
            )
        self.particles_per_cell = particles_per_cell
        self.min_h = min_h
        self.max_h = max_h

        pass

    @abstractmethod
    def algorithm_steps(self):
        """!

        Return sequence of algorithm steps that have to be called per time step

        """
        assert False, "Should be overwritten by subclass"
        return []

    @abstractmethod
    def initialisation_steps(self):
        """!

        Return sequence of algorithm steps that have to be performed throughout initialisation

        """
        assert False, "Should be overwritten by subclass"
        return []

    @property
    def readme_descriptor(self):
        """!

        Create default readme descriptor

        You might want to overwrite this for your particular particle
        species to get more detailed info the the README.md file generated
        by Peano.

        """
        return (
            """### Particle """
            + self.name
            + """

  Mesh properties:

  - particles per cell:         """
            + str(self.particles_per_cell)
            + """
  - h_min:                      """
            + str(self.min_h)
            + """
  - h_max:                      """
            + str(self.max_h)
            + """

  Attributes: """
            + str(len(self.data._attributes))
            + """

    """
        )
