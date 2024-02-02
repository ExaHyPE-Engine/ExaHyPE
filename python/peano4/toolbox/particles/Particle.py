# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4.datamodel.DaStGen2

from peano4.datamodel.DaStGen2 import DaStGen2

import dastgen2


class Particle(DaStGen2):
    """!

    Single particle

    Represent a single particle. This is a DaStGen2 wrapper, i.e. I
    define a DaStGen object and add some particular fields that I always
    need to administer the particles.


    ## Usage

    If you use these particles, please do not add them to your use
    definitions of the actions/observers. For pidt, you need a second
    ParticleSet and this one is used by the observers.

    You have to add the particle to your project though via

            my_project.add_global_object

    If you want to attributes to a particle, use the data subattribute.
    An all-time classic is the call

           add_attribute( peano4.dastgen2.Peano4DoubleArray("v","Dimensions") )

    ## Pre-defined fields

    I actually need only very few fields in Peano's particle toolbox:

    - A position x. You can change this position in your code, but please
      invoke a particle re-sort once you change a particle's position. Usually,
      I employ the pidt (particle in dual tree) scheme, i.e. particles are
      always stored in the closest vertex. If you alter the position of a
      particle, you might destroy this association and have to re-assign the
      object to another vertex.
    - A search radius. Every particle in Peano has a search radius, i.e. a maximal
      interaction radius. As I only compare particles stored in one vertex to
      particles stored in a cell-connected vertex, the search radius also
      implicitly determines the tree grid level that I use to hold a particle.
      The bigger the search, the higher up in the mesh I have to store a
      particle.
    - A state. This state should not be manipulated by the user. A tree owns the
      particles that are contained within its local tree cells. Furthermore,
      a tree knows about the particles which are associated to a vertex that's
      adjacent to a local mesh. The latter particles are virtual. We effectively
      work with a halo of h/2 per mesh level. 
      
    The only action set that should alter the state is UpdateParallelState. 
      
    ## State updates  
    
    If a particle moves, we have to update its state, and we might have to 
    update its vertex association. This discussion focuses on the state 
    update. 
    
    Details can be found in
      UpdateParallelState.

    """

    def __init__(self, name):
        """!

        Constructor

        ## Attributes

        name: String
          Name of the particle. Has to be a valid C++ class name. We pass this
          to the superclass as fully qualified name

        """
        peano4.datamodel.DaStGen2.__init__(self, name)

        self.name = name

        self.data.add_attribute(peano4.dastgen2.Peano4DoubleArray("x", "Dimensions"))
        self.data.add_attribute(
            peano4.dastgen2.Peano4DoubleArray("cellH", "Dimensions")
        )
        self.data.add_attribute(dastgen2.attributes.Double("searchRadius"))

        #
        # Never manipulate as user
        #
        self.data.add_attribute(
            dastgen2.attributes.Enumeration("ParallelState", ["Local", "Virtual"])
        )
