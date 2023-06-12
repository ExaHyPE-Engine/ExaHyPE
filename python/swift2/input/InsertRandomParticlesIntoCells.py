# This file is part of the Swift2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles.InsertParticlesAlongCartesianLayoutIntoUnrefinedCells


class InsertRandomParticlesIntoCells(
    peano4.toolbox.particles.InsertParticlesAlongCartesianLayoutIntoUnrefinedCells
):
    def __init__(
        self,
        particle_set,
        average_distance_between_particles_along_one_axis,
        initialisation_call,
        additional_includes="",
    ):
        """!

        Very simplistic wrapper around generic factory mechanism from
        the toolbox.

        """
        super(InsertRandomParticlesIntoCells, self).__init__(
            particle_set=particle_set,
            average_distance_between_particles=average_distance_between_particles_along_one_axis,
            initialisation_call=initialisation_call,
            noise=True,
            additional_includes=additional_includes
            + """
#include "Constants.h"
""",
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
