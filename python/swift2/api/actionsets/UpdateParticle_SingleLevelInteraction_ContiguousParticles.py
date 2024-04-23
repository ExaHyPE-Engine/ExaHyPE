# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles


from .TaskGraphKernelWrappers import (
    construct_touch_vertex_first_time_call,
    construct_touch_cell_first_time_call,
    construct_touch_vertex_last_time_call,
)


class UpdateParticle_SingleLevelInteraction_ContiguousParticles(
    peano4.toolbox.particles.UpdateParticle_SingleLevelInteraction_ContiguousParticles
):
    """!
    
    Wrapper around toolbox baseclass
    
    Close to trivial wrapper which really only adds a tarch::multicore::waitForAllTasks()
    iff the particles might move. The interesting stuff is actually happing in 
    TaskGarphKernelWrappers.py however.
    
    """
    def __init__(
        self,
        particle_set,
        particle_cell_update_kernel,
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

        """
        if wait_for_all_tasks_to_finish_after_traversal:
            unprepare_traversal_kernel = (
                unprepare_traversal_kernel + "tarch::multicore::waitForAllTasks();"
            )

        super(UpdateParticle_SingleLevelInteraction_ContiguousParticles, self).__init__(
            particle_set,
            particle_cell_update_kernel=construct_touch_cell_first_time_call(
                particle_set,particle_cell_update_kernel,False
            ),
            touch_vertex_first_time_compute_particle_update_kernel=construct_touch_vertex_first_time_call(
                particle_set,touch_vertex_first_time_compute_particle_update_kernel,False
            ),
            touch_vertex_last_time_compute_particle_update_kernel=construct_touch_vertex_last_time_call(
                particle_set,touch_vertex_last_time_compute_particle_update_kernel,False,current_step_alters_particle_position
            ),
            prepare_traversal_kernel=prepare_traversal_kernel,
            unprepare_traversal_kernel=unprepare_traversal_kernel,
            additional_includes=additional_includes
            + """
#include "tarch/multicore/Tasks.h"                
#include "swift2/swift2.h"                
#include "swift2/TaskNumber.h"                
                 """,
            active_particle_set=active_particle_set,
        )


    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
