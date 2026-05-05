# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer


class InsertParticlesByCoordinates(
    peano4.toolbox.particles.InsertParticlesByCoordinates
):
    """
    Insert particles by explicit coordinates and assign spacetree-local IDs.

    Adds spacetree ID and local particle number to each inserted particle.
    """

    def __init__(
        self,
        particle_set,
        coordinates,
        additional_includes="",
    ):
        # Set dictionary for template rendering
        self.d = {
            "PARTICLE": particle_set.particle_model.name,
        }

        # Call superclass constructor with correct templated code
        super(InsertParticlesByCoordinates, self).__init__(
            particle_set=particle_set,
            coordinates=coordinates,
            initialisation_call=jinja2.Template(
                """
      particle->setNumber(0,_spacetreeId);
      particle->setNumber(1,_particleNumberOnThisTree);
      particle->setMoveState(globaldata::{{PARTICLE}}::MoveState::NotMovedYet);

      _particleNumberOnThisTree++;

      logInfo("Insert particle at " << particle->getX());
      """
            ).render(**self.d),
            additional_includes=additional_includes
            + jinja2.Template(
                """
#include "Constants.h"
#include "globaldata/{{PARTICLE}}.h"
"""
            ).render(**self.d),
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_constructor_body(self):
        return (
            super(InsertParticlesByCoordinates, self).get_constructor_body()
            + """
  _particleNumberOnThisTree = 0;
"""
        )

    def get_attributes(self):
        return (
            super(InsertParticlesByCoordinates, self).get_attributes()
            + """
  int _particleNumberOnThisTree;
"""
        )
