# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import peano4.toolbox.particles

import jinja2


class DiscontinuousGalerkinTracing(
    peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_Sets
):
    """!

    Particle tracing over the DG solver.

    The class mirrors FiniteVolumesTracing: it couples a tracer particle set to
    the solver and projects the cell solution onto the tracer attributes. The
    default argument list passes the polynomial order and the solver's quadrature
    points, so the projection kernel can evaluate the DG polynomial right at the
    particle position.

    ## Update plug-in points

    We only update cells which are marked with getHasUpdated(). For Runge-Kutta
    type solvers, we assume that this flag is set by the computation of the final
    linear combination of estimates.
    """

    def __init__(
        self,
        particle_set,
        solver,
        project_on_tracer_properties_kernel,
        projection_kernel_arguments="""
        marker,
        {{ORDER}},
        repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d,
        {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
        fineGridCell{{CURRENT_UNKNOWN_IDENTIFIER}}.data(),
        *p
""",
    ):
        """!

        Set up the tracing.

        Setting up the tracing means that we couple the tracer particle set with
        the solver and state which projection kernel writes solver data into the
        tracer. The file exahype2/dg/Tracer.h provides a few pre-manufactured
        routines. The kernel is a plain function call, so you have to ensure that
        its argument list matches the data you want to dump.

        ## Arguments

        particle_set: ParticleSet
         The tracer particles which are coupled to the solver.

        solver: ADERDG
         The DG solver whose cell data is projected onto the tracers.

        project_on_tracer_properties_kernel: String
         Name of the routine that projects the DG representation onto the
         tracers' properties. Hand in None if you do not want any field
         properties to be projected on the tracer. This makes sense if the
         trajectory of a tracer already encodes all information.

        projection_kernel_arguments: String
         Argument list handed to the projection kernel. The default passes the
         cell marker, the polynomial order, the solver's quadrature points, the
         number of data entries per degree of freedom, the cell data and the
         particle.
        """

        self.tracerDict = {}
        # Do not overwrite the dictionary of the superclass, as we need it.
        self.tracerDict["PARTICLE"] = particle_set.particle_model.name
        self.tracerDict["PARTICLES_CONTAINER"] = particle_set.name
        self.tracerDict["SOLVER_NAME"] = solver._name
        self.tracerDict["SOLVER_INSTANCE"] = solver.get_name_of_global_instance()
        self.tracerDict["CURRENT_UNKNOWN_IDENTIFIER"] = solver.current_unknown_identifier()
        self.tracerDict["ORDER"] = solver._basis.order
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
    repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::GridInitialisation
  )
) {
  for (auto* p: localParticles) {
    if (
      p->getMoveState()==globaldata::{{PARTICLE}}::MoveState::NotMovedYet and marker.isContained(p->getX())
    ) {
      {{PROJECTION_KERNEL}}(
        {{PROJECTION_KERNEL_ARGUMENTS}}
      );

      p->setMoveState( globaldata::{{PARTICLE}}::MoveState::Moved );
    }
  }
}
"""

        touch_vertex_first_time_compute_kernel = """
  for (auto* p: assignedParticles) {
    p->setMoveState(globaldata::{{PARTICLE}}::MoveState::NotMovedYet);
  }
"""

        super(DiscontinuousGalerkinTracing,self).__init__(
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
#include "exahype2/dg/Tracer.h"
"""
        )
        return (super(DiscontinuousGalerkinTracing,self).get_includes()
            + "\n"
            + result.render(**self.tracerDict)
        )

    def get_attributes(self):
        return (
            super(DiscontinuousGalerkinTracing,self).get_attributes()
            + """
"""
        )
