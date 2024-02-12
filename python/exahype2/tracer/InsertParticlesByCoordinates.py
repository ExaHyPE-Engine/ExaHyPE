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

    Basically superclass, though we add these numbers.

    """

    def __init__(
        self,
        particle_set,
        coordinates,
        additional_includes="",
    ):
        """

        See superclass.

        """
        super(InsertParticlesByCoordinates, self).__init__(
            particle_set=particle_set,
            coordinates=coordinates,
            initialisation_call=f"""
      particle->setNumber(0,_spacetreeId);
      particle->setNumber(1,_particleNumberOnThisTree);
      particle->setMoveState(globaldata::{particle_set.particle_model.name}::MoveState::NotMovedYet);

      _particleNumberOnThisTree++;
      
      logInfo( "touchVertexFirstTime(...)", "insert particle at " << particle->getX() );
      """,
            additional_includes=additional_includes
            + """  
#include "Constants.h"
""",
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
