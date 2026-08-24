# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

from peano4.particles.AbstractUpdateParticleGridAssociation import (
    AbstractUpdateParticleGridAssociation,
)
from peano4.solversteps.ActionSet import ActionSet


class GatherParticlesInMemoryPool(ActionSet):
    """Gathers each vertex's particles into its configured memory pool.

    The gather runs in ``touchVertexFirstTime()`` before action sets with the
    default particle-sorting order. A caller inserts it after traversals that
    change the particle-list layout and before kernels that require contiguous
    storage.
    """

    DefaultDescendInvocationOrder = (
        AbstractUpdateParticleGridAssociation.DefaultDescendInvocationOrder - 1
    )

    def __init__(self, particle_set):
        super().__init__(descend_invocation_order=self.DefaultDescendInvocationOrder)
        self._dictionary = {"PARTICLES_CONTAINER": particle_set.name}

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
  fineGridVertex{{PARTICLES_CONTAINER}}.gather();
"""
    )

    def get_body_of_operation(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            return self.__Template_TouchVertexFirstTime.render(**self._dictionary)
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector<peano4::grid::GridControlEvent>{};\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return jinja2.Template(
            """
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
"""
        ).render(**self._dictionary)
