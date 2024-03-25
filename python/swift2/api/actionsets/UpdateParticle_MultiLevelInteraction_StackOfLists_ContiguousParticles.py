# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.toolbox.particles


from .TaskGraphKernelWrappers import (
    construct_touch_vertex_first_time_call,
    construct_touch_cell_first_time_call,
    construct_touch_vertex_last_time_call,
)

class UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles(
    peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles
):
    """!
    
    Plain wrapper around peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles that introduces tasks
    
    The interesting stuff is actually happing in 
    TaskGarphKernelWrappers.py however. 
    
    
    ## Multiscale task dependencies
    
    An interesting feature covered here that is 
    worth mentioning is the attribute _childCellDependencies. When we hit a 
    cell, we will create cell tasks. These cell tasks will feed into their 
    parent. However, we have no access to children cells in Peano's signatures
    anymore when we backtrack. We also cannot spawn these dependencies 
    straightaway as we would create anti-dependencies.
    
    We therefore implement things differently: While we do not spawn tasks
    logically for touchCellLastTime(), we technically spawn them (they are 
    empty) and each touchVertexLastTime() feeds into these helper tasks. 
    Further to that, we maintain a stack _childCellDependencies which is added
    the numbers of these helper tasks. Eventually, coarse grid 
    touchVertexLastTime() events can link against these helper tasks.
    
    """
    def __init__(
        self,
        particle_set,
        particle_particle_interaction_kernel,
        touch_vertex_first_time_compute_particle_update_kernel,
        touch_vertex_last_time_compute_particle_update_kernel,
        prepare_traversal_kernel,
        unprepare_traversal_kernel,
        additional_includes,
        active_particle_set,
        current_step_alters_particle_position,
        wait_for_all_tasks_to_finish_after_traversal,
    ):
        """!

        Construct


        @param wait_for_all_tasks_to_finish_after_traversal: Boolean
          If this one is set, we insert a wait into the unprepare traversal 
          event. Each rank then waits for all tasks to be completed before
          it continues. We have a strong synchronisation point.
          

        """
        if wait_for_all_tasks_to_finish_after_traversal:
            unprepare_traversal_kernel = (
                unprepare_traversal_kernel
                + """
              tarch::multicore::waitForAllTasks();
              """
            )

        super(
            UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles, self
        ).__init__(
            particle_set,
            particle_particle_interaction_kernel=construct_touch_cell_first_time_call(
                particle_set,particle_particle_interaction_kernel,True
            ),
            touch_vertex_first_time_compute_particle_update_kernel=construct_touch_vertex_first_time_call(
                particle_set,touch_vertex_first_time_compute_particle_update_kernel,True
            ),
            touch_vertex_last_time_compute_particle_update_kernel=construct_touch_vertex_last_time_call(
                particle_set,touch_vertex_last_time_compute_particle_update_kernel,True,current_step_alters_particle_position
            ),
            prepare_traversal_kernel=prepare_traversal_kernel,
            unprepare_traversal_kernel=unprepare_traversal_kernel,
            additional_includes=additional_includes
            + """
#include "tarch/multicore/Tasks.h"
#include "swift2/TaskNumber.h"                
                 """,
            active_particle_set=active_particle_set,
        )

    def get_attributes(self):
        return (
            super(UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles, self).get_attributes()
                + """
                ::swift2::PendingDependencies  _pendingDependencies;
            """
            )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")


    def get_body_of_operation(self, operation_name):
        if operation_name == peano4.solversteps.ActionSet.OPERATION_BEGIN_TRAVERSAL:
            return "_pendingDependencies.clear();"
        else:
            return super(
                UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles, self
            ).get_body_of_operation(operation_name)
            