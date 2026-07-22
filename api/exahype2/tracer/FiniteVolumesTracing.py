# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import peano4.toolbox.particles

import jinja2


class FiniteVolumesTracing(
    peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_Sets
):
    """!

    Particle tracing over the Finite Volumes solver.

    The action set works for cell-centred Finite Differences, too.

    ## Update plug-in points

    We only update cells which are marked with getHasUpdated(). Further to that,
    we also trace in the grid initialisation. If we did not trace there, the first
    dump prior to the first time step would always yield uninitialised tracer
    data. It would not even help to add tracing to the plot, as the tracing
    typically is cell-based, whereas most plotters plug into
    touchVertexFirstTime(), i.e. they would still dump the particle data prior to
    the tracing.

    The class supports different ways how the data is projected onto tracer
    attributes (both which data entries and how they are interpolated), as
    well as various time integrators. The integration happens patch by patch,
    i.e. you cannot access any neighbouring patches or intermediate data from
    the time stepping calculations.
    """

    def __init__(
        self,
        particle_set,
        solver,
        project_on_tracer_properties_kernel,
        projection_kernel_arguments="""
        marker,
        {{PATCH_SIZE}},
        {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
        fineGridCell{{CURRENT_UNKNOWN_IDENTIFIER}}.data(),
        *p
""",
    ):
        """!

        Set up the tracing.

        Setting up the tracing first and foremost means that we have to couple
        the tracer particle set with the solver, and then we have to specify which
        projection kernels and which time integrator we want to use. We have to
        provide the C++ kernels which shall be used to move the tracer and to
        write new data into the tracer.

        For the time stepping, Peano's particle toolbox provides some default
        integrators. If you use stationary tracers, toolbox::particles::StaticPosition
        (handed in as string) or None do the job.

        For the projection of solution data onto the tracer, the file
        exahype2/fv/Tracer.h provides a few pre-manufactured routines. Again, the
        projection kernel here requires a plain function call, and you have to
        ensure that this function call matches the data you want to dump.

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
         velocity/trajectory of a tracer already encodes all information.
         Some popular arguments for this routine are listed above.

        projection_kernel_arguments: String
         Argument list handed to the projection kernel. The default passes the
         cell marker, the patch size, the number of data entries per volume, the
         patch data and the particle.
        """

        self.tracerDict = {}
        # Do not overwrite the dictionary of the superclass, as we need it.
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
  for (auto* p: localParticles) {
    if (
      p->getMoveState()==globaldata::{{PARTICLE}}::MoveState::NotMovedYet and marker.isContained(p->getX())
    ) {
      p->setMoveState(globaldata::{{PARTICLE}}::MoveState::Moved);

      {{PROJECTION_KERNEL}}(
        {{PROJECTION_KERNEL_ARGUMENTS}}
      );
    }
  }
}
"""

        touch_vertex_first_time_compute_kernel = """
  for (auto* p: assignedParticles) {
    p->setMoveState(globaldata::{{PARTICLE}}::MoveState::NotMovedYet);
  }
"""

        super(FiniteVolumesTracing, self).__init__(
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
        result = jinja2.Template(
            """
#include "tarch/multicore/Lock.h"
#include "peano4/Loop.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
#include "repositories/SolverRepository.h"
#include "exahype2/fv/Tracer.h"
"""
        )
        return (
            super(FiniteVolumesTracing, self).get_includes()
            + "\n"
            + result.render(**self.tracerDict)
        )

    def get_attributes(self):
        return (
            super(FiniteVolumesTracing, self).get_attributes()
            + """
double _timeStepSize;
"""
        )
