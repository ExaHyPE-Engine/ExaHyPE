# This file is part of the Swift2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles.InsertRandomParticlesIntoUnrefinedCells



class InsertParticlesAlongCartesianGrid(peano4.toolbox.particles.InsertRandomParticlesIntoUnrefinedCells):
  def __init__(self,
               particle_set,
               distance_between_particles_along_one_axis,
               initialisation_call,
               additional_includes = "",
               noise=False,
               ):
    """!

    Very simplistic wrapper around generic factory mechanism from
    the toolbox.

    """
    super( InsertParticlesAlongCartesianGrid, self ).__init__(particle_set        = particle_set,
                                                              average_distance_between_particles         = distance_between_particles_along_one_axis,
                                                              initialisation_call = initialisation_call,
                                                              noise               = noise,
                                                              additional_includes = additional_includes + """
#include "Constants.h"
""")


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
