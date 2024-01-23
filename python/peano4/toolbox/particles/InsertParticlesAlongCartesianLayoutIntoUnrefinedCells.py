# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


class InsertParticlesAlongCartesianLayoutIntoUnrefinedCells(ActionSet):
    def __init__(
        self,
        particle_set,
        distance_between_particles,
        computational_domain_offset,
        computational_domain_size,
        initialisation_call="",
        noise=True,
        additional_includes="",
        guard="true",
    ):
        """!
        Insert particles


        :Attibutes:

        particle: ParticleSet
           I take this as particle set.

        average_distance_between_particles: Float
           Some positive floating point value.

        initialisation_call: String (C code)
           Within this code snippet, you have a variable particle which is a pointer to
           your particle type. You can use it to set the particle attributes.

        computational_domain_offset: [Float]
           Has to be an array with Dimensions entries that specifies the global domain
           offset. You can pass in an array of floats, and this is definitely the
           default use case. However, you can also pass in an arbitrary array of
           strings, as long as these strings make sense in the generated C++ code
           and eventually return a double each.

        computational_domain_size: [Float]
           Defines size of domain.

        guard: String (C code)
           A simple boolean expression. You can decide if a particle is inserted or not.

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
        super(InsertParticlesAlongCartesianLayoutIntoUnrefinedCells, self).__init__(
            descend_invocation_order=1, parallel=False
        )

        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d["INITIALISATION_CALL"] = initialisation_call
        self.d["DOMAIN_OFFSET"] = computational_domain_offset
        self.d["DOMAIN_SIZE"] = computational_domain_size
        self.d["H"] = distance_between_particles
        self.d["GUARD"] = guard
        if noise:
            self.d["NOISE"] = "true"
        else:
            self.d["NOISE"] = "false"
        self.additional_includes = additional_includes

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  if ( not marker.hasBeenRefined() and not marker.willBeRefined() ) {
    #if Dimensions==2
    tarch::la::Vector<2,double> domainOffset = { {{DOMAIN_OFFSET[0]}}, {{DOMAIN_OFFSET[1]}} };
    tarch::la::Vector<2,double> domainSize   = { {{DOMAIN_SIZE[0]}}, {{DOMAIN_SIZE[1]}} };
    #else
    tarch::la::Vector<3,double> domainOffset = { {{DOMAIN_OFFSET[0]}}, {{DOMAIN_OFFSET[1]}}, {{DOMAIN_OFFSET[2]}} };
    tarch::la::Vector<3,double> domainSize   = { {{DOMAIN_SIZE[0]}}, {{DOMAIN_SIZE[1]}}, {{DOMAIN_SIZE[2]}} };
    #endif
    std::vector< globaldata::{{PARTICLE}}* > newParticles = toolbox::particles::createParticlesAlignedWithGlobalCartesianMesh<globaldata::{{PARTICLE}}>(
      {{H}},
      marker.x(),
      marker.h(),
      domainOffset,
      domainSize,
      {{NOISE}}
    );
    for (auto* particle: newParticles) {
      if ( {{GUARD}} ) {
        // See documentation of constructor on initialisation snippet
        {{INITIALISATION_CALL}}
        toolbox::particles::insertParticleIntoCell(
          marker,
          particle,
          fineGridVertices{{PARTICLES_CONTAINER}},
          _spacetreeId
        );
      }
      else {
        delete particle;
      }
    }
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
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "toolbox/particles/particles.h"
#include "toolbox/particles/ParticleFactory.h"
"""
        )
        return result.render(**self.d) + self.additional_includes

    def get_attributes(self):
        return f"""
    int                                        _spacetreeId;
"""

    def get_constructor_body(self):
        return """
  _spacetreeId              = treeNumber;
"""
