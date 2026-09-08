# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import peano4.particles

import jinja2


class FiniteVolumesTracing(peano4.particles.UpdateParticle_MultiLevelInteraction_Sets):
    """!

    Particle tracing over the Finite Volumes solver.

    The action set works for cell-centred Finite Differences, too. It updates
    cells which are marked with getHasUpdated() and traces in the grid
    initialisation. The projection runs patch by patch, so it reads no
    neighbouring patch.
    """

    def __init__(
        self,
        particle_set,
        solver,
        project_on_tracer_properties_kernel,
        projection_kernel_arguments="""
        marker,
        {{PATCH_SIZE}},
        {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},
        fineGridCell{{CURRENT_UNKNOWN_IDENTIFIER}}.data(),
        *particle
""",
    ):
        """!

        Set up the tracing.

        The projection kernel is a plain function call, so its argument list has
        to match the data the tracer stores.

        ## Arguments

        particle_set: ParticleSet
         The tracer particles which are coupled to the solver.

        solver: FV
         The Finite Volumes solver whose patch data is projected onto the
         tracers.

        project_on_tracer_properties_kernel: String
         Name of the routine that projects the Finite Volumes representation
         onto the tracers' properties. Hand in None if you do not want any field
         properties to be projected on the tracer. This makes sense if the
         velocity or trajectory of a tracer already encodes all information.

        projection_kernel_arguments: String
         Argument list handed to the projection kernel. The default passes the
         cell marker, the patch size, the number of data entries per volume, the
         patch data and the particle.
        """

        self.tracerDict = {}
        # Do not overwrite the dictionary of the superclass.
        self.tracerDict["PARTICLE"] = particle_set.particle_model.name
        self.tracerDict["PARTICLES_CONTAINER"] = particle_set.name
        self.tracerDict["SOLVER_NAME"] = solver._name
        self.tracerDict["SOLVER_INSTANCE"] = solver.get_name_of_global_instance()
        self.tracerDict["CURRENT_UNKNOWN_IDENTIFIER"] = (
            solver.current_unknown_identifier()
        )
        self.tracerDict["PATCH_SIZE"] = solver._patch_size
        self.tracerDict["NUMBER_OF_UNKNOWNS"] = solver.unknowns
        self.tracerDict["NUMBER_OF_AUXILIARY_VARIABLES"] = solver.auxiliary_variables
        self.tracerDict["PROJECTION_KERNEL"] = project_on_tracer_properties_kernel
        self.tracerDict["PROJECTION_KERNEL_ARGUMENTS"] = jinja2.Template(
            projection_kernel_arguments
        ).render(**self.tracerDict)

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
  const bool domainWrapsParticles = peano4::particles::domainHasPeriodicAxes();
  for (auto* const particle : localParticles) {
    // The wall vertex of a periodic axis holds particles from both walls, so
    // the cell reads a particle through its periodic image.
    const auto image = domainWrapsParticles
      ? peano4::particles::periodicImageTowardsCell(particle->getX(), marker)
      : particle->getX();
    if (
      particle->getMoveState() == globaldata::{{PARTICLE}}::MoveState::NotMovedYet
      and marker.isContained(image)
    ) {
      if (domainWrapsParticles) {
        particle->setX(image);
      }
      particle->setMoveState(globaldata::{{PARTICLE}}::MoveState::Moved);

{% if PROJECTION_KERNEL %}
      {{PROJECTION_KERNEL}}(
        {{PROJECTION_KERNEL_ARGUMENTS}}
      );
{% endif %}
    }
  }
}
"""

        touch_vertex_first_time_compute_kernel = """
  for (auto* const particle : assignedParticles) {
    particle->setMoveState(globaldata::{{PARTICLE}}::MoveState::NotMovedYet);
  }
"""

        super().__init__(
            particle_set,
            jinja2.Template(cell_compute_kernel).render(**self.tracerDict),
            jinja2.Template(touch_vertex_first_time_compute_kernel).render(
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
#include "peano4/Loop.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "repositories/SolverRepository.h"
#include "exahype2/fv/Tracer.h"
#include "peano4/particles/particles.h"
"""
        )
        return super().get_includes() + "\n" + includes.render(**self.tracerDict)
