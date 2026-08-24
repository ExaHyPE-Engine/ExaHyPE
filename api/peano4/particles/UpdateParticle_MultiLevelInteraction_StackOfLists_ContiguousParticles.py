# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

from peano4.solversteps.ActionSet import ActionSet


class UpdateParticle_MultiLevelInteraction_StackOfLists_ContiguousParticles(ActionSet):
    """Runs multilevel interactions over contiguous particle chunks.

    The action builds the active list on descent by appending one range for
    each adjacent vertex and removes these ranges on ascent. It exposes the
    range lengths through numberOfCoalescedActiveParticlesPerVertex and
    numberOfCoalescedLocalParticlesPerVertex. These lengths identify
    contiguous chunks only when each vertex container is gathered; use this
    action with the memory pool described by @ref toolbox_particles_memorypool.

    The cell kernel receives marker, activeParticles, localParticles,
    localParticleIsContainedInCell, and both range-length vectors. The local
    list covers all adjacent vertices, while the aligned Boolean vector marks
    the particles owned by the current cell. Vertex kernels receive
    assignedParticles and numberOfCoalescedAssignedParticles. A value of -1
    in any length vector means that the corresponding container is not
    gathered.

    Particle reassociation during this action invalidates active pointers and
    chunk boundaries. Drops and local lifts must therefore run in another
    traversal. Sieving may precede touchVertexFirstTime(), and global lifts
    may follow touchVertexLastTime(). A caller sets active_particle_set to
    read a second particle species while updating the local species.
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
        super().__init__(descend_invocation_order=1)
        self._dictionary = {}
        self._dictionary["LOCAL_PARTICLE"] = particle_set.particle_model.name
        self._dictionary["LOCAL_PARTICLES_CONTAINER"] = particle_set.name
        if active_particle_set is None:
            self._dictionary["ACTIVE_PARTICLE"] = particle_set.particle_model.name
            self._dictionary["ACTIVE_PARTICLES_CONTAINER"] = particle_set.name
        else:
            self._dictionary["ACTIVE_PARTICLE"] = (
                active_particle_set.particle_model.name
            )
            self._dictionary["ACTIVE_PARTICLES_CONTAINER"] = active_particle_set.name
        self._dictionary["PARTICLE_PARTICLE_INTERACTION_KERNEL"] = (
            particle_particle_interaction_kernel
        )
        self._dictionary["TOUCH_VERTEX_FIRST_COMPUTE_KERNEL"] = (
            touch_vertex_first_time_compute_particle_update_kernel
        )
        self._dictionary["TOUCH_VERTEX_LAST_COMPUTE_KERNEL"] = (
            touch_vertex_last_time_compute_particle_update_kernel
        )
        self._dictionary["ADDITIONAL_INCLUDES"] = additional_includes
        self._dictionary["PREPARE_TRAVERSAL_KERNEL"] = prepare_traversal_kernel
        self._dictionary["UNPREPARE_TRAVERSAL_KERNEL"] = unprepare_traversal_kernel

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
{% if TOUCH_VERTEX_FIRST_COMPUTE_KERNEL is not none %}
  auto& assignedParticles = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  [[maybe_unused]] const int numberOfCoalescedAssignedParticles =
    assignedParticles.isGathered() ? assignedParticles.size() : -1;

  {{TOUCH_VERTEX_FIRST_COMPUTE_KERNEL}}
{% endif %}
"""
    )

    __Template_TouchVertexLastTime = jinja2.Template(
        """
{% if TOUCH_VERTEX_LAST_COMPUTE_KERNEL is not none %}
  auto& assignedParticles = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  [[maybe_unused]] const int numberOfCoalescedAssignedParticles =
    assignedParticles.isGathered() ? assignedParticles.size() : -1;

  {{TOUCH_VERTEX_LAST_COMPUTE_KERNEL}}
{% endif %}
"""
    )

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  std::list<globaldata::{{LOCAL_PARTICLE}}*> localParticles;
  std::vector<int>                           numberOfCoalescedLocalParticlesPerVertex(TWO_POWER_D);
  std::vector<bool>                          localParticleIsContainedInCell;

  _numberOfActiveParticlesAdded.push_back(0);
  [[maybe_unused]] const auto ownershipTolerance = toolbox::particles::GrabOwnershipSpatialSortingTolerance
    * tarch::la::max(marker.getH()) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
  for (int vertex = 0; vertex < TWO_POWER_D; vertex++) {
    const auto& activeVertexParticles   = fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(vertex);
    const auto  numberOfActiveParticles = activeVertexParticles.size();
    _numberOfActiveParticlesPerVertex.push_back(
      activeVertexParticles.isGathered() ? numberOfActiveParticles : -1
    );
    _numberOfActiveParticlesAdded.back() += numberOfActiveParticles;

    const auto& localVertexParticles = fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(vertex);
    numberOfCoalescedLocalParticlesPerVertex[vertex] =
      localVertexParticles.isGathered() ? localVertexParticles.size() : -1;

    for (auto* const particle : activeVertexParticles) {
      _activeParticles.push_back(particle);
    }
    for (auto* const particle : localVertexParticles) {
      std::bitset<TWO_POWER_D> adjacentCellMask{};
      adjacentCellMask.set(TWO_POWER_D - vertex - 1);
      const bool isLocal = (adjacentCellMask & particle->getContainedInAdjacentCell()).any();
      assertion(
        isLocal == marker.isContained(particle->getX(), ownershipTolerance),
        isLocal,
        marker.isContained(particle->getX(), ownershipTolerance),
        marker.toString(),
        particle->toString(),
        vertex,
        adjacentCellMask
      );
      localParticleIsContainedInCell.push_back(isLocal);
      localParticles.push_back(particle);
    }
  }

  logDebug(
    "Local/active particle counts: " << localParticles.size() << "/"
                                     << _activeParticles.size() << " at " << marker.toString() << "."
  );

{% if PARTICLE_PARTICLE_INTERACTION_KERNEL is not none %}
  auto& activeParticles                           = _activeParticles;
  auto& numberOfCoalescedActiveParticlesPerVertex = _numberOfActiveParticlesPerVertex;
  assertion(
    not numberOfCoalescedActiveParticlesPerVertex.empty()
      and numberOfCoalescedActiveParticlesPerVertex.size() % TWO_POWER_D == 0,
    numberOfCoalescedActiveParticlesPerVertex.size()
  );
  {{PARTICLE_PARTICLE_INTERACTION_KERNEL}}

  // Interaction kernels may move particles and invalidate their cached cell ownership.
  for (int vertex = 0; vertex < TWO_POWER_D; vertex++) {
    const std::bitset<DIMENSIONS> assignedVertex(vertex);
    for (auto* const particle : fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(vertex)) {
      toolbox::particles::updateContainedInAdjacentCellProperty(marker, assignedVertex, *particle);
    }
  }
{% endif %}
"""
    )

    __Template_TouchCellLastTime = jinja2.Template(
        """
  assertion(not _numberOfActiveParticlesAdded.empty());
  assertion(_activeParticles.size() >= _numberOfActiveParticlesAdded.back());
  assertion(_numberOfActiveParticlesPerVertex.size() >= TWO_POWER_D);

  _activeParticles.erase(
    std::prev(_activeParticles.end(), _numberOfActiveParticlesAdded.back()),
    _activeParticles.end()
  );
  _numberOfActiveParticlesPerVertex.erase(
    std::prev(_numberOfActiveParticlesPerVertex.end(), TWO_POWER_D),
    _numberOfActiveParticlesPerVertex.end()
  );
  _numberOfActiveParticlesAdded.pop_back();
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  assertion(_activeParticles.empty(), (*_activeParticles.begin())->toString());
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCellFirstTime.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.__Template_TouchCellLastTime.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self.__Template_TouchVertexFirstTime.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_TouchVertexLastTime.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self.__Template_EndTraversal.render(**self._dictionary)
        return result

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector<peano4::grid::GridControlEvent>{};\n"

    def get_body_of_prepareTraversal(self):
        return self._dictionary["PREPARE_TRAVERSAL_KERNEL"]

    def get_body_of_unprepareTraversal(self):
        return self._dictionary["UNPREPARE_TRAVERSAL_KERNEL"]

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return jinja2.Template(
            """
#include "globaldata/{{LOCAL_PARTICLE}}.h"
{% if ACTIVE_PARTICLE != LOCAL_PARTICLE %}
#include "globaldata/{{ACTIVE_PARTICLE}}.h"
{% endif %}
#include "toolbox/particles/particles.h"
#include "vertexdata/{{LOCAL_PARTICLES_CONTAINER}}.h"
{% if ACTIVE_PARTICLES_CONTAINER != LOCAL_PARTICLES_CONTAINER %}
#include "vertexdata/{{ACTIVE_PARTICLES_CONTAINER}}.h"
{% endif %}

{{ADDITIONAL_INCLUDES}}
"""
        ).render(**self._dictionary)

    def get_attributes(self):
        return jinja2.Template(
            """
  std::list<globaldata::{{ACTIVE_PARTICLE}}*> _activeParticles;
  std::vector<int> _numberOfActiveParticlesAdded;
  std::vector<int> _numberOfActiveParticlesPerVertex;
"""
        ).render(**self._dictionary)
