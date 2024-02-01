# This file is part of the SWIFT2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from enum import Enum, IntEnum

from abc import abstractmethod

import peano4


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
          However, the graph compilers have to know whether you might ask it to
          rerun this step. This is important, as this step might change the
          topology.

        Please note that any update of a particle position should happen within
        touch vertex last time. Otherwise, we'd destroy the particle topology.

        By default, any algorithm step runs only once. However, there are
        variants of each step which can rerun. But the user has to flag this
        possibility explicitly.

        """

        ALTER_GLOBAL_STATE = 0
        ALTER_GLOBAL_STATE_AND_MIGHT_RERUN = 1
        ALTER_LOCAL_STATE = 2
        ALTER_LOCAL_STATE_AND_MIGHT_RERUN = 3
        CHANGE_POSITION_OR_INTERACTION_RADIUS = 4
        CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN = 5

    def may_trigger_rerun(effect: Effect):
        """!

        Return True if the effect object may trigger a rerun

        """
        if effect in [
            AlgorithmStep.Effect.ALTER_GLOBAL_STATE_AND_MIGHT_RERUN,
            AlgorithmStep.Effect.ALTER_LOCAL_STATE_AND_MIGHT_RERUN,
            AlgorithmStep.Effect.CHANGE_POSITION_OR_INTERACTION_RADIUS_AND_MIGHT_RERUN,
        ]:
            return True
        else:
            return False

    class PeanoEventUsedBySwift(IntEnum):
        """!

        Enumeration of different @ref peano_action_sets "Peano events" during a
        grid traversal into which Swift 2 plugs in. We don't use all events.
        Therefore, this is a subset of the operations in
        peano4.solversteps.ActionSet. The enumeration is based upon integers,
        as we need to know the order of the enumerations.

        Should we ever pick up more events, make sure to also update the dict
        containing the names of the stages in
        """

        TOUCH_VERTEX_FIRST_TIME = 0
        CELL_KERNEL = 1
        TOUCH_VERTEX_LAST_TIME = 2
        EVENT_COUNT = 3

    @staticmethod
    def get_event_name(stage: PeanoEventUsedBySwift):
        """!

        Get the name as a string of a sweep stage for a given PeanoEventUsedBySwift enum.

        """
        Results = {
            AlgorithmStep.PeanoEventUsedBySwift.TOUCH_VERTEX_FIRST_TIME: peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME,
            AlgorithmStep.PeanoEventUsedBySwift.CELL_KERNEL: "cellKernel",
            AlgorithmStep.PeanoEventUsedBySwift.TOUCH_VERTEX_LAST_TIME: peano4.solversteps.ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME,
            AlgorithmStep.PeanoEventUsedBySwift.EVENT_COUNT: "count",
        }
        return Results[stage]

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
        cell_kernel_dependency_policy=None,
        touch_vertex_first_time_dependency_policy=None,
        touch_vertex_last_time_dependency_policy=None,
    ):
        """!

        The algorithmic step description is a meta data object, i.e. it only
        holds properties.

        ## Parameters/attributes

        @param cell_kernel: C++ code (string)
          Kernel invocation what happens in a cell. Set to None if nothing
          is to be done per cell.

        @param touch_vertex_first_time_kernel: C++ code (string)
          Kernel invocation what happens when we read a vertex and its particles
          for the first time. Set to None if nothing is to be done.

        @param touch_vertex_last_time_kernel: C++ code (string)
          Kernel invocation what happens when we read a vertex and its particles
          for the last time. Set to None if nothing is to be done.

        @param input_particles: ParticleSet
          Switch to None in most cases. This fragment gives you the opportunity
          to couple the particles of one step with another particle species. If
          you hand in None, then we assume that the algorithm step updates the
          particles as input function from the particles.

        @param includes: String
          Additional include statements

                   prepare_traversals_kernel        = None,

        @param prepare_traversal_kernel: String (C++ code snippet)
          Whenever Peano runs through the mesh, it creates one instance of the
          mesh observer and then runs through all subpartitions (trees) in
          parallel. Each replica of the observer issues its own touchVertexFirstTime(),
          touchCellLastTime(), ... events, and each replica starts its traversal
          with a call to beginTraversal() and endTraversal(). If you want to
          plug into the phase such before these parallel instances are created,
          you can use prepareTraversal() - the C++ code snippet passed to this
          attribute is copied into prepareTraversal(). The overview over
          @ref peano_action_sets "action sets" provides more details. Note that
          no action set (or observer object) is created at this point, i.e. the
          C++ code snippet will be injected into a static routine and can
          therefore only access static information. Often, this routine is used
          for some MPI communication (global data exchange), e.g., or to update
          some other static properties for whole sets of objects.

        @param unprepare_traversal_kernel: String (C++ code snippet)
          Counterpart of prepare_traversal_kernel. Consult
          @ref peano_action_sets "action sets" for more context.

        @param cell_kernel_dependency_policy: String or None
          If it is a string, it should identify one of the variants of
          swift2::dependencychecks::Invariant. If you pick None, Swift 2
          will add default dependencies. They might be too strict.

        @param touch_vertex_first_time_dependency_policy: String or None
          If it is a string, it should identify one of the variants of
          swift2::dependencychecks::Invariant. If you pick None, Swift 2
          will add default dependencies. They might be too strict.

        @param touch_vertex_last_time_dependency_policy: String or None
          If it is a string, it should identify one of the variants of
          swift2::dependencychecks::Invariant. If you pick None, Swift 2
          will add default dependencies. They might be too strict.

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
        self.includes = (
            """
#include "Constants.h"
#include "swift2/kernels/ParticleUpdatePredicates.h"
#include "swift2/kernels/ParticleSetIterators.h"

#include "swift2/dependencychecks/DependencyChecks.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"
#include "toolbox/particles/assignmentchecks/Utils.h"
        """
            + includes
        )

        self.cell_kernel_dependency_policy = cell_kernel_dependency_policy
        self.touch_vertex_first_time_dependency_policy = (
            touch_vertex_first_time_dependency_policy
        )
        self.touch_vertex_last_time_dependency_policy = (
            touch_vertex_last_time_dependency_policy
        )

    def __str__(self):
        return "({},{},{})".format(self.name, self.dependencies, self.effect)
