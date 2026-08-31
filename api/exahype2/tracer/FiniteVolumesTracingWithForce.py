# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2
import peano4.particles


class FiniteVolumesTracingWithForce(
    peano4.particles.UpdateParticle_MultiLevelInteraction_Sets
):
    """
    Moves finite-volume tracer particles after their solver cell has updated.

    ``fields_to_interpolate`` lists the destination particle attributes for
    consecutive solver values. For example, ``"0,3,4"`` maps the first three
    solver values to particle attributes 0, 3, and 4.

    Integration schemes use 0 for static particles, 1 for explicit Euler, 2
    for semi-implicit Euler, 3 for velocity Verlet, 4 for midpoint, 5 for RK3,
    and 6 for RK4. Interpolation uses 0 for none, 1 for piecewise constant, and
    2 for piecewise linear interpolation.

    Force methods form a bit mask: 1 selects central gravity, 2 buoyancy, 4
    Stokes drag, and 8 constant gravity. Values can be combined, so 14 enables
    buoyancy, drag, and constant gravity, while 15 enables every force.
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
        fluid_density="1",  # Default fluid density in kg/m³.
    ):
        self.tracerDict = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "SOLVER_NAME": solver._name,
            "SOLVER_INSTANCE": solver.get_name_of_global_instance(),
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
            "PERIODIC_BC": "{"
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
            "FLUID_DENSITY": fluid_density,
        }
        self.tracerDict["UPDATE_KERNEL_ARGUMENTS"] = (
            jinja2.Template(update_kernel_arguments).render(**self.tracerDict).strip()
        )
        cell_compute_kernel = """
  if (
    fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated()
    and
    (
      repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
      or
      repositories::{{SOLVER_INSTANCE}}.getSolverState() == {{SOLVER_NAME}}::SolverState::GridInitialisation
    )
  ) {
    const bool domainWrapsParticles = toolbox::particles::domainHasPeriodicAxes();
    for (auto* const particle : localParticles) {
      // The wall vertex of a periodic axis holds particles from both walls, so
      // the cell reads a particle through its periodic image.
      const auto image = domainWrapsParticles
        ? toolbox::particles::periodicImageTowardsCell(particle->getX(), marker)
        : particle->getX();
      if (
        particle->getMoveState() == globaldata::{{PARTICLE}}::MoveState::NotMovedYet
        and
        marker.isContained(image)
      ) {
        if (domainWrapsParticles) {
          particle->setX(image);
        }
        {{UPDATE_PARTICLE_KERNEL}}<{{FIELDS_TO_INTERPOLATE}}>(
          {{UPDATE_KERNEL_ARGUMENTS | indent(10)}}
        );
        particle->setMoveState(globaldata::{{PARTICLE}}::MoveState::Moved);
      }
    }
  }
"""

        touch_vertex_first_time_compute_kernel = """
  for (auto* const particle : assignedParticles) {
    particle->setMoveState(globaldata::{{PARTICLE}}::MoveState::NotMovedYet);
  }
"""

        prepare_traversal_kernel = """
  ::exahype2::fv::particles::utils::setGlobalParameters(
    {{DOMAIN_OFFSET}},
    {{DOMAIN_SIZE}},
    {{PERIODIC_BC}}
  );

  if ({{INTERPOLATION_SCHEME}} != 0) {
    ::exahype2::fv::particles::interpolation::setGlobalInterpolationParameters(
      {{PATCH_SIZE}},
      {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{INTERPOLATION_SCHEME}}
    );
  }

  if ({{FORCE_METHOD}} != 0) {
    ::exahype2::fv::particles::forces::setAllParameters(
      {{FORCE_METHOD}},
      {{MU}},
      {{GRAVITY_MAGNITUDE}},
      {{DYNAMIC_VISCOSITY}},
      {{FLUID_DENSITY}}
    );
  }
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

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        includes = jinja2.Template(
            """
#include "exahype2/fv/particles/ParticleForces.h"
#include "exahype2/fv/particles/ParticleHelper.h"
#include "exahype2/fv/particles/ParticleInterpolation.h"
#include "exahype2/fv/particles/ParticleUpdate.h"
#include "repositories/SolverRepository.h"
#include "toolbox/particles/particles.h"
"""
        )
        return super().get_includes() + "\n" + includes.render(**self.tracerDict)
