# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class UpdateParticle_SingleLevelInteraction(ActionSet):
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

        Update particles on a single level

        To be used if you want to update particles within their cell, while they
        either

        - do not interact at all; or
        - interact only with particles on the same mesh level.

        This code snippet creates a tree walker, i.e. an action set that runs
        through the underlying spacetree top-down. Users can plug into what
        happens per cell and what happens when we touch a set of particles for
        the first and for the last time.

        ## Per cell updates

        Per tree node (cell) there are two different sets.
        The set of local particles of a cell are all of the particles whose
        centre is contained within the cell.
        The set of active particles are all particles which are associated
        with an adjacent vertex of the cell.
        In line with @ref page_toolbox_particles_mesh_storage "Peano's particle storage scheme",
        this effectively means that the active particles are those from the
        cell plus its h/2 environment.

        If the active_particle_set attribute is None, then
        the active set and the local set are of the same type. They are
        however not the same sets, as one contains only the particles held
        within the cell.
        You can set the active set to another species of particles
        and thus couple two types of particles.

        The local set is a strict subset of the active set per cell. The code has
        not checked beforehand if its particles overlap with the current cell.


        ## Per vertex updates

        Besides the cell kernel which is there to realise per-cell updates,
        we also have a vertex kernel which we call whenever a
        vertex is loaded for the first time. That is, you can assume that the
        vertex kernel has been launched for all 2^d vertices of a cell before
        its cell kernel is triggered. The vertex kernel has access to a local
        set: These sets contain all the particles
        whose centre lies within the square/cube around the vertex with mesh size
        2h. So this area goes h along each each coordinate axis into the
        neighbouring cells. The vertex's local particles also have to reside
        on the same resolution level.


        ## Type of updates

        - You can use this action set to realise particle updates without any
          dependencies on other particles.
        - You can use this action set to realise particle updates where the
          particle depends on other particles which are stored on the same
          level.

        The following constraints apply:

        - If you work with the local set within the cell, you are on the safe
          side, though the association of particles to cells is not unique.
          There is a tiny overlap between cells, i.e. particles on the face
          between two cells might be associated to both cells. If you update
          particles, you should employ a boolean flag to memorise if a particle
          has been updated already or not.
        - If you work with the local set within a vertex, this local set overlaps
          with adjacent cells (see previous item), and it might even overlap with
          a remote subdomain.
        - If you change a particle position while the action set runs, you might
          end up with inconsistent data view.


        ## Validity if particles change their position of cut-off radius

        This particular action set constructs its active and local sets totally
        on-the-fly. It does not maintain any data while is marches through the
        mesh. Consequently, it is robust w.r.t. particle position updates or
        lift and drops and in general any resorting.


        ## Arguments

        @param particle_set: ParticleSet

        @param particle_particle_interaction_kernel: String holding C++ code
          This C++ code can access three different types of variables: There's
          a list of particles called activeParticles, there's a list of particles
          called localParticles, and there's the cell marker. See the guidebook
          for further info. A typical kernel resembles

          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
              for (auto* localParticle: localParticles )
              for (auto* activeParticle: activeParticles ) {
                localParticle->doSomethingFancy( activeParticle );
              }
          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        @param active_particle_set: ParticleSet or None
          You can compare different particles species with this argument. It
          allows you to make the active particles stem from another species than
          the local ones that you actually update. Pick None if both sets are of
          the same type.

        @param touch_vertex_first_time_compute_particle_update_kernel: String or None
          Can be empty, but if you wanna move particles, then a minimal example
          string passed equals

          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
              for (auto* localParticle: localParticles ) {
                localParticle->setMoveState( globaldata::Particle::MoveState::NotMovedYet );
              }
          ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

          i.e. you will have a variable localParticle available in this kernel
          and it is a pointer. There is no notion of an active variable for
          touch first or touch last.


        """
        super(UpdateParticle_SingleLevelInteraction, self).__init__(
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
  {% if PARTICLE_CELL_UPDATE_KERNEL!=None %}
  std::list< globaldata::{{LOCAL_PARTICLE}}* >   localParticles;
  std::list< globaldata::{{ACTIVE_PARTICLE}}* >  activeParticles;

  for (int i=0; i<TwoPowerD; i++) {
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
