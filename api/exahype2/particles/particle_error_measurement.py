# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet
from exahype2.actionsets.solver_action_set import SolverActionSet


def number_of_particle_unknowns(particle_set):
    """Returns the size of the ``data`` attribute of the set's particle."""
    for attribute in particle_set.particle_model.attributes:
        if attribute.name == "data":
            return int(attribute.cardinality)
    raise ValueError(
        "The particle {} carries no data attribute.".format(
            particle_set.particle_model.name
        )
    )


class ParticleErrorMeasurement(SolverActionSet):
    """Compares every local particle of ``particle_set`` against the solver's particle
    analytical solution once the traversal ends.

    The particle container replaces objects while it updates the grid association, so the map
    holds copies of the particles.
    """

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  _timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
  _timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
  ::exahype2::collectLocalParticles(fineGridVertices{{PARTICLES_CONTAINER}}, _activeParticles);
"""

    _TEMPLATE_END_TRAVERSAL = """
  if ({{PREDICATE}}) {
    ::exahype2::accumulateParticlePositionErrors<{{NUMBER_OF_UNKNOWNS_PARTICLE}} + DIMENSIONS>(
      _activeParticles,
      _timeStamp,
      _timeStepSize,
      [](auto* const particle, const double timeStamp, const double timeStepSize, double* const solution) {
        {{FULL_QUALIFIED_SOLVER_NAME}}::analyticalSolution(particle, timeStamp, timeStepSize, solution);
      },
      repositories::{{SOLVER_INSTANCE}}.errorMeasurement
    );
  }

  _activeParticles.clear();
"""

    def __init__(self, solver, particle_set, descend_invocation_order):
        super().__init__(solver, descend_invocation_order)
        self.guard = (
            "(repositories::{{SOLVER_INSTANCE}}.isGridInitialisationSweep() or "
            "repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep())"
        )
        self._particle_set = particle_set

    def _entries(self):
        return {
            "PARTICLE": self._particle_set.particle_model.name,
            "PARTICLES_CONTAINER": self._particle_set.name,
            "NUMBER_OF_UNKNOWNS_PARTICLE": number_of_particle_unknowns(
                self._particle_set
            ),
            "PREDICATE": self.render(self.guard),
        }

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self.render(self._TEMPLATE_TOUCH_CELL_FIRST_TIME, **self._entries())
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            return self.render(self._TEMPLATE_END_TRAVERSAL, **self._entries())
        return ""

    def includes(self):
        return super().includes() + [
            "exahype2/ParticleErrorMeasurement.h",
            "globaldata/{}.h".format(self._particle_set.particle_model.name),
            "vertexdata/{}.h".format(self._particle_set.name),
        ]

    def attributes(self):
        return """
  std::map<std::pair<int, int>, globaldata::{}> _activeParticles{{}};
  double _timeStepSize{{0.0}};
  double _timeStamp{{0.0}};
""".format(self._particle_set.particle_model.name)
