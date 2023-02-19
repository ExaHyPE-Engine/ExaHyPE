# This file is part of the Swift2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.toolbox.particles.InsertParticlesByCoordinates



class InsertParticlesByCoordinates(peano4.toolbox.particles.InsertParticlesByCoordinates):
  def __init__(self,
               particle_set,
               coordinates,
               initialisation_call,
               additional_includes = ""
               ):
    """

    Very simplistic wrapper around generic factory mechanism from
    the toolbox.

    """
    super( InsertParticlesByCoordinates, self ).__init__(particle_set        = particle_set,
                                                         coordinates         = coordinates,
                                                         initialisation_call = initialisation_call,
                                                         additional_includes = additional_includes + """
#include "Constants.h"
""")


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
