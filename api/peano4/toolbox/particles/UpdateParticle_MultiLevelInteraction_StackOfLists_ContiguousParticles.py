# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles(ActionSet):
    """!
    Tree walker for particle-particle interactions across adjacent cells and mesh levels.

    This code snippet creates a tree walker, i.e. an action set that runs
    through the underlying spacetree top down and builds up
    @ref page_toolbox_particles_mesh_traversal "the active set and the local set".
    These sets can then be used to trigger mesh events.
    It works only when you use continuous storage per vertex as introduced
    with the @ref toolbox_particles_memorypool "memory pools".

    Within touchCellFirstTime(), the routine exposes the following data
    besides the generic information of any action set:

    - ***marker*** The CellMarker is available to a compute kernel and allows you
      to query spatial information. It is one of the generic objects which every
      action set has available, but we highlight that it is there, as
      the @ref page_toolbox_particles_mesh_traversal "discussion on uniqueness for particle-particle interactions"
      explicitly refers to the marker.
    - ***activeParticles*** The list of active particles is a list of pointers. It
      follows the @ref page_toolbox_particles_mesh_storage "generic definition of active sets".
    - ***localParticles*** This is a list of local particles, i.e. particles which
      are tied to the cell's adjacent vertices. While they reside on the cell's
      level, we have no guarantee that they reside within the current cell. They
      might also be contained within its halo of marker.getH()/2.

    The additional field _activeParticles is an alias for activeParticles.

    The code is slightly more sophisticated than its cousin, the
    UpdateParticle_MultiLevelInteraction_Sets, as it exposes more information:

    - ***numberOfCoalescedActiveParticlesPerVertex*** This alias for
      _numberOfActiveParticlesPerVertex is a sequence of integers
      which tells you how many particles within activeParticles are added by one
      particular vertex.
    - ***numberOfCoalescedLocalParticlesPerVertex*** is a sequence of integers which
      tells you how many particles within localParticles are
      added by each adjacent vertex.

    Both pieces of information are important if you work with memory pools
    as discussed below.

    ## Different particles species

    Both active and local set are of the same type if you stick
    to the default value of None for active_particle_set. But you can also
    let particles of different type interact by selecting a local particle
    set that is different to the active one. Users can inject code to modify
    the local set per cell and hence to alter particle states.

    ## Realisation and data consistency

    This tree walker realises the two sets via two lists: Per cell, we
    append the local set to the active set and memorise on a stack how
    many of these particles have been added. When we backtrace, i.e.
    ascend within the tree again, this memorised size is used to remove
    the last n elements from the active set.

    So the active set grows while we descend in the tree, and it is
    truncated when we ascend again.

    The implementation requires that particles do not move up and down
    within the tree while we walk. If particles move up and down, the
    present algorithms might have redundant pointers within the active set.
    It is therefore absolutely key that this action set is not used in
    combination with any other action set which lifts data, as a lift
    destroys contributions made by coarser mesh cells. If particles drop,
    we also cannot use this action set, as, again, it might corrupt the
    active sets constructed by coarser meshes. The issue with the lifts
    notably becomes apparent once you take into account that lifts trigger
    a scatter on gathered vertices. That is, we descend within the tree and
    build up our continuous index spaces. Then, later, we decide that we
    lift one particle. This lift means that particles associated with a
    coarser vertex are scattered. However, our active set assumes that
    we had a valid, continuous memory for them. So all of our active sets
    are now invalid. Sieves in contrast work, as long as their action
    set is used before the current one. They merely add data to the
    active set that's constructed next.

    - No lifts.
    - No drops.
    - Sieves are fine as long as they happen before touchVertexFirstTime()
      of this action set.
    - Lifts into a global sieve list are fine, as long as they happen in
      touchVertexLastTime() called after this action set.

    In the case of a dynamic setup, i.e. in combination with the action
    set UpdateParticleGridAssociation, you might be better off using
    the set-based implementation of the particle-particle interaction.

    ## Further plug-in points

    Besides the cell kernel which is there to realise particle-to-particle
    interactions, we also have a vertex kernel which we call whenever a
    vertex is loaded for the first time. That is, you can assume that the
    vertex kernel has been launched for all @f$ 2^d @f$ vertices of a cell before
    its cell kernel is triggered. The vertex kernel is also passed on the
    active set (from the coarser level). Its local set is all the particles
    whose centre lies within the square/cube around the vertex with mesh size
    h. So this area goes h/2 along each coordinate axis into the
    neighbouring cells.

    ## Flattening of particle arrays

    The routine not only keeps track of how many particles we add per iteration.
    It also keeps track of the number of particles per vertex that have been
    added. If you know that you work with a @ref page_toolbox_particles_mesh_storage "memory pool", i.e. a consecutive
    storage of particles, you can use this additional meta information within
    your kernel invocations to work with loops over consecutive arrays (AoS)
    rather than sole link lists. You can exploit that you know something
    about individual chunks indexed by the pointers.

    To exploit this additional information, you cannot run through localParticles
    particle by particle anymore. You have to walk the list chunk by chunk, and
    consequently you have to check yourself whether a particle is contained
    within the cell.
    Once you rewrite your kernels that way, you can exploit the integers
    numberOfCoalescedActiveParticlesPerVertex and
    numberOfCoalescedLocalParticlesPerVertex:

    @image html UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles_continuous-memory.png

    Assume you enter a cell and you get the localParticles
    list hosting 19 pointers. Further to that, you receive the array
    numberOfCoalescedLocalParticlesPerVertex with four integer entries. You know that the
    first five entries point to one continuous chunk of particles in memory, the
    next seven do so as well, and so forth. This holds only when you work
    with a memory pool which maintains vertex-local or globally continuous
    particles.

    Call toolbox::particles::ensureThatParticleListsEncodeContinuousMemoryChunks()
    for your data to ensure that the memory layout above is preserved. Simply
    add the calls

    ~~~~~~~~~~~~~~~~~~~~~~~~~~
    ::toolbox::particles::ensureThatParticleListsEncodeContinuousMemoryChunks(
      activeParticles,
      _numberOfActiveParticlesPerVertex
    );
    ::toolbox::particles::ensureThatParticleListsEncodeContinuousMemoryChunks(
      localParticles,
      numberOfCoalescedLocalParticlesPerVertex
    );
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    to your cell-wise compute kernel. Furthermore, you might want to check if all
    vertices are properly gathered:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~
    ::toolbox::particles::ensureAllParticleListsAreGatheredOrEmpty(
      fineGridVerticesMyParticleSetName
    );
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    The actual compute kernels then typically look similar to the following code
    snippet:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~
    typename std::list<T*>::const_iterator localParticlesIterator  = localParticles.begin();

    for (auto localParticlesChunkSize: numberOfCoalescedLocalParticlesPerVertex) {
      T*  localParticlesChunk = *localParticlesIterator;
      std::advance( localParticlesIterator, localParticlesChunkSize );

      for (int localParticleNumberInThisChunk=0; localParticleNumberInThisChunk<localParticlesChunkSize; localParticleNumberInThisChunk++) {
        if (
          marker.isContained( localParticlesChunk[localParticleNumberInThisChunk].getX() )
          and
          // ensure that particle is not updated twice
        ) {
          [...]
        }
      }
    }
    ~~~~~~~~~~~~~~~~~~~~~~~~~~


    Iterating over the active set follows the same pattern. Most codes will
    have to couple local and active sets and hence use a kernel structure
    similar to

    ~~~~~~~~~~~~~~~~~~~~~~~~~~
    typename std::list< typename ParticleContainer::value_type >::const_iterator localParticlesIterator  = localParticles.begin();

    for (auto localParticlesChunkSize: numberOfCoalescedLocalParticlesPerVertex) {
      typename ParticleContainer::value_type  localParticlesChunk = *localParticlesIterator;
      std::advance( localParticlesIterator, localParticlesChunkSize );

      typename std::list< typename ParticleContainer::value_type >::const_iterator activeParticlesIterator = activeParticles.begin();
      for (auto activeParticlesChunkSize: numberOfCoalescedActiveParticlesPerVertex) {
        typename ParticleContainer::value_type  activeParticlesChunk = *activeParticlesIterator;
        std::advance( activeParticlesIterator, activeParticlesChunkSize );

        computeInnerLoop(
            marker,
            localParticlesChunk,
            localParticlesChunkSize,
            activeParticlesChunk,
            activeParticlesChunkSize
        );
      }
    }
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    The inner loop can exploit the fact that localParticlesChunk and
    activeParticlesChunk both point to continuous memory regions. This means
    that computeInnerLoop is structurally simple:

    ~~~~~~~~~~~~~~~~~~~~~~~~~~
    void computeInnerLoop(
        const ::peano4::datamanagement::CellMarker& marker,
        Particle*                                  localParticlesChunk,
        int                                        numberOfLocalParticles,
        Particle*                                  activeParticlesChunk,
        int                                        numberOfActiveParticles
    ) {
      for (int activeParticleNumberInThisChunk=0; activeParticleNumberInThisChunk<numberOfActiveParticles; activeParticleNumberInThisChunk++) {
        for (int localParticleNumberInThisChunk=0; localParticleNumberInThisChunk<numberOfLocalParticles; localParticleNumberInThisChunk++) {
          // read from activeParticlesChunk[activeParticleNumberInThisChunk]
          // write to localParticlesChunk[localParticleNumberInThisChunk]
        }
      }
    }
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    ## Dynamic adaptive mesh refinement

    In the second sweep of any adaptive mesh refinement, we might drop particles
    and hence destroy our nicely arranged memory layout.
    Once particles move up and down the mesh hierarchy, the entries in
    _numberOfActiveParticlesPerVertex are all invalid.
    In such a case, we have to switch off any coalesced memory access.

    The @ref page_toolbox_particles_mesh_traversal "generic mesh traversal description" provides
    further context and details.
    The important detail here is:
    As soon as we might suspect that the memory layout is corrupted, we have to
    assume that our pointers are all over the place and do not point to continuous
    data anymore. Even worse, the active sets built up further up in the tree
    might be invalid. We build them up top-down, but when we drop data out of the
    active set, they change on-the-fly.

    ## Optimising coalesced compute kernels

    Compute kernels can be optimised quite aggressively, but working out how they
    can be optimised is tedious. MAQAO and Intel's compiler optimisation feedback
    help here, and the page @ref toolbox_particles_memorypool holds additional
    information. Some lessons learned:

    - You cannot collapse the two for loops. That would lead to a scattered
      iteration space in memory and hence make any vectorisation impossible.
    - The active loop has to be the inner one. Intel's icpx refuses to
      vectorise if the local loop is the inner one.
    - The compiler has to be able to inline. According to https://clang.llvm.org/docs/AttributeReference.html#always-inline-force-inline
      this is something one should be able to control via the annotation

              [[clang::always_inline]]

      but this led to multiple compiler crashes (again with Intel). The
      function has to reside in the header, and ipo does not resolve the
      fundamental issue either.
    - You might have to recursively study all inlined methods and make the
      routines they use inline as well.
    - Typically, you have to embed the inner-most loop into a statement alike
      ~~~~~~~~~~~~~~~~~~~~~~~
        if (
          marker.isContained(localParticlesChunk[localParticleNumberInThisChunk].getX())
          and
          not localParticlesChunk[localParticleNumberInThisChunk].getCellHasUpdatedParticle()
        ) {
          // read from activeParticlesChunk[activeParticleNumberInThisChunk]
          // write to localParticlesChunk[localParticleNumberInThisChunk]
        }
      ~~~~~~~~~~~~~~~~~~~~~~~

    The @ref page_peano_performance_optimisation "generic Peano optimisation" page
    provides some useful hints how to guide these optimisations through the
    compiler.
    """

    def __init__(
        self,
        particle_set,
        particle_particle_interaction_kernel,
        touch_vertex_first_time_compute_particle_update_kernel=None,
        touch_vertex_last_time_compute_particle_update_kernel=None,
        prepare_traversal_kernel="",
        unprepare_traversal_kernel="",
        additional_includes="",
        active_particle_set=None,
    ):
        """!
        Initialise the action set.

        ## Arguments

        particle_set: ParticleSet

        particle_particle_interaction_kernel: String holding C++ code

        active_particle_set: ParticleSet or None
          You can compare different particles species with this argument. It
          allows you to make the active particles stem from another species than
          the local ones that you actually update. Pick None if both sets are of
          the same type.

        touch_vertex_first_time_compute_particle_update_kernel: String or None
          Can be empty, but if you want to move particles, then a minimal example
          string passed equals

              for (auto* localParticle: localParticles ) {
                localParticle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );
              }

          i.e. you will have a variable localParticle available in this kernel
          and it is a pointer.
        """
        super(
            UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles, self
        ).__init__(descend_invocation_order=1)

        self._particle_set = particle_set
        self.d = {}
        self.d["LOCAL_PARTICLE"] = particle_set.particle_model.name
        self.d["LOCAL_PARTICLES_CONTAINER"] = particle_set.name
        if active_particle_set is None:
            self.d["ACTIVE_PARTICLE"] = particle_set.particle_model.name
            self.d["ACTIVE_PARTICLES_CONTAINER"] = particle_set.name
        else:
            self.d["ACTIVE_PARTICLE"] = active_particle_set.particle_model.name
            self.d["ACTIVE_PARTICLES_CONTAINER"] = active_particle_set.name
        self.d[
            "PARTICLE_PARTICLE_INTERACTION_KERNEL"
        ] = particle_particle_interaction_kernel
        self.d[
            "TOUCH_VERTEX_FIRST_COMPUTE_KERNEL"
        ] = touch_vertex_first_time_compute_particle_update_kernel
        self.d[
            "TOUCH_VERTEX_LAST_COMPUTE_KERNEL"
        ] = touch_vertex_last_time_compute_particle_update_kernel
        self.d["ADDITIONAL_INCLUDES"] = additional_includes
        self.d["PREPARE_TRAVERSAL_KERNEL"] = prepare_traversal_kernel
        self.d["UNPREPARE_TRAVERSAL_KERNEL"] = unprepare_traversal_kernel

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
  {% if TOUCH_VERTEX_FIRST_COMPUTE_KERNEL!=None %}
  auto& assignedParticles  = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  int  numberOfCoalescedAssignedParticles = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}}.isGathered() ? fineGridVertex{{LOCAL_PARTICLES_CONTAINER}}.size() : -1;

  {{TOUCH_VERTEX_FIRST_COMPUTE_KERNEL}}
  {% endif %}
"""
    )

    __Template_TouchVertexLastTime = jinja2.Template(
        """
  {% if TOUCH_VERTEX_LAST_COMPUTE_KERNEL!=None %}
  auto& assignedParticles  = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  int  numberOfCoalescedAssignedParticles = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}}.isGathered() ? fineGridVertex{{LOCAL_PARTICLES_CONTAINER}}.size() : -1;

  {{TOUCH_VERTEX_LAST_COMPUTE_KERNEL}}
  {% endif %}
"""
    )

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  std::list< globaldata::{{LOCAL_PARTICLE}}* >  localParticles;
  std::vector< int >                            numberOfCoalescedLocalParticlesPerVertex( TWO_POWER_D );
  std::vector< bool >                           localParticleIsContainedInCell;

  _numberOfActiveParticlesAdded.push_back(0);
  for (int i=0; i<TWO_POWER_D; i++) {
    _numberOfActiveParticlesPerVertex.push_back( fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i).size() );
    _numberOfActiveParticlesAdded.back()        += fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i).size();
    numberOfCoalescedLocalParticlesPerVertex[i]  = fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i).size();

    for (auto* p: fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i) ) {
      _activeParticles.push_back( p );
    }

    for (auto* p: fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(i) ) {
      std::bitset<TWO_POWER_D> mask = 0;
      mask.set(TWO_POWER_D-i-1);
      bool isLocal = (mask & p->getContainedInAdjacentCell()).any();
      assertion((isLocal) == (marker.isContained( p->getX(), toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance( marker.getH() ) * tarch::la::NUMERICAL_ZERO_DIFFERENCE )), isLocal, marker.isContained( p->getX(), toolbox::particles::internal::relativeGrabOwnershipSpatialSortingTolerance( marker.getH() ) * tarch::la::NUMERICAL_ZERO_DIFFERENCE ), marker.toString(), p->toString(), i, mask);
      localParticleIsContainedInCell.push_back( isLocal );
      localParticles.push_back( p );
    }
  }

  logDebug("size of local/active particles=" << localParticles.size() << "/" << _activeParticles.size() << " at " << marker.toString());

  {% if PARTICLE_PARTICLE_INTERACTION_KERNEL!=None %}
  std::list< globaldata::{{ACTIVE_PARTICLE}}* >&  activeParticles = _activeParticles;
  auto& numberOfCoalescedActiveParticlesPerVertex = _numberOfActiveParticlesPerVertex;
  assertion(
    not numberOfCoalescedActiveParticlesPerVertex.empty()
    and
    (numberOfCoalescedActiveParticlesPerVertex.size() % TWO_POWER_D == 0),
    numberOfCoalescedActiveParticlesPerVertex.size()
  );
  {{PARTICLE_PARTICLE_INTERACTION_KERNEL}}

  // The interaction kernel is allowed to move local particles. If it does,
  // containedInAdjacentCell becomes stale immediately. Refresh it before the
  // next cell consumes the cached ownership bitset and checks it against the
  // current particle position.
  for (int i=0; i<TWO_POWER_D; i++) {
    const std::bitset<DIMENSIONS> assignedVertex(i);
    for (auto* p: fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(i)) {
      toolbox::particles::updateContainedInAdjacentCellProperty(
        marker, assignedVertex, *p
      );
    }
  }
  {% endif %}
"""
    )

    __Template_TouchCellLastTime = jinja2.Template(
        """
  assertion( not _numberOfActiveParticlesAdded.empty() );
  assertion( _activeParticles.size()>=_numberOfActiveParticlesAdded.back() );
  assertion( _numberOfActiveParticlesPerVertex.size()>=TWO_POWER_D );

  _activeParticles.erase(
    std::prev(_activeParticles.end(),_numberOfActiveParticlesAdded.back() ),
    _activeParticles.end()
  );

  _numberOfActiveParticlesPerVertex.erase(
    std::prev(_numberOfActiveParticlesPerVertex.end(),TWO_POWER_D ),
    _numberOfActiveParticlesPerVertex.end()
  );

  _numberOfActiveParticlesAdded.pop_back();
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  assertion( _activeParticles.empty(), (*_activeParticles.begin())->toString() );
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCellFirstTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.__Template_TouchCellLastTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self.__Template_TouchVertexFirstTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_TouchVertexLastTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self.__Template_EndTraversal.render(**self.d)
        return result

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< ::peano4::grid::GridControlEvent >();\n"

    def get_body_of_prepareTraversal(self):
        return self.d["PREPARE_TRAVERSAL_KERNEL"]

    def get_body_of_unprepareTraversal(self):
        return self.d["UNPREPARE_TRAVERSAL_KERNEL"]

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        result = jinja2.Template(
            """
#include "tarch/multicore/Lock.h"
#include "toolbox/particles/particles.h"
#include "vertexdata/{{LOCAL_PARTICLES_CONTAINER}}.h"
#include "globaldata/{{LOCAL_PARTICLE}}.h"
#include "vertexdata/{{ACTIVE_PARTICLES_CONTAINER}}.h"
#include "globaldata/{{ACTIVE_PARTICLE}}.h"
{{ADDITIONAL_INCLUDES}}
"""
        )
        return result.render(**self.d)

    def get_attributes(self):
        result = jinja2.Template(
            """
  std::list< globaldata::{{ACTIVE_PARTICLE}}* >  _activeParticles;
  std::vector< int >                             _numberOfActiveParticlesAdded;
  std::vector< int >                             _numberOfActiveParticlesPerVertex;
  int                                            _spacetreeId;
"""
        )
        return result.render(**self.d)

    def get_constructor_body(self):
        return """
            _spacetreeId              = treeNumber;
          """
