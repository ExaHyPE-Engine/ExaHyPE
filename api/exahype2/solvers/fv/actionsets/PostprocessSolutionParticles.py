# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractFVActionSet import AbstractFVActionSet
import peano4
import jinja2


class ParticleWisePostprocessSolution(AbstractFVActionSet):
    """
    Postprocess the solution particle by particle.

    The action set collects the local particles while it runs through the cells
    and evaluates them once the traversal has finished. Collecting first and
    evaluating afterwards keeps the action set compatible with particle-particle
    interaction schemes: a particle may interact across cell boundaries, so its
    state is not settled while we still work cell by cell. An evaluation in
    touchCellLastTime() would read particle states that a later cell still
    changes, and any derived quantity such as an error metric would become
    inconsistent.

    Once the sweep has finished, all particle positions and attributes are
    updated, the particle-particle interactions and boundary reflections have
    been resolved, and we can walk over the particles without any per-cell
    context. For every particle we compare its position with the analytical
    solution and reduce the per-coordinate deviations into their sum, their sum
    of squares and their maximum. These three figures are handed to the solver
    through setError().

    Only particles with ParallelState::Local contribute, and a particle that is
    adjacent to several vertices is recognised by its number pair, so each
    particle enters the evaluation exactly once.

    The action set is suitable for hybrid methods where tracers carry physical
    state in a Lagrangian fashion.
    """

    def __init__(self, solver, particle_set, descend_invocation_order=1):
        """
        Couple the tracer particles to the solver.

        ## Arguments

        solver: FV
          The Finite Volumes solver whose analytical solution the particles are
          compared against.

        particle_set: ParticleSet
          The tracer particles which are evaluated at the end of the traversal.

        descend_invocation_order: Int
          Position of this action set within the solver's action set sequence.
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

  for (int i = 0; i < TWO_POWER_D; i++)
  {
    for (auto* p : fineGridVertices{{PARTICLES_CONTAINER}}(i))
    {
      if (p->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local)
      {
        bool isDuplicate = false;
        for (auto* existingP : _activeParticles)
        {
          if (p->getNumber(0) == existingP->getNumber(0) &&
              p->getNumber(1) == existingP->getNumber(1))
          {
            isDuplicate = true;
            break;
          }
        }

        if (!isDuplicate)
        {
          _activeParticles.push_back(p);
        }
      }
    }
  }
"""
            ).render(**self.d)

        if operation_name == peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
            return jinja2.Template(
                """
  for (auto* p : _activeParticles)
  {
    const double timeStamp    = _timeStamp;
    const double timeStepSize = _timeStepSize;
    double errors[3] = {0.0, 0.0, 0.0};
    double solution[{{NUMBER_OF_UNKNOWNS_PARTICLE}} + DIMENSIONS] = {0.0};

    repositories::{{SOLVER_INSTANCE}}.analyticalSolution(
      p,
      timeStamp,
      timeStepSize,
      solution
    );

    for (int i = 0; i < DIMENSIONS; i++)
    {
      double error = std::abs(p->getX(i) - solution[i]);
      errors[0] += error;
      errors[1] += error * error;
      errors[2]  = std::max(errors[2], error);
    }

    repositories::{{SOLVER_INSTANCE}}.setError(errors);
  }

  _activeParticles.clear();
"""
            ).render(self.d)

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
std::list<globaldata::{self.d['PARTICLE']}*> _activeParticles;
double _timeStepSize = 0.0;
double _timeStamp    = 0.0;
"""

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
