# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.solversteps.ActionSet import ActionSet


class InsertParticlesAlongCartesianLayoutIntoUnrefinedCells(ActionSet):
    """Inserts a domain-wide Cartesian particle layout into leaf cells.

    ``distance_between_particles`` is the positive global spacing. Domain
    offset and size entries may be numbers or generated C++ expressions.
    ``noise_scale`` multiplies the random displacement of at most half the
    spacing per coordinate. The factory initialises each particle before
    ``initialisation_call`` runs, and ``guard`` selects which particles enter
    the mesh.
    """

    def __init__(
        self,
        particle_set,
        distance_between_particles,
        computational_domain_offset,
        computational_domain_size,
        initialisation_call="",
        noise=True,
        noise_scale=1.0,
        additional_includes="",
        guard="true",
    ):
        super().__init__(descend_invocation_order=1)

        self._dictionary = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "INITIALISATION_CALL": initialisation_call,
            "DOMAIN_OFFSET": computational_domain_offset,
            "DOMAIN_SIZE": computational_domain_size,
            "SPACING": distance_between_particles,
            "GUARD": guard,
            "NOISE": "true" if noise else "false",
            "NOISE_SCALE": noise_scale,
        }
        self._additional_includes = additional_includes

    _Template_TouchCellFirstTime = jinja2.Template(
        """
  if (not marker.hasBeenRefined() and not marker.willBeRefined()) {
#if DIMENSIONS == 2
    const tarch::la::Vector<2, double> domainOffset{
      {{DOMAIN_OFFSET[0]}},
      {{DOMAIN_OFFSET[1]}}
    };
    const tarch::la::Vector<2, double> domainSize{
      {{DOMAIN_SIZE[0]}},
      {{DOMAIN_SIZE[1]}}
    };
#else
    const tarch::la::Vector<3, double> domainOffset{
      {{DOMAIN_OFFSET[0]}},
      {{DOMAIN_OFFSET[1]}},
      {{DOMAIN_OFFSET[2]}}
    };
    const tarch::la::Vector<3, double> domainSize{
      {{DOMAIN_SIZE[0]}},
      {{DOMAIN_SIZE[1]}},
      {{DOMAIN_SIZE[2]}}
    };
#endif
    auto newParticles =
      toolbox::particles::createParticlesAlignedWithGlobalCartesianMesh<
        globaldata::{{PARTICLE}}
      >(
        {{SPACING}},
        marker.getX(),
        marker.getH(),
        domainOffset,
        domainSize,
        {{NOISE}},
        {{NOISE_SCALE}}
      );
    for (auto* const particle : newParticles) {
      if ({{GUARD}}) {
        {{INITIALISATION_CALL}}
        toolbox::particles::insertParticleIntoCell(
          marker,
          particle,
          fineGridVertices{{PARTICLES_CONTAINER}},
          _spacetreeId
        );
      } else {
        delete particle;
      }
    }
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self._Template_TouchCellFirstTime.render(**self._dictionary)
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector<peano4::grid::GridControlEvent>{};\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        template = jinja2.Template(
            """
#include "toolbox/particles/ParticleFactory.h"
#include "toolbox/particles/particles.h"

#include "globaldata/{{PARTICLE}}.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
"""
        )
        return template.render(**self._dictionary) + self._additional_includes

    def get_attributes(self):
        return "  int _spacetreeId;\n"

    def get_constructor_body(self):
        return "  _spacetreeId = treeNumber;\n"
