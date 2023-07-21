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

        Wrapper around toolbox tying it to Swift's global properties

        Very simplistic wrapper around generic factory mechanism from
        the toolbox. The script adds references to DomainOffset[2] and
        DomainSize[2] for example which are defined in Constants.h. This
        constants file is something that's specific to Swift and not to
        the particles toolbox.

        We do not care at this point, if entries like DomainOffset[2]
        do exist, i.e. if we run a 3d simulation. These are just inserted
        into the generated C++ code which then in turn masks entries like
        DomainSize[2] out if we work with Dimensions==2.

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
