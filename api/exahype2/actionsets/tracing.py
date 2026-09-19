# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

import jinja2

import exahype2.particles


class Tracing(exahype2.particles.UpdateParticleMultiLevelInteractionSets):
    """Projects the solution of an updated cell onto the tracer particles the cell holds.

    ``kernel`` names the projection function and ``arguments`` its argument list, a Jinja2
    template over the solver dictionary; ``None`` projects nothing and only moves the particle
    through its periodic image.
    """

    _TEMPLATE_CELL_KERNEL = """
  if (
    fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated()
    and (
      repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
      or repositories::{{SOLVER_INSTANCE}}.isGridInitialisationSweep()
    )
  ) {
    ::exahype2::moveParticlesThroughCell(
      marker,
      localParticles,
      DomainSize,
      PeriodicBoundaries,
      [&]([[maybe_unused]] auto* const particle) {
{%- if PROJECTION_KERNEL %}
        {{PROJECTION_KERNEL}}(
          {{PROJECTION_KERNEL_ARGUMENTS | indent(10)}}
        );
{%- endif %}
      }
    );
  }
"""

    _TEMPLATE_VERTEX_KERNEL = """
  ::exahype2::markParticlesAsNotMoved(assignedParticles);
"""

    def __init__(self, solver, particle_set, kernel=None, arguments=None):
        self._entries = dict(solver.dictionary)
        self._entries["PARTICLE"] = particle_set.particle_model.name
        self._entries["PARTICLES_CONTAINER"] = particle_set.name
        self._entries["PROJECTION_KERNEL"] = kernel
        self._entries["PROJECTION_KERNEL_ARGUMENTS"] = textwrap.dedent(
            self._render(
                solver.layout.tracer_arguments if arguments is None else arguments
            )
        ).strip()
        self._entries["TRACER_INCLUDE"] = solver.layout.tracer_include
        super().__init__(
            particle_set,
            self._render(self._TEMPLATE_CELL_KERNEL),
            self._render(self._TEMPLATE_VERTEX_KERNEL),
        )

    def _render(self, template):
        return jinja2.Template(template, undefined=jinja2.DebugUndefined).render(
            **self._entries
        )

    def includes(self):
        return super().includes() + [
            "Constants.h",
            "exahype2/Tracing.h",
            "peano4/Loop.h",
            "repositories/SolverRepository.h",
            self._entries["TRACER_INCLUDE"],
        ]
