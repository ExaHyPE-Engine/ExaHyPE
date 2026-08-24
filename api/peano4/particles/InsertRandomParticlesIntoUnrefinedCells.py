# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.solversteps.ActionSet import ActionSet


class InsertRandomParticlesIntoUnrefinedCells(ActionSet):
    """Inserts a perturbed cell-local Cartesian layout into leaf cells.

    ``average_distance_between_particles`` is the positive target spacing.
    Each cell constructs its layout independently, so particles are not
    globally equidistant across cell faces. The factory initialises every
    particle before ``initialisation_call`` runs.
    """

    def __init__(
        self,
        particle_set,
        average_distance_between_particles,
        initialisation_call,
        noise=True,
        additional_includes="",
    ):
        super().__init__(descend_invocation_order=1)

        self._dictionary = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "INITIALISATION_CALL": initialisation_call,
            "SPACING": average_distance_between_particles,
            "NOISE": "true" if noise else "false",
        }
        self._additional_includes = additional_includes

    _Template_TouchCellFirstTime = jinja2.Template(
        """
  if (not marker.hasBeenRefined() and not marker.willBeRefined()) {
    auto newParticles = toolbox::particles::createEquallySpacedParticles<
      globaldata::{{PARTICLE}}
    >(
      {{SPACING}},
      marker.getX(),
      marker.getH(),
      {{NOISE}}
    );
    for (auto* const particle : newParticles) {
      {{INITIALISATION_CALL}}
    }
    toolbox::particles::insertParticlesIntoCell(
      marker,
      newParticles,
      fineGridVertices{{PARTICLES_CONTAINER}},
      _spacetreeId
    );
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
