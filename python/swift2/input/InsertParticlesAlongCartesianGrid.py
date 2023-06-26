# This file is part of the Swift2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles.InsertParticlesAlongCartesianLayoutIntoUnrefinedCells


class InsertParticlesAlongCartesianGrid(
    peano4.toolbox.particles.InsertParticlesAlongCartesianLayoutIntoUnrefinedCells
):
    def __init__(
        self,
        particle_set,
        distance_between_particles_along_one_axis,
        initialisation_call,
        additional_includes="",
    ):
        """!

        Very simplistic wrapper around generic factory mechanism from
        the toolbox.

        """
        super(InsertParticlesAlongCartesianGrid, self).__init__(
            particle_set=particle_set,
            distance_between_particles=distance_between_particles_along_one_axis,
            initialisation_call=initialisation_call,
            noise=False,
            computational_domain_offset=[
                "DomainOffset[0]",
                "DomainOffset[1]",
                "DomainOffset[2]",
            ],
            computational_domain_size=[
                "DomainSize[0]",
                "DomainSize[1]",
                "DomainSize[2]",
            ],
            additional_includes=additional_includes
            + """
#include "Constants.h"
""",
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
