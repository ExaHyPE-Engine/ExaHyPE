# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

from peano4.toolbox.particles.api.AbstractUpdateParticleGridAssociation import (
    AbstractUpdateParticleGridAssociation,
)

import jinja2


class GatherParticlesInMemoryPool(ActionSet):
    """!

    Simplistic action set which invokes gather() on the particle set

    Plugs into touchVertexFirstTime(). Should be called prior to any use of
    vertex data, but after all rearrangements have completed.

    """

    DefaultDescendInvocationOrder = (
        AbstractUpdateParticleGridAssociation.DefaultDescendInvocationOrder - 1
    )

    def __init__(
        self,
        particle_set,
    ):
        super(GatherParticlesInMemoryPool, self).__init__(
            descend_invocation_order=self.DefaultDescendInvocationOrder, parallel=False
        )
        self._particle_set = particle_set
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
  fineGridVertex{{PARTICLES_CONTAINER}}.gather();
"""
    )

    __Template_TouchVertexLastTime = jinja2.Template(
        """
  assertion1( fineGridVertex{{PARTICLES_CONTAINER}}.isGathered(), fineGridVertex{{PARTICLES_CONTAINER}}.toString() );
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self.__Template_TouchVertexFirstTime.render(**self.d)
        return result

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< peano4::grid::GridControlEvent >();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        result = jinja2.Template(
            """
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"
"""
        )
        return result.render(**self.d)
