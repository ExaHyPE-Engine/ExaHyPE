# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from enum import Enum

from abc import abstractmethod


class AlgorithmStep(object):
    """!

    Defines the meta data around one algorithmic step per particle

    A sequence of these steps describes the particle lifecycle per time step. It
    is Peano's/SWIFT's responsibility to arrange these steps in grid sweeps and
    to identify concurrency.

    """

    class Dependencies(Enum):
        """

        An algorithmic step for one particle can depend on different
        pre-conditions:

        - It can only depend on its own previous step (SELF);
        - It can depend on the neighbours within a particle's interaction radius
          (NEIGHBOURS);

        """

        SELF = 0
        NEIGHBOURS = 1

    class Effect(Enum):
        """

        What happens in this algorithmic step per particle:

        - ALTER_GLOBAL_STATE is the most generic baseline routine. No local data
          are changed, but the code (might) alter some global properties. Typical
          examples are the reduction of the global admissible time step size.
        - ALTER_LOCAL_STATE implies that only the local state of a particle is
          changed. The code might also modify the global state, but it may not
          change a particle's position or its search radius, i.e. the spatial
          topology between particles.
        - CHANGE_POSITION_OR_INTERACTION_RADIUS is the most general update which
          means that literally any global or local attribute might be modified.

        Please note that any update of a particle position should happen within
        touch vertex last time. Otherwise, we'd destroy the particle topology.

        """

        ALTER_GLOBAL_STATE = 0
        ALTER_LOCAL_STATE = 1
        CHANGE_POSITION_OR_INTERACTION_RADIUS_BUT_NEVER_RERUN = 2
        CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN = 3

    def __init__(
        self,
        name,
        dependencies: Dependencies,
        effect: Effect,
        cell_kernel=None,
        touch_vertex_first_time_kernel=None,
        touch_vertex_last_time_kernel=None,
        prepare_traversal_kernel="",
        unprepare_traversal_kernel="",
        input_particles=None,
        includes="",
    ):
        """

        The algorithmic step description is a meta data object, i.e. it only
        holds properties.

        ## Parameters/attributes

        cell_kernel: C++ code (string)
          Kernel invocation what happens in a cell. Set to None if nothing
          is to be done per cell.

        touch_vertex_first_time_kernel: C++ code (string)
          Kernel invocation what happens when we read a vertex and its particles
          for the first time. Set to None if nothing is to be done.

        touch_vertex_last_time_kernel: C++ code (string)
          Kernel invocation what happens when we read a vertex and its particles
          for the last time. Set to None if nothing is to be done.

        input_particles: ParticleSet
          Switch to None in most cases. This fragment gives you the opportunity
          to couple the particles of one step with another particle species. If
          you hand in None, then we assume that the algorithm step updates the
          particles as input function from the particles.

        includes: String
          Additional include statements

                   prepare_traversals_kernel        = None,

        """
        self.name = name
        self.dependencies = dependencies
        self.effect = effect

        self.cell_kernel = cell_kernel
        self.touch_vertex_first_time_kernel = touch_vertex_first_time_kernel
        self.touch_vertex_last_time_kernel = touch_vertex_last_time_kernel
        self.prepare_traversal_kernel = prepare_traversal_kernel
        self.unprepare_traversal_kernel = unprepare_traversal_kernel

        self.input_particles = input_particles
        self.includes = includes

    def __str__(self):
        return "({},{},{})".format(self.name, self.dependencies, self.effect)
