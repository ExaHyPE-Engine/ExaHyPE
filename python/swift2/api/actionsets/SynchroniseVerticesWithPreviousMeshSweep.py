# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2
import peano4


class SynchroniseVerticesWithPreviousMeshSweep(ActionSet):
    def __init__(
        self,
        particle_set,
    ):
        """!

        Ensure that touchVertexLastTime() on vertex terminates prior to subsequent touchVertexLastTime()

        This action set is used by MultisweepTaskGraph, where we run through
        the mesh, spawn graphs and immediately switch to the next mesh traversal.
        This way, we produce the whole task graph of multiple logical steps
        on-the-fly. To make the whole thing work, we have to ensure that the
        task of touchVertexLastTime() of one traversal finishes before the one
        spawned by touchVertexFirstTime() is activated.

        In prinicple, we can embed this into the task creation by creating
        the corresponding in-dependency. I tried this in the TaskGraphKernelWrapper.
        However, when we write the touchVertexFirstTime() task, we really do not
        know if there has been another touchVertexFirstTime() task before. So
        instead of having complicated logic, I use an empty task to create that
        dependency once and for all.

        """
        self._particle_set = particle_set
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d[
            "MARKER_NAME"
        ] = peano4.toolbox.api.EnumerateCellsAndVertices.construct_marker_name(
            particle_set.name
        )

        return

    def user_should_modify_template(self):
        return False

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
      const std::set<::swift2::TaskNumber> inTaskNumber{ ::swift2::TaskNumber(
        fineGridVertex{{MARKER_NAME}}.getNumber(),
        ::swift2::TaskNumber::TaskAssociation::TouchVertexLastTime
      )};

      const ::swift2::TaskNumber outTaskNumber{
        fineGridVertex{{MARKER_NAME}}.getNumber(),
        ::swift2::TaskNumber::TaskAssociation::TouchVertexFirstTime
      };
        
      tarch::multicore::Task* newTask = new tarch::multicore::EmptyTask(
        tarch::multicore::Task::DefaultPriority
      );

      tarch::multicore::spawnTask( newTask, ::swift2::flatten(inTaskNumber), ::swift2::flatten(outTaskNumber) );        
        """
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self.__Template_TouchVertexFirstTime.render(**self.d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = jinja2.Template(
            """
#include "tarch/multicore/Task.h"
#include "swift2/swift2.h"                
#include "swift2/TaskNumber.h"                

#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
"""
        )
        return result.render(**self.d)
