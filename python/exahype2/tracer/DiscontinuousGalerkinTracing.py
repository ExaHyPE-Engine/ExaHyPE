# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4.toolbox.particles

import jinja2


class DiscontinuousGalerkinTracing(
    peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_Sets
):
    """!

    Particle tracing over the DG solver.

    Please consult the class FiniteVolumesTracing for details on the semantics
    of this type. It is an exact analogon for DG.

    ## Update plug-in points

    We only update cells which are marked with getHasUpdated(). For enclave solvers,
    we assume that this flag is only set after a cell has updated. For Runge-Kutta
    type solvers, I assume that this flag is set by the computation of the final
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
        fineGridCell{{SOLVER_NAME}}Q.value,
        *p
""",
    ):
        """

        @see FiniteVolumesTracing.__init()__.

        """

        self.tracerDict = {}
        # Don't overwrite hte dictionary of hte superclass. We'll need it
        self.tracerDict["PARTICLE"] = particle_set.particle_model.name
        self.tracerDict["PARTICLES_CONTAINER"] = particle_set.name
        self.tracerDict["SOLVER_NAME"] = solver._name
        self.tracerDict["SOLVER_INSTANCE"] = solver.get_name_of_global_instance()
        self.tracerDict["ORDER"] = solver._basis.order
        self.tracerDict["NUMBER_OF_UNKNOWNS"] = solver._unknowns
        self.tracerDict["NUMBER_OF_AUXILIARY_VARIABLES"] = solver._auxiliary_variables
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
      p->getMoveState()==globaldata::{{PARTICLE}}::MoveState::Moved;

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
 
        super(DiscontinuousGalerkinTracing,self).__init__(
            particle_set,
            jinja2.Template(cell_compute_kernel).render(**self.tracerDict),
            jinja2.Template(touch_vertex_first_time_compute_kernel).render(
                **self.tracerDict
            ),
        )

    # def get_body_of_operation(self,operation_name):
    #  result = peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles.get_body_of_operation(self,operation_name)
    #  if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
    #    result = self.__Template_BeginIteration.render(**self.tracerDict)
    #  return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        result = jinja2.Template(
            """
#include "tarch/multicore/Lock.h"
#include "peano4/utils/Loop.h"
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
double _timeStepSize;    
"""
        )
