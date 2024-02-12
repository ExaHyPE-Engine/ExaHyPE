# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


class InsertRandomParticlesIntoUnrefinedCells(ActionSet):
    """!

    Insert particles

    The particles are topologically Cartesian, i.e. they are not totally random. So
    we embed a Cartesian "grid" of particles into each unrefined cell, and then add
    some noise to them. As the individual cells are not coupled with each other, and
    as we do not evaluate any global coordinates, the overall result will not be
    random, even if you switch off noise completely, as the spacing between two
    regular particle layouts between two neighbouring cells is not "synchronised".

    """

    def __init__(
        self,
        particle_set,
        average_distance_between_particles,
        initialisation_call,
        round_down=False,
        noise=True,
        additional_includes="",
    ):
        """!

        Insert particles

        The particles are topologically Cartesian, i.e. they are not totally random. So
        we embed a Cartesian "grid" of particles into each unrefined cell, and then add
        some noise to them. As the individual cells are not coupled with each other, and
        as we do not evaluate any global coordinates, the overall result will not be
        random, even if you switch off noise completely, as the spacing between two
        regular particle layouts between two neighbouring cells is not "synchronised".

        :Attibutes:

        particle: ParticleSet
           I take this as particle set.

        average_distance_between_particles: Float
           Some positive floating point value. I call it average, as it is an
           average if you apply noise. Otherwise, it is the precise distance.

        initialisation_call: String (C++ code snippet)
          Arbitrary code snippet which can work over a pointer particle. The object
          to which this pointer points to is properly allocated, and its
          coordinates are set. Furthermore, the object's search radius is
          initialised with zero. You can alter the particle attributes now, i.e.
          initialise the domain-specific data. Please also ensure you assign the
          particle a proper search (interaction) radius if the radius is of
          relevance for your application.

          Please note that we create a lot of particles and then decide if we
          actually insert them. Only those particles where we come to the
          conclusion that we should insert them (as they overlap with the cell)
          are actually then initialised through this code snippet.

        """
        super(InsertRandomParticlesIntoUnrefinedCells, self).__init__(
            descend_invocation_order=1, parallel=False
        )

        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d["INITIALISATION_CALL"] = initialisation_call
        self.d["H"] = average_distance_between_particles
        if noise:
            self.d["NOISE"] = "true"
        else:
            self.d["NOISE"] = "false"
        if round_down:
            self.d["ROUND_DOWN"] = "true"
        else:
            self.d["ROUND_DOWN"] = "false"
        self.additional_includes = additional_includes

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  if ( not marker.hasBeenRefined() and not marker.willBeRefined() ) {
    std::vector< globaldata::{{PARTICLE}}* > newParticles = toolbox::particles::createEquallySpacedParticles<globaldata::{{PARTICLE}}>(
      {{H}},
      marker.x(),
      marker.h(),
      {{ROUND_DOWN}},
      {{NOISE}}
    );
    for (auto* particle: newParticles) {
      // See documentation of constructor on initialisation snippet
      {{INITIALISATION_CALL}}
    }
    toolbox::particles::insertParticlesIntoCell(
      marker,
      newParticles,
      fineGridVertices{{PARTICLES_CONTAINER}}
    );
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCellFirstTime.render(**self.d)
        return result

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        result = jinja2.Template(
            """
#include "tarch/multicore/multicore.h"
#include "toolbox/particles/particles.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "toolbox/particles/ParticleFactory.h"
"""
        )
        return result.render(**self.d) + self.additional_includes
