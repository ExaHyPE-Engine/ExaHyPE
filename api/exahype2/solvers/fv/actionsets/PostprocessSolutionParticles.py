# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2
import peano4

from .AbstractFVActionSet import AbstractFVActionSet


class ParticleWisePostprocessSolution(AbstractFVActionSet):
    def __init__(self, solver, particle_set, descend_invocation_order=1):
        """
        The action set copies each local particle of particle_set while it runs
        through the cells and compares the copies against the solver's analytical
        solution once the traversal ends. The particle container replaces objects
        while it updates the grid association, so the map holds values, not
        addresses.
        """
        AbstractFVActionSet.__init__(self, solver)
        self.descend_invocation_order = descend_invocation_order
        self._compute_kernel = ""
        self.guard = "true"
        self.d = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "SOLVER_NAME": solver._name,
            "SOLVER_INSTANCE": solver.get_name_of_global_instance(),
            "NUMBER_OF_UNKNOWNS_PARTICLE": solver._particle_unknowns[0],
        }

    def add_postprocessing_kernel(self, operation_per_particle):
        self._compute_kernel = operation_per_particle

    def get_body_of_operation(self, operation_name):
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            return jinja2.Template(
                """
  _timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
  _timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

  for (int vertexNumber = 0; vertexNumber < peano4::VerticesPerCell; vertexNumber++) {
    for (auto* const particle : fineGridVertices{{PARTICLES_CONTAINER}}(vertexNumber)) {
      if (particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local) {
        const auto particleNumber = std::make_pair(particle->getNumber(0), particle->getNumber(1));
        _activeParticles.insert_or_assign(particleNumber, *particle);
      }
    }
  }
"""
            ).render(**self.d)

        if operation_name == peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
            return jinja2.Template(
                """
  for (auto& particleEntry : _activeParticles) {
    auto* const           particle     = &particleEntry.second;
    const auto            timeStamp    = _timeStamp;
    const auto            timeStepSize = _timeStepSize;
    std::array<double, 3> errors{};
    std::array<double, {{NUMBER_OF_UNKNOWNS_PARTICLE}} + DIMENSIONS> solution{};

    repositories::{{SOLVER_INSTANCE}}.analyticalSolution(particle, timeStamp, timeStepSize, solution.data());

    for (int coordinate = 0; coordinate < DIMENSIONS; coordinate++) {
      const auto error = std::abs(particle->getX(coordinate) - solution[coordinate]);
      errors[0] += error;
      errors[1] += error * error;
      errors[2] = std::max(errors[2], error);
    }

    repositories::{{SOLVER_INSTANCE}}.setError(errors.data());
  }

  _activeParticles.clear();
"""
            ).render(**self.d)

        return ""

    def get_includes(self):
        return (
            AbstractFVActionSet.get_includes(self)
            + f"""
#include "vertexdata/{self.d['PARTICLES_CONTAINER']}.h"
#include "globaldata/{self.d['PARTICLE']}.h"
#include "repositories/SolverRepository.h"
"""
        )

    def get_attributes(self):
        return f"""
std::map<std::pair<int, int>, globaldata::{self.d['PARTICLE']}> _activeParticles{{}};
double _timeStepSize{{0.0}};
double _timeStamp{{0.0}};
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
