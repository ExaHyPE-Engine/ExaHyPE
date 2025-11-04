# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractFVActionSet import AbstractFVActionSet
import peano4
import jinja2


class ParticleWisePostprocessSolution(AbstractFVActionSet):
    """
    Post-process solution by iterating through individual particles rather than control volumes.

    This action set collects all local (non-virtual) particles during the traversal phase and applies
    a user-defined postprocessing step to each of them at the end of the traversal.

    The motivation for executing postprocessing in the `endTraversal` phase—rather than cell-by-cell
    during the traversal—is compatibility with particle-particle (pp) interaction schemes. When pp
    interaction is enabled, particles may interact across cell boundaries, and their final state can
    depend on global or non-local operations that are not yet finalized during the per-cell loop.

    If we were to perform postprocessing on a per-cell basis during traversal (e.g., in touchCellLastTime),
    we would risk reading incomplete or inconsistent particle states, especially for particles whose
    behavior depends on neighbors or bin-based searches updated later in the sweep. This would break
    consistency for any dependent operations, such as computing error metrics, diagnostics, or applying
    boundary conditions that rely on finalized particle states.

    Instead, by deferring processing to the end of the sweep, we guarantee:
    - All particle positions and attributes have been updated (including those affected by collisions).
    - Interactions such as particle-particle forces or boundary reflections have been resolved.
    - We can loop over particles globally or locally without needing per-cell context.

    This action set avoids reliance on ExaHyPE2’s finite-volume postprocessing routines and is
    suitable for hybrid methods where tracers or particles carry physical state in a Lagrangian fashion.

    Only particles with ParallelState::Local are considered to avoid processing redundant or ghost particles.
    """

    def __init__(self, solver, particle_set, descend_invocation_order=1):
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
#include <list>
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
