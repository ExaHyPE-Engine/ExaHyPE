# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.toolbox.particles

import jinja2


# TODO: Combine with FiniteVolumesTracing
class FiniteVolumesTracingWithForce(
    peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_Sets
):
    """
    Tracing particles in a finite volume solver with flexible configuration.

    This action set updates tracer particles by:
      - Reading field values from the solver
      - Moving particles using selected integration schemes
      - Optionally applying external forces such as gravity, drag, and buoyancy

    Supported features:
      - Interpolation of cell-based field values to particles
      - Euler, RK, and Verlet integration schemes
      - Configurable force models (gravity, buoyancy, drag)

    Fields to interpolate from solver Q must be listed via 'fields_to_interpolate'.
    For example: "0,3,4" maps Q[0] to particle[0], Q[1] to particle[3] and Q[2] to particle[4].

    Mapping scheme:
      Q[0] → p[0]
      Q[1] → p[3]
      Q[2] → p[4]
      Q[3] → Not interpolated
      Q[4] → Not interpolated and so on.

    Available integration schemes (selectable via configuration):
      "Static"    → 0 : No integration (static particles)
      "Euler"     → 1 : Explicit Euler method (simple but less accurate)
      "SemiEuler" → 2 : Semi-implicit Euler method (better stability)
      "Verlet"    → 3 : Velocity Verlet algorithm (good for orbital mechanics)
      "Midpoint"  → 4 : Midpoint method (2nd order Runge-Kutta)
      "RK3"       → 5 : 3rd order Runge-Kutta method (high accuracy)
      "RK4"       → 6 : 4th order Runge-Kutta method (high accuracy)

    Available interpolation methods:
      "None"     → 0 : No interpolation
      "Constant" → 1 : Piecewise constant interpolation (nearest neighbor)
      "Linear"   → 2 : Piecewise linear interpolation (more accurate)

    Available force models (bitmask, can be combined):
        0  : No forces
        1  : Central gravitational force (orbital mechanics)
        2  : Buoyancy force
        4  : Stokes drag force
        8  : Constant gravity force
        14 : Combined buoyancy + drag + gravity (settling particles)
        15 : All forces enabled
    """

    def __init__(
        self,
        particle_set,
        solver,
        project,
        update_kernel_arguments="""
*p,
marker,
fineGridCell{{SOLVER_NAME}}Q.data()
""",
        interpolation_scheme=0,
        integration_scheme=0,
        force_method=14,
        fields_to_interpolate="0,3,4",
        mu="10.0",  # Gravitational constant for central force
        gravity_magnitude="1",  # Constant gravity magnitude (m/s^2)
        dynamic_viscosity="1",  # Fluid dynamic viscosity (Pa·s)
        fluid_density="1",  # Default fluid density (kg/m^3) # TODO: Find a better name
    ):
        self.tracerDict = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
            "SOLVER_NAME": solver._name,
            "SOLVER_INSTANCE": solver.get_name_of_global_instance(),
            "PATCH_SIZE": solver._patch_size,
            "NUMBER_OF_UNKNOWNS": solver.unknowns,
            "NUMBER_OF_AUXILIARY_VARIABLES": solver.auxiliary_variables,
            "INTERPOLATION_SCHEME": interpolation_scheme,
            "INTEGRATION_SCHEME": integration_scheme,
            "FORCE_METHOD": force_method,
            "UPDATE_PARTICLE_KERNEL": "::exahype2::fv::particles::updateParticleState",
            "DOMAIN_SIZE": "{"
            + ",".join(map(str, project._domain_size[: project._dimensions]))
            + "}",
            "DOMAIN_OFFSET": "{"
            + ",".join(map(str, project._domain_offset[: project._dimensions]))
            + "}",
            "PERIODIC_BC": "{"
            + ",".join(
                ["1" if b else "0" for b in project._periodic_BC[: project._dimensions]]
            )
            + "}",
            "FIELDS_TO_INTERPOLATE": fields_to_interpolate,
            "MU": mu,
            "GRAVITY_MAGNITUDE": gravity_magnitude,
            "DYNAMIC_VISCOSITY": dynamic_viscosity,
            "FLUID_DENSITY": fluid_density,
        }
        self.tracerDict["UPDATE_KERNEL_ARGUMENTS"] = jinja2.Template(
            update_kernel_arguments
        ).render(**self.tracerDict)
        solver._finish_time_step_implementation += """
    vertexdata::{}::reduceNumberOfParticles();
    """.format(
            self.tracerDict["PARTICLES_CONTAINER"]
        )

        cell_compute_kernel = """
_timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();

::exahype2::fv::particles::integration::setGlobalIntegrationParameters(
  _timeStepSize,
  {{INTEGRATION_SCHEME}}
);

if (
  fineGridCell{{SOLVER_NAME}}CellLabel.getHasUpdated()
  and
  (
    repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
    or
    repositories::{{SOLVER_INSTANCE}}.getSolverState() == {{SOLVER_NAME}}::SolverState::GridInitialisation
  )
) {
  for (auto* p: localParticles) {
    if (
      p->getMoveState() == globaldata::{{PARTICLE}}::MoveState::NotMovedYet and marker.isContained(p->getX())
    ) {
      {{UPDATE_PARTICLE_KERNEL}}<{{FIELDS_TO_INTERPOLATE}}>(
        {{UPDATE_KERNEL_ARGUMENTS}}
      );
      p->setMoveState(globaldata::{{PARTICLE}}::MoveState::Moved);
    }
  }
}
"""

        touch_vertex_first_time_compute_kernel = """
  for (auto* p: assignedParticles) {
    p->setMoveState(globaldata::{{PARTICLE}}::MoveState::NotMovedYet);
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
  // Set all force parameters
  ::exahype2::fv::particles::forces::setAllParameters(
    {{FORCE_METHOD}},      // Force method bitmask
    {{MU}},                // Gravitational constant for central force
    {{GRAVITY_MAGNITUDE}}, // Constant gravity magnitude
    {{DYNAMIC_VISCOSITY}}, // Fluid dynamic viscosity
    {{FLUID_DENSITY}}      // Default fluid density
  );
}
"""

        super(FiniteVolumesTracingWithForce, self).__init__(
            particle_set,
            jinja2.Template(cell_compute_kernel).render(**self.tracerDict),
            jinja2.Template(touch_vertex_first_time_compute_kernel).render(
                **self.tracerDict
            ),
            prepare_traversal_kernel=jinja2.Template(prepare_traversal_kernel).render(
                **self.tracerDict
            ),
        )
        self.extend_solver_print_implementation(solver=solver)

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_static_initialisations(self, full_qualified_classname):
        return f"""
double {full_qualified_classname}::_timeStepSize;
"""

    def get_includes(self):
        result = jinja2.Template(
            """
#include "peano4/utils/Loop.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "repositories/SolverRepository.h"
#include "exahype2/fv/particles/ParticleUpdate.h"
#include "toolbox/particles/ParticleFactory.h"
#include "toolbox/particles/ParticleSet.h"
#include "exahype2/fv/particles/ParticleHelper.h"
#include "exahype2/fv/particles/ParticleInterpolation.h"
#include "exahype2/fv/particles/ParticleForces.h"
"""
        )
        return (
            super(FiniteVolumesTracingWithForce, self).get_includes()
            + "\n"
            + result.render(**self.tracerDict)
        )

    def get_attributes(self):
        return (
            super(FiniteVolumesTracingWithForce, self).get_attributes()
            + """
static double _timeStepSize;
"""
        )

    def extend_solver_print_implementation(self, solver):
        """
        Extend the solver's print time step implementation to include particle statistics
        """
        # Add necessary includes for particle statistics
        solver.add_user_solver_includes(
            """ #include "vertexdata/{}.h"
    """.format(
                self.tracerDict["PARTICLES_CONTAINER"]
            )
        )

        # Particle statistics code
        particle_statistics_code = jinja2.Template("""
    // TODO: Only print if number of particles > 0
    logInfo("printTimeStep()", vertexdata::{{PARTICLES_CONTAINER}}::printTotalNumberOfParticles());
"""
        ).render(**self.tracerDict)

        # Get current implementation
        current_implementation = getattr(solver, "_print_time_step_implementation", "")

        # Simply append to the end - the template will handle the braces
        solver._print_time_step_implementation = (
            current_implementation + particle_statistics_code
        )
