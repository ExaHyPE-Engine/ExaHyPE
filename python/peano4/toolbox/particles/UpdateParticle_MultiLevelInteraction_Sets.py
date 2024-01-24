# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel.DaStGen2

import dastgen2.attributes.Integer


class UpdateParticle_MultiLevelInteraction_Sets(ActionSet):
    """!

    Tree walker to realise particle-particle interactions which span multiple levels.

    This code has the same semantics as peano4.toolbox.particles.UpdateParticle_MultiLevelInteraction_StackOfLists.
    However, it uses a different container to maintain the active and local sets. Please consult the
    documentation of UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles for a description
    of the arguments, plug-in points and the semantics of the two sets. Before you
    do so, read throuth @ref page_toolbox_particles_mesh_traversal "the generic set discussion"
    for the overall context.

    As this class uses a ***set*** to maintain the active set and local set, the
    tree walker works even if your particles change position and mesh level while
    you traverse the mesh.


    ## Realisation

    Both the active set and the local set are realised as proper sets over
    pointers. The sets are updated in touchCellFirstTime() and touchCellLastTime(),
    i.e. when we walk down the tree or up again:

    In touchCellFirstTime(), i.e. while we go from coarse to fine, we first
    look at all the particles which are associated to an adjacent vertex of
    the cell and are contained within the cell itself. These particles form
    the local set.

    Furthermore, we take the active set from coarser levels, and we add all
    the particles which are associated with an adjacent vertex of the current
    cell. If particles more from coarse to fine while we walk through the tree,
    we encounter one out of two situations:
    - If they are sieved within "our" current tree, we add them twice to the
      active set. Once on the coarser level and then again on the finer one
      into which they are sieved. For the present tree walker, this does not
      matter, as we work with a set of pointers. The particle might be added
      "too early", but it is not added multiple times to the active set.
    - If they are sieved within another tree eventually, we might add it to
      the active set even though they should be sieved down into another
      branch of the tree. Should not make a difference.

    In touchCellLastTime(), we take all the particles which are associated to
    any adjacent vertex of the current cell, and we remove those particles
    from the active set.


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
        super(UpdateParticle_MultiLevelInteraction_Sets, self).__init__()
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
        self.d["PREPARE_TRAVERSAL_KERNEL"] = prepare_traversal_kernel
        self.d["UNPREPARE_TRAVERSAL_KERNEL"] = unprepare_traversal_kernel

        self.d["ADDITIONAL_INCLUDES"] = additional_includes

        self.d["SET_IMPLEMENTATION"] = "std::unordered_set"

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
  {{SET_IMPLEMENTATION}}< globaldata::{{LOCAL_PARTICLE}}* >  localParticles;
  for (int i=0; i<TwoPowerD; i++) {
    for (auto* p: fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i) ) {
      _activeParticles.insert( p );
    }
    for (auto* p: fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(i) ) {
      localParticles.insert( p );
    }
  }

  {% if PARTICLE_CELL_UPDATE_KERNEL!=None %}
  {{SET_IMPLEMENTATION}}< globaldata::{{ACTIVE_PARTICLE}}* >&  activeParticles = _activeParticles;
  {{PARTICLE_PARTICLE_INTERACTION_KERNEL}}
  {% endif %}
"""
    )

    __Template_TouchCellLastTime = jinja2.Template(
        """
  for (int i=0; i<TwoPowerD; i++) {
    for (auto* p: fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(i) ) {
      _activeParticles.erase( p );
    }
  }
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

#include <unordered_set>
#include <set>
#include <vector>
"""
        )
        return result.render(**self.d)

    def get_attributes(self):
        result = jinja2.Template(
            """
  {{SET_IMPLEMENTATION}}< globaldata::{{ACTIVE_PARTICLE}}* >  _activeParticles;
  int  _spacetreeId;
"""
        )
        return result.render(**self.d)

    def get_constructor_body(self):
        return """
            _spacetreeId              = treeNumber;
          """
