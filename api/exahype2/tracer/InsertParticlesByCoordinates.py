# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2
import peano4.particles
from peano4.solversteps.ActionSet import ActionSet


class InsertParticlesByCoordinates(peano4.particles.InsertParticlesByCoordinates):
    """
    Adds the spacetree number and a tree-local particle number to every inserted
    particle.
    """

    def __init__(
        self,
        particle_set,
        coordinates,
        additional_includes="",
    ):
        dictionary = {
            "PARTICLE": particle_set.particle_model.name,
        }

        super().__init__(
            particle_set=particle_set,
            coordinates=coordinates,
            initialisation_call=jinja2.Template(
                """
      particle->setNumber(0, _treeId);
      particle->setNumber(1, _particleNumberOnThisTree);
      particle->setMoveState(globaldata::{{PARTICLE}}::MoveState::NotMovedYet);

      _particleNumberOnThisTree++;

      logInfo("Insert particle at " << particle->getX());
      """
            ).render(**dictionary),
            additional_includes=additional_includes
            + jinja2.Template(
                """
#include "Constants.h"
#include "globaldata/{{PARTICLE}}.h"
#include "runner/RuntimeParameters.h"
#include "tarch/instrumentation/Info.h"
"""
            ).render(**dictionary),
        )

    def get_body_of_operation(self, operation_name):
        body = super().get_body_of_operation(operation_name)
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            # A resumed run takes its particles from the checkpoint loader, so
            # the insertion stays off while a load path is set.
            return (
                "  if (not ::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath()) {\n"
                + body
                + "  }\n"
            )
        return body

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_constructor_body(self):
        return (
            super().get_constructor_body()
            + """
  _particleNumberOnThisTree = 0;
"""
        )

    def get_attributes(self):
        return (
            super().get_attributes()
            + """
  int _particleNumberOnThisTree{0};
"""
        )
