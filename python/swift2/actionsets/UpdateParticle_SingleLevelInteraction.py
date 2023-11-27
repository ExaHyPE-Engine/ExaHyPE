# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles



class UpdateParticle_SingleLevelInteraction(peano4.toolbox.particles.UpdateParticle_SingleLevelInteraction):
    def __init__(self,
                 particle_set,
                 particle_cell_update_kernel,
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
            unprepare_traversal_kernel = unprepare_traversal_kernel + "tarch::multicore::waitForAllTasks();"

        super( UpdateParticle_SingleLevelInteraction, self ).__init__(
                 particle_set,
                 particle_cell_update_kernel,
                 touch_vertex_first_time_compute_particle_update_kernel,
                 touch_vertex_last_time_compute_particle_update_kernel,
                 prepare_traversal_kernel,
                 unprepare_traversal_kernel,
                 additional_includes + """
#include "tarch/multicore/Tasks.h"                
#include "swift2/VertexTask.h"                
#include "swift2/CellTask.h"                
#include "swift2/TaskNumber.h"                
                 """,
                 active_particle_set,
                 )
