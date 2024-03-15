# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class UpdateParticle_SingleLevelInteraction_ContiguousParticles(ActionSet):
    def __init__(
        self,
        particle_set,
        particle_cell_update_kernel=None,
        touch_vertex_first_time_compute_particle_update_kernel=None,
        touch_vertex_last_time_compute_particle_update_kernel=None,
        prepare_traversal_kernel="",
        unprepare_traversal_kernel="",
        additional_includes="",
        active_particle_set=None,
    ):
        """!

        Update particles without non-local interaction

        This action set is very similar to UpdateParticle_SingleLevelInteraction,
        but it assumes that data are stored @ref toolbox_particles_memorypool "continuously in memory".
        Therefore, we can provide meta information how many particles we have per
        vertex stored en bloc.

        Per cell, there's a @f$ 2^d @f$ vector over integers called
        numberOfActiveParticles. It encodes how many particles are contributed
        towards the active set from each vertex. If you employ contiguous particle
        storage schemes, you can loop over this integer range rather than the
        particle set, and you can exploit the fact that data within the index set
        is contigous.

        A cell update then resembles

        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        auto localParticlesIterator = activeParticles.begin();
        // There is some parallelism here if you wanna exploit it
        for (int vertex=0; vertex<TwoPowerD; vertex++) {
          auto* firstParticle = *localParticlesIterator;
          std::advance( localParticlesIterator, numberOfActiveParticles[vertex] );
          // This loop encodes vector-level parallelism (SIMD)
          for (int currentParticleNo = 0; currentParticleNo < numberOfActiveParticles[vertex]; currentParticleNo++) {
             doSomethingFancy( firstParticle+currentParticleNo );
          }
        }
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~


        A vertex update resembles
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        auto* firstParticle = *localParticles.begin();
        // This loop encodes vector-level parallelism (SIMD)
        for (int currentParticleNo = 0; currentParticleNo < numberOfActiveParticles; currentParticleNo++) {
           doSomethingFancy( firstParticle+currentParticleNo );
        }
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        ## Validity if particles change their position of cut-off radius

        If particles change their position or cut-off radius, you have to resort
        them. See also UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles
        for a discussion of the data validity.


        """
        super(UpdateParticle_SingleLevelInteraction_ContiguousParticles, self).__init__(
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
        self.d["PARTICLE_CELL_UPDATE_KERNEL"] = particle_cell_update_kernel
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
  {% if PARTICLE_CELL_UPDATE_KERNEL!=None %}
  std::list< globaldata::{{LOCAL_PARTICLE}}* >   localParticles;
  std::list< globaldata::{{ACTIVE_PARTICLE}}* >  activeParticles;
  std::vector< int >                             numberOfActiveParticlesPerVertex( TwoPowerD );
  
  for (int i=0; i<TwoPowerD; i++) {
    numberOfActiveParticlesPerVertex[i] = fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i).size();
    for (auto* p: fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i) ) {
      activeParticles.push_front( p );
    }
    for (auto* p: fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(i) ) {
      localParticles.push_front( p );
    }
  }

  {{PARTICLE_CELL_UPDATE_KERNEL}}
  {% endif %}
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCellFirstTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self.__Template_TouchVertexFirstTime.render(**self.d)
        if operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_TouchVertexLastTime.render(**self.d)
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
#include "toolbox/particles/particles.h"
#include "vertexdata/{{LOCAL_PARTICLES_CONTAINER}}.h"
#include "globaldata/{{LOCAL_PARTICLE}}.h"
#include "vertexdata/{{ACTIVE_PARTICLES_CONTAINER}}.h"
#include "globaldata/{{ACTIVE_PARTICLE}}.h"

{{ADDITIONAL_INCLUDES}}

#include <list>
"""
        )
        return result.render(**self.d)

    def get_attributes(self):
        return """
  int  _spacetreeId;
"""

    def get_constructor_body(self):
        return """
            _spacetreeId              = treeNumber;
          """
