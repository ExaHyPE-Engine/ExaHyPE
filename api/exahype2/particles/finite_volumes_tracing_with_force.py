# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from .update_particle_multi_level_interaction_sets import (
    UpdateParticleMultiLevelInteractionSets,
)


class FiniteVolumesTracingWithForce(UpdateParticleMultiLevelInteractionSets):
    """
    Moves finite-volume tracer particles after their solver cell has updated.
    """

    def __init__(
        self,
        particle_set,
        solver,
        project,
        update_kernel_arguments="""
*particle,
marker,
fineGridCell{{CURRENT_UNKNOWN_IDENTIFIER}}.data(),
fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize(),
{{INTEGRATION_SCHEME}}
""",
        interpolation_scheme=0,
        integration_scheme=0,
        force_method=14,
        fields_to_interpolate="0,3,4",
        mu="10.0",  # Central-gravity parameter G M.
        gravity_magnitude="1",  # Constant gravity magnitude in m/s².
        dynamic_viscosity="1",  # Fluid dynamic viscosity in Pa s.
    ):
        self.tracerDict = {
            "SOLVER_NAME": solver._name,
            "SOLVER_INSTANCE": solver.instance_name,
            "CURRENT_UNKNOWN_IDENTIFIER": solver.current_unknown_identifier(),
            "PATCH_SIZE": solver._patch_size,
            "NUMBER_OF_UNKNOWNS": solver.unknowns,
            "NUMBER_OF_AUXILIARY_VARIABLES": solver.auxiliary_variables,
            "INTERPOLATION_SCHEME": interpolation_scheme,
            "INTEGRATION_SCHEME": integration_scheme,
            "FORCE_METHOD": force_method,
            "UPDATE_PARTICLE_KERNEL": "::exahype2::fv::particles::updateParticleState",
            "DOMAIN_SIZE": "{"
            + ", ".join(map(str, project._domain_size[: project._dimensions]))
            + "}",
            "DOMAIN_OFFSET": "{"
            + ", ".join(map(str, project._domain_offset[: project._dimensions]))
            + "}",
            "PERIODIC_BOUNDARIES": "{"
            + ", ".join(
                [
                    "1" if is_periodic else "0"
                    for is_periodic in project._periodic_bc[: project._dimensions]
                ]
            )
            + "}",
            "FIELDS_TO_INTERPOLATE": ", ".join(
                attribute_index.strip()
                for attribute_index in fields_to_interpolate.split(",")
            ),
            "MU": mu,
            "GRAVITY_MAGNITUDE": gravity_magnitude,
            "DYNAMIC_VISCOSITY": dynamic_viscosity,
        }
        self.tracerDict["UPDATE_KERNEL_ARGUMENTS"] = (
            jinja2.Template(update_kernel_arguments).render(**self.tracerDict).strip()
        )
        cell_compute_kernel = """
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
      [&](auto* const particle) {
        {{UPDATE_PARTICLE_KERNEL}}<{{FIELDS_TO_INTERPOLATE}}>(
          {{UPDATE_KERNEL_ARGUMENTS | indent(10)}}
        );
      }
    );
  }
"""

        touch_vertex_first_time_compute_kernel = """
  ::exahype2::markParticlesAsNotMoved(assignedParticles);
"""

        prepare_traversal_kernel = """
  ::exahype2::fv::particles::utils::setGlobalParameters(
    {{DOMAIN_OFFSET}},
    {{DOMAIN_SIZE}},
    {{PERIODIC_BOUNDARIES}}
  );
{%- if INTERPOLATION_SCHEME | int != 0 %}
  ::exahype2::fv::particles::interpolation::setGlobalInterpolationParameters(
    {{PATCH_SIZE}},
    {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},
    {{INTERPOLATION_SCHEME}}
  );
{%- endif %}
{%- if FORCE_METHOD | int != 0 %}
  ::exahype2::fv::particles::forces::setAllParameters(
    {{FORCE_METHOD}},
    {{MU}},
    {{GRAVITY_MAGNITUDE}},
    {{DYNAMIC_VISCOSITY}}
  );
{%- endif %}
"""

        super().__init__(
            particle_set,
            jinja2.Template(cell_compute_kernel).render(**self.tracerDict),
            jinja2.Template(touch_vertex_first_time_compute_kernel).render(
                **self.tracerDict
            ),
            prepare_traversal_kernel=jinja2.Template(prepare_traversal_kernel).render(
                **self.tracerDict
            ),
        )

    def includes(self):
        return super().includes() + [
            "Constants.h",
            "exahype2/Tracing.h",
            "exahype2/fv/particles/ParticleForces.h",
            "exahype2/fv/particles/ParticleHelper.h",
            "exahype2/fv/particles/ParticleInterpolation.h",
            "exahype2/fv/particles/ParticleUpdate.h",
            "repositories/SolverRepository.h",
        ]
