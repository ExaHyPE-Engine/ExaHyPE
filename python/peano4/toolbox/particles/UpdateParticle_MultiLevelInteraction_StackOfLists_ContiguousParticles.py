# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer


class UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles(ActionSet):
    """!

    Tree walker to realise particle-particle interactions with particles held by different adjacent cells or on different mesh levels

    This code snippet creates a tree walker, i.e. an action set that runs
    through the underlying spacetree top down and builds up
    @ref page_toolbox_particles_mesh_traversal "the active set and the local set".
    These sets can then be used to trigger mesh events.
    It works if and only if you use continuous storage per vertex as introduced
    with the @ref toolbox_particles_memorypool "memory pools".

    Within touchCellFirstTime(), the routine exposes the following data
    besides the generic information of any action set:

    - ***marker*** The CellMarker is avaialable to a compute kernel and allows you
      to query spatial information. This one is actually one of the generic objects
      which every action set has available, but we highlight that it is there, as
      the @ref page_toolbox_particles_mesh_traversal "discussion on uniqueness for particle-particle interactions"
      explicitly refers to the marker.
    - ***activeParticles*** The list of active particles is a list of pointers. It
      follows the @ref page_toolbox_particles_mesh_storage "generic definition of active sets".
    - ***localParticles*** This is a list of local particles, i.e. particles which
      are tied to the cell's adjacent vertices. While they reside on the cell's
      level, we have no guarantee that they reside within the current cell. They
      might also be contained within its halo of marker.h()/2.

    The additional field _activeParticles is an alias for activeParticles.


    The code is slightly more sophisticated compared than its cousin, the
    UpdateParticle_MultiLevelInteraction_Sets, as it exposes more information:


    - ***numberOfActiveParticlesPerVertex*** This alias for
      _numberOfActiveParticlesPerVertex is a sequence of integers
      which tells you how many particles within activeParticles are added by one
      particular vertex.
    - ***numberOfParticlesPerLocalVertex*** is a sequence of integers which
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

    This is a very efficient implementation yet fails if particles move
    up and down within the tree while we walk. It should work perfectly
    fine if particles do not move. If particles move up and down, the
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
    we had a valid, continous memory for them. So all of our active sets
    are now invalid. Sieves in contrast work, as long as their action
    set is used before the current one. They merely add data to the
    active set that's constructed next.

    - No lifts.
    - No drops.
    - Sieves are fine as long as they happen before touchVertexFirstTime()
      of this action set.
    - Lifts into a global sieve list are fine, as long as they happen in
      touchVertexLastTime() called after this action set.

    In the case of a dynamic setup, i.e. in combinatino with the action
    set UpdateParticleGridAssociation, you might be better of with using
    the set-based implementation of the particle-particle interaction.



    ## Further plug-in points

    Besides the cell kernel which is there to realise particle-to-particle
    interactions, we also have a vertex kernel which we call whenever a
    vertex is loaded for the first time. That is, you can assume that the
    vertex kernel has been launched for all @f$ 2^d @f$ vertices of a cell before
    its cell kernel is triggered. The vertex kernel is also passed on the
    active set (from the coarser level). Its local set is all the particles
    whose centre lies within the square/cube around the vertex with mesh size
    h. So this area goes h/2 along each each coordinate axis into the
    neighbouring cells.


    ## Flattening of particle arrays

    The routine not only keeps track of how many particles we add per iteration.
    It also keeps track of the number of particles per vertex that have been
    added. If you know that you work with a @ref page_toolbox_particles_mesh_storage "memory pool", i.e. a consecutive
    storage of particles, you can use this additional meta information within
    your kernel invocations to work with loops over consecutive arrays (AoS)
    rather than sole link lists. You can exploit that you know something
    about individual chunks indexed by the pointers.

    To exploit this additional information, you cannot use localParticles anymore,
    i.e. you have to use localParticles. This means, you
    might have to check if a particle is contained within a cell manually.
    Once you rewrite your kernels that way, you can exploit the integers
    _numberOfActiveParticlesPerVertex and numberOfParticlesPerLocalVertex:

    @image html UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles_continuous-memory.png

    Assume you enter a cell and you get the localParticles
    list hosting 19 pointers. Further to that, you receive the array
    numberOfParticlesPerLocalVertex with four integer entries. You know that the
    first five entries point to one continuous chunk of particles in memory, the
    next seven do so as well, and so forth. This holds if and only if you work
    with a memory pool which maintains vertex-local or globally continuous
    particles.

    I do recommend to call toolbox::particles::ensureThatParticleListsEncodeContinuousMemoryChunks()
    for your data to ensure that the memory layout above is preserved. Simply
    add the calls

    ~~~~~~~~~~~~~~~~~~~~~~~~~~
    ::toolbox::particles::ensureThatParticleListsEncodeContinuousMemoryChunks(
      activeParticles,
      _numberOfActiveParticlesPerVertex
    );
    ::toolbox::particles::ensureThatParticleListsEncodeContinuousMemoryChunks(
      localParticles,
      numberOfParticlesPerLocalVertex
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

    for (auto localParticlesChunkSize: numberOfParticlesPerLocalVertex) {
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

    for (auto localParticlesChunkSize: numberOfParticlesPerLocalVertex) {
      typename ParticleContainer::value_type  localParticlesChunk = *localParticlesIterator;
      std::advance( localParticlesIterator, localParticlesChunkSize );

      typename std::list< typename ParticleContainer::value_type >::const_iterator activeParticlesIterator = activeParticles.begin();
      for (auto activeParticlesChunkSize: numberOfActiveParticlesPerVertex) {
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

    This inner loop is where the magic happens, i.e. where we can exploit the
    fact that localParticlesChunk and activeParticlesChunk both point to coninuous
    memory regions. This means that computeInnerLoop is structurally simple:


    ~~~~~~~~~~~~~~~~~~~~~~~~~~
    void computeHydroForceInnerLoop(
        const peano4::datamanagement::CellMarker&  marker,
        Particle*                                  particlesAssociatedWithLocalVertex, // das ist falsch
        int                                        numberOfLocalParticles,
        Particle*                                  activeParticles,
        int                                        numberOfActiveParticles
    ) {
      for (int activeParticleNumberInThisChunk=0; activeParticleNumberInThisChunk<numberOfActiveParticles; activeParticleNumberInThisChunk++) {
        for (int localParticleNumberInThisChunk=0; localParticleNumberInThisChunk<numberOfLocalParticles; localParticleNumberInThisChunk++) {
          // read from activeParticles[activeParticleNumberInThisChunk]
          // write to particlesAssociatedWithLocalVertex[localParticleNumberInThisChunk]
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
    assume that our pointers are all ov the place and do not point to continuous
    data anymore. Even worse, we know that previously built-up active sets might be
    invalid. We have built them up top-down, but when we drop data out of the
    active set, they change on-the-fly.



    ## Optimising coalesced compute kernels

    Compute kernels can be optimised quite aggressively, but fiddling out how they
    can be optimised is tedious. I recommend to use MAQAO and Intel's compiler
    optimisation feedback. The page @ref toolbox_particles_memorypool holds
    additional information. It is also worth studying @ref page_swift_performance_optimisation "Swift's performance optimisation"
    remarks with respect to vecorisation as well as Swift's discussion of @ref swift_particle_lifecycle Particle Lifecycle "vectorised kernels".


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

        Initialise object

        ## Arguments

        particle_set: ParticleSet

        particle_particle_interaction_kernel: String holding C++ code

        active_particle_set: ParticleSet or None
          You can compare different particles species with this argument. It
          allows you to make the active particles stem from another species than
          the local ones that you actually update. Pick None if both sets are of
          the same type.

        touch_vertex_first_time_compute_particle_update_kernel: String or None
          Can be empty, but if you wanna move particles, then a minimal example
          string passed equals

              for (auto* localParticle: localParticles ) {
                localParticle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );
              }

          i.e. you will have a variable localParticle available in this kernel
          and it is a pointer.


        """
        super(
            UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles, self
        ).__init__(descend_invocation_order=1, parallel=False)

        self._particle_set = particle_set
        self.d = {}
        self.d["LOCAL_PARTICLE"] = particle_set.particle_model.name
        self.d["LOCAL_PARTICLES_CONTAINER"] = particle_set.name
        if active_particle_set == None:
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
  int  numberOfAssignedParticles = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}}.isGathered() ? fineGridVertex{{LOCAL_PARTICLES_CONTAINER}}.size() : 0;

  {{TOUCH_VERTEX_FIRST_COMPUTE_KERNEL}}
  {% endif %}
"""
    )

    __Template_TouchVertexLastTime = jinja2.Template(
        """
  {% if TOUCH_VERTEX_LAST_COMPUTE_KERNEL!=None %}
  auto& assignedParticles  = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  int  numberOfAssignedParticles = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}}.isGathered() ? fineGridVertex{{LOCAL_PARTICLES_CONTAINER}}.size() : 0;

  {{TOUCH_VERTEX_LAST_COMPUTE_KERNEL}}
  {% endif %}
"""
    )

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  std::list< globaldata::{{LOCAL_PARTICLE}}* >  localParticles;
  std::vector< int >                            numberOfParticlesPerLocalVertex( TwoPowerD );
  
  _numberOfActiveParticlesAdded.push_back(0);
  for (int i=0; i<TwoPowerD; i++) {
    // Keep track of number of particles
    _numberOfActiveParticlesPerVertex.push_back( fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i).size() );
    _numberOfActiveParticlesAdded.back() += fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i).size();
    numberOfParticlesPerLocalVertex[i] = fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i).size();
    
    // Add particles to active particle set
    for (auto* p: fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i) ) {
      _activeParticles.push_back( p );
    }
    
    // Construct the two local particle sets
    for (auto* p: fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(i) ) {
      localParticles.push_back( p );
    }
  }
  
  logDebug( "touchCellFirstTime(...)", "size of local/active particles=" << localParticles.size() << "/" << _activeParticles.size() << " at " << marker.toString() );

  {% if PARTICLE_CELL_UPDATE_KERNEL!=None %}
  std::list< globaldata::{{ACTIVE_PARTICLE}}* >&  activeParticles = _activeParticles;
  {{PARTICLE_PARTICLE_INTERACTION_KERNEL}}
  {% endif %}
"""
    )

    __Template_TouchCellLastTime = jinja2.Template(
        """
  assertion( not _numberOfActiveParticlesAdded.empty() );
  assertion( _activeParticles.size()>=_numberOfActiveParticlesAdded.back() );
  assertion( _numberOfActiveParticlesPerVertex.size()>=TwoPowerD );
  

  // More elegant way to write
  // -------------------------  
  // for (int i=0; i<_numberOfActiveParticlesAdded.back(); i++) {
  //  _activeParticles.pop_back();
  //}
  _activeParticles.erase( 
    std::prev(_activeParticles.end(),_numberOfActiveParticlesAdded.back() ),
    _activeParticles.end()
  );
    
  _numberOfActiveParticlesPerVertex.erase( 
    std::prev(_numberOfActiveParticlesPerVertex.end(),TwoPowerD ),
    _numberOfActiveParticlesPerVertex.end()
  );
  
  _numberOfActiveParticlesAdded.pop_back();
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  assertion1( _activeParticles.empty(), (*_activeParticles.begin())->toString() );
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        # if operation_name==ActionSet.OPERATION_BEGIN_TRAVERSAL:
        #  result = self.__Template_BeginTraversal.render(**self.d)
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
        return "  return std::vector< peano4::grid::GridControlEvent >();\n"

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

#include <list>
#include <vector>
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
