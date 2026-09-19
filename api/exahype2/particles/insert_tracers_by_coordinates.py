# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from .insert_particles_by_coordinates import InsertParticlesByCoordinates


class InsertTracersByCoordinates(InsertParticlesByCoordinates):
    """Inserts tracers at fixed coordinates and gives every tracer the spacetree number and a
    tree-local running number."""

    def __init__(self, particle_set, coordinates):
        super().__init__(
            particle_set=particle_set,
            coordinates=coordinates,
            initialisation_call=jinja2.Template("""
  particle->setNumber(0, _treeId);
  particle->setNumber(1, _particleNumberOnThisTree);
  particle->setMoveState(globaldata::{{PARTICLE}}::MoveState::NotMovedYet);

  _particleNumberOnThisTree++;

  logInfo("Insert particle at " << particle->getX());
""").render(PARTICLE=particle_set.particle_model.name),
        )

    def includes(self):
        return super().includes() + ["tarch/instrumentation/Info.h"]

    def get_constructor_body(self):
        return super().get_constructor_body() + """
  _particleNumberOnThisTree = 0;
"""

    def attributes(self):
        return super().attributes() + """
  int _particleNumberOnThisTree{0};
"""
