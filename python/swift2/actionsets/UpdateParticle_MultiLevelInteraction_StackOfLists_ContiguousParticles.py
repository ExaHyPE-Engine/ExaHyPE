# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.toolbox.particles


class UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles(
    peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles
):
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
        wait_for_all_tasks_to_finish_after_traversal,
    ):
        """!

        Construct


        @param wait_for_all_tasks_to_finish_after_traversal: Boolean

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
            particle_particle_interaction_kernel,
            touch_vertex_first_time_compute_particle_update_kernel,
            touch_vertex_last_time_compute_particle_update_kernel,
            prepare_traversal_kernel,
            unprepare_traversal_kernel,
            additional_includes
            + """
#include "tarch/multicore/Tasks.h"                
#include "swift2/VertexTask.h"                
#include "swift2/CellTask.h"                
#include "swift2/TaskNumber.h"                
                 """,
            active_particle_set,
        )

    def get_body_of_operation(self, operation_name):
        if operation_name == peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
            return "_pendingDependencies.empty();"
        else:
            return super(
                UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles,
                self,
            ).get_body_of_operation(operation_name)

    def get_attributes(self):
        return (
            super(
                UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles,
                self,
            ).get_attributes()
            + """
          ::swift2::PendingDependencies  _pendingDependencies;
        """
        )
