# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer


class UpdateParticle_MultiLevelInteraction_StackOfLists(ActionSet):
    """!

    Tree walker to realise particle-particle interactions with particles held by different adjacent cells or on different mesh levels

    This code snippet creates a tree walker, i.e. an action set that runs
    through the underlying spacetree top down and builds up
    @ref page_toolbox_particles_mesh_storage "the active set and the local set".
    These sets can then be used to trigger mesh events.

    This version of the interactions makes no assumptions on how the particles
    are stored. Hence, it cannot expose any such information. If you want to
    offer information about consecutive storage, e.g., please use
    UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles in combination
    with @ref toolbox_particles_memorypool "the memory pool".


    The code is slightly more sophisticated compared to the set-based tree
    traversal approach, as it exposes more information:


    - ***marker*** The CellMarker is avaialable to a compute kernel and allows you
      to query spatial information.
    - ***activeParticles*** The list of active particles is a list of pointers. It
      follows the @ref page_toolbox_particles_mesh_storage "generic definition of active sets".
    - ***localParticles*** This is a list of local particles, i.e. particles within
      the cell which are tied to the cell's adjacent vertices. While they reside on
      the cell's level, we cannot be sure that they are not also local to a
      neighbouring octant within the spacetree, as we work with floating point
      arithmetics and hence have no unique particle-cell assocation. In any case,
      marker.isContained() holds for all particles within this set.
    - ***particlesAssociatedWithLocalVertices*** This additional list holds all
      particles that are associated with adjacent vertices of the current cell.
      This is a superset of localParticles and also holds particles for which
      marker.isContained() does not hold.
    - ***_numberOfActiveParticlesPerVertex*** is a sequence of integers
      which tells you how many particles within activeParticles are added by one
      particular vertex.
    - ***numberOfParticlesPerLocalVertex*** is a sequence of integers which
      tells you how many particles within particlesAssociatedWithLocalVertices are
      added by each adjacent vertex.




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
    I am also not 100% sure if we might run into memory issues if particles
    suddenly leave cells and hence "disappear" from the active sets.

    In the case of a dynamic setup, i.e. in combination with the action
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
    i.e. you have to use particlesAssociatedWithLocalVertices. This means, you
    might have to check if a particle is contained within a cell manually.
    Once you rewrite your kernels that way, you can exploit the integers
    _numberOfActiveParticlesPerVertex and numberOfParticlesPerLocalVertex:

    @image html UpdateParticle_MultiLevelInteraction_StackOfLists_continuous-memory.png

    Assume you enter a cell and you get the particlesAssociatedWithLocalVertices
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
      particlesAssociatedWithLocalVertices,
      numberOfParticlesPerLocalVertex
    );
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    to your cell-wise compute kernel.

    The actual compute kernels then typically look similar to the following code
    snippet:


    ~~~~~~~~~~~~~~~~~~~~~~~~~~
    typename std::list<T*>::const_iterator localParticlesIterator  = particlesAssociatedWithLocalVertices.begin();

    for (auto localParticlesChunkSize: numberOfParticlesPerLocalVertex) {
      T*  localParticlesChunk = *localParticlesIterator;
      std::advance( localParticlesIterator, localParticlesChunkSize );

      for (int localParticleNumberInThisChunk=0; localParticleNumberInThisChunk<localParticlesChunkSize; localParticleNumberInThisChunk++) {
        if (
          not localParticle->getCellHasUpdatedParticle()
          and
          marker.isContained(localParticle->getX())
          and
          not toolbox::particles::particleWillBeDroppedFurther( *localParticle, marker)
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
    typename std::list<typename ParticleContainer::value_type>::const_iterator localParticlesIterator
      = particlesAssociatedWithLocalVertices.begin();

    for (auto localParticlesChunkSize : numberOfParticlesPerLocalVertex) {
      typename ParticleContainer::value_type localParticlesChunk = *localParticlesIterator;
      std::advance(localParticlesIterator, localParticlesChunkSize);

      typename std::list<typename ParticleContainer::value_type>::const_iterator activeParticlesIterator
        = activeParticles.begin();
      for (auto activeParticlesChunkSize : numberOfActiveParticlesPerVertexAdded) {
        typename ParticleContainer::value_type activeParticlesChunk = *activeParticlesIterator;
        std::advance(activeParticlesIterator, activeParticlesChunkSize);

        for (int localParticleNumberInThisChunk = 0; localParticleNumberInThisChunk < localParticlesChunkSize;
             localParticleNumberInThisChunk++) {
          if (
            marker.isContained(localParticlesChunk[localParticleNumberInThisChunk].getX())
            and
            not localParticlesChunk[localParticleNumberInThisChunk].getCellHasUpdatedParticle()
            and
            not toolbox::particles::particleWillBeDroppedFurther( localParticlesChunk[localParticleNumberInThisChunk], marker)
          ) {
            for (int activeParticleNumberInThisChunk = 0; activeParticleNumberInThisChunk < activeParticlesChunkSize; activeParticleNumberInThisChunk++) {
                p2pEvaluation(
                  localParticlesChunk + localParticleNumberInThisChunk,
                  activeParticlesChunk + activeParticleNumberInThisChunk
                );
            }
          }
        }
      }
    }
    ~~~~~~~~~~~~~~~~~~~~~~~~~~

    This inner loop is where the magic happens, i.e. where we can exploit the
    fact that localParticlesChunk and activeParticlesChunk both point to coninuous
    memory regions.


    ## Optimising coalesced compute kernels

    Compute kernels can be optimised quite aggressively, but fiddling out how they
    can be optimised is tedious. I recommend to use MAQAO and Intel's compiler
    optimisation feedback. Here are some lessons learned:

    - You cannot collapse the two for loops. That would lead to a scattered
      iteration space in memory and hence make any vectorisation impossible.
    - The active loop has to be the inner one. I do not understand why, but
      Intel's icpx refuses to vectorise if the local loop is the inner one.
    - The compiler has to be able to inline. According to https://clang.llvm.org/docs/AttributeReference.html#always-inline-force-inline
      this is something one should be able to control via the annotation

              [[clang::always_inline]]

      but this led to multiple compiler crashes (again with Intel). To some
      degree this makes sense: The function has to be in the header! I played
      around with ipo, but that did not resolve the fundamental issues.
    - You might have to recursively study all inlined methods and make the
      stuff they use inline as well.
    - Typically, you have to embed the inner-most loop into a statement alike
      ~~~~~~~~~~~~~~~~~~~~~~~
        if (
          marker.isContained(particlesAssociatedWithLocalVertex[localParticleNumberInThisChunk].getX())
          and
          not particlesAssociatedWithLocalVertex[localParticleNumberInThisChunk].getCellHasUpdatedParticle()
        ) {
          // read from activeParticles[activeParticleNumberInThisChunk]
          // write to particlesAssociatedWithLocalVertex[localParticleNumberInThisChunk]
        }
      ~~~~~~~~~~~~~~~~~~~~~~~

    The @ref page_peano_performance_optimisation "generic Peano optimisation" page
    provides some useful hints how to guide these optimisation through the
    compiler. It also makes sense to study swift2::kernels::computeHydroForce()
    which was one of the first routines that we optimised along these ideas.


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
          This C++ code can access three different types of variables: There's
          a list of particles called activeParticles, there's a list of particles
          called localParticles, and there's the cell marker. See the guidebook
          for further info. A typical kernel resembles

              for (auto* localParticle: localParticles )
              for (auto* activeParticle: activeParticles ) {
                localParticle->doSomethingFancy( activeParticle );
              }

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
        super(UpdateParticle_MultiLevelInteraction_StackOfLists, self).__init__(
            descend_invocation_order=1, parallel=False
        )

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

  {{TOUCH_VERTEX_FIRST_COMPUTE_KERNEL}}
  {% endif %}
"""
    )

    __Template_TouchVertexLastTime = jinja2.Template(
        """
  {% if TOUCH_VERTEX_LAST_COMPUTE_KERNEL!=None %}
  auto& assignedParticles  = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};

  {{TOUCH_VERTEX_LAST_COMPUTE_KERNEL}}
  {% endif %}
"""
    )

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  std::list< globaldata::{{LOCAL_PARTICLE}}* >  localParticles;
  std::list< globaldata::{{LOCAL_PARTICLE}}* >  particlesAssociatedWithLocalVertices;
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
      bool append = marker.isContained( p->getX() );
      if (append) {
        localParticles.push_back( p );
      }
      particlesAssociatedWithLocalVertices.push_back( p );
    }
  }
  
  logDebug( "touchCellFirstTime(...)", "size of local/active particles=" << localParticles.size() << "/" << _activeParticles.size() << " at " << marker.toString() );

  {% if PARTICLE_CELL_UPDATE_KERNEL!=None %}
  std::list< globaldata::{{ACTIVE_PARTICLE}}* >&  activeParticles                  = _activeParticles;
  std::vector< int >&                             numberOfActiveParticlesPerVertex = _numberOfActiveParticlesPerVertex;
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
