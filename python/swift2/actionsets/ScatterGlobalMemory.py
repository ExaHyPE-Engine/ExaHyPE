# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2
import dastgen2
import peano4


class ScatterGlobalMemory(ActionSet):
    """!

    Scatter all data 
    
    Memory pools can ask the system to scatter all data to start from a clean
    plate again. This is notably important for the global memory pool. 

    The action set is kind of the antidote to peano4.toolbox.particles.GatherParticlesInMemoryPool.
    It should be used within dummy sweeps typically.

    """
    def __init__(
        self,
        particle_set
    ):
        """!

        Construct the action set

        ## Arguments

        particle_set: peano4.toolbox.particles.ParticleSet
           Instance of the particle set that is to be scattered.


        """
        super(ScatterGlobalMemory,self).__init__()
        self._particle_set = particle_set
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        pass

    def user_should_modify_template(self):
        return False

    __Template_TouchVertexLastTime = jinja2.Template(
        """
  if ( vertexdata::{{PARTICLES_CONTAINER}}::MemoryPool::requestCompleteScatter() ) {
    fineGridVertex{{PARTICLES_CONTAINER}}.scatter();
  }
"""
    )

    def get_body_of_prepareTraversal(self):
        return jinja2.Template(
        """
  if ( vertexdata::{{PARTICLES_CONTAINER}}::MemoryPool::requestCompleteScatter() ) {
    logInfo( "prepareTraversal(...)", "start to scatter all memory to allow for a data rearrangement" );
  }
"""
    ).render(**self.d)

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_TouchVertexLastTime.render(**self.d)
        return result


    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")


    def get_includes(self):
        result = jinja2.Template(
            """
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
"""
        )
        return result.render(**self.d)
