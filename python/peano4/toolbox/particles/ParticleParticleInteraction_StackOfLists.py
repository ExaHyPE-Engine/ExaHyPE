# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer


class ParticleParticleInteraction_StackOfLists(ActionSet):
    """!

Tree walker to realise particle-particle interactions with particles held by different adjacent cells or on different mesh levels

This code snippet creates a tree walker, i.e. an action set that runs
through the underlying spacetree top down and builds up 
@ref page_toolbox_particles_mesh_storage "the active set and the local set".
These sets can then be used to trigger mesh events. 


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

To exploit this additional information, you can read out _numberOfParticlesPerVertexAdded.
It partitions all of your data.


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

        use_biased_locality_check: Boolean
          When we hit a cell, we have to decide what defines the active set within
          this cell. For this, we can either use peano4::datamanagement::CellMarker::isContained()
          or peano4::datamanagement::CellMarker::isContainedWithBias(). The the
          documentation of these routines for further details.



        """
        super(ParticleParticleInteraction_StackOfLists, self).__init__(
            priority=1, parallel=False
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
  auto& localParticles  = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  auto& activeParticles = fineGridVertex{{ACTIVE_PARTICLES_CONTAINER}};

  {{TOUCH_VERTEX_FIRST_COMPUTE_KERNEL}}
  {% endif %}
"""
    )

    __Template_TouchVertexLastTime = jinja2.Template(
        """
  {% if TOUCH_VERTEX_LAST_COMPUTE_KERNEL!=None %}
  auto& localParticles  = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  auto& activeParticles = fineGridVertex{{ACTIVE_PARTICLES_CONTAINER}};

  {{TOUCH_VERTEX_LAST_COMPUTE_KERNEL}}
  {% endif %}
"""
    )

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  std::list< globaldata::{{LOCAL_PARTICLE}}* >  localParticles;
  _numberOfActiveParticlesAdded.push_back(0);
  for (int i=0; i<TwoPowerD; i++) {
    _numberOfParticlesPerVertexAdded.push_back( fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i).size() );
    _numberOfActiveParticlesAdded.back() += fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i).size();
    for (auto* p: fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i) ) {
      _activeParticles.push_back( p );
    }
    for (auto* p: fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(i) ) {
      bool append = marker.isContained( p->getX() );
      if (append) {
        localParticles.push_front( p );
      }
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
  assertion( _numberOfParticlesPerVertexAdded.size()>=TwoPowerD.back() );
  

  // More elegant way to write
  // -------------------------  
  // for (int i=0; i<_numberOfActiveParticlesAdded.back(); i++) {
  //  _activeParticles.pop_back();
  //}
  _activeParticles.erase( 
    std::prev(_activeParticles.end(),_numberOfActiveParticlesAdded.back() ),
    _activeParticles.end()
  );
    
  _numberOfParticlesPerVertexAdded.erase( 
    std::prev(_numberOfParticlesPerVertexAdded.end(),TwoPowerD ),
    _numberOfParticlesPerVertexAdded.end()
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
  std::vector< int >                             _numberOfParticlesPerVertexAdded;
"""
        )
        return result.render(**self.d)
