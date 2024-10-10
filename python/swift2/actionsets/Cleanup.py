# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2
import peano4


class Cleanup(ActionSet):
    def __init__(
        self,
        particle_set,
    ):
        """!
        Cleanup action set

        As the final grid traversal, clean up after yourself. At the moment, this
        entails deleting particles properly.

        """
        self._particle_set = particle_set
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d[
            "MARKER_NAME"
        ] = peano4.toolbox.api.EnumerateCellsAndVerticesOnEnclaveMesh.construct_marker_name(
            particle_set.name
        )

        return

    def user_should_modify_template(self):
        return False

    __Template_TouchVertexLastTime = jinja2.Template(
        """
        fineGridVertex{{PARTICLES_CONTAINER}}.deleteParticles();
        """
    )

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
