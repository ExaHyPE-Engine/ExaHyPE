# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

from peano4.solversteps.ActionSet import ActionSet


class UpdateParticle_MultiLevelInteraction_Sets(ActionSet):
    """! Run particle interactions across cells and mesh levels.

    On descent, each cell appends the particles of its adjacent vertices to
    the active list inherited from coarser levels.  The local list contains
    the particles of the cell's adjacent vertices, and the aligned
    localParticleIsContainedInCell flags identify particles owned by this
    cell.  The appended active ranges are removed again on ascent.

    The pointer collections do not require contiguous particle storage.  The
    coalesced-length vectors exposed to kernels consequently hold -1.  Select
    active_particle_set to read a second particle species while updating the
    local species.

    Vertex kernels receive assignedParticles and
    numberOfCoalescedAssignedParticles.  The cell kernel receives marker,
    activeParticles, localParticles, localParticleIsContainedInCell,
    numberOfCoalescedActiveParticlesPerVertex, and
    numberOfCoalescedLocalParticlesPerVertex.
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
        super().__init__()
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
        self._dictionary["PREPARE_TRAVERSAL_KERNEL"] = prepare_traversal_kernel
        self._dictionary["UNPREPARE_TRAVERSAL_KERNEL"] = unprepare_traversal_kernel
        self._dictionary["ADDITIONAL_INCLUDES"] = additional_includes

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
{% if TOUCH_VERTEX_FIRST_COMPUTE_KERNEL is not none %}
  auto& assignedParticles = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  [[maybe_unused]] const int numberOfCoalescedAssignedParticles = -1;

  {{TOUCH_VERTEX_FIRST_COMPUTE_KERNEL}}
{% endif %}
"""
    )

    __Template_TouchVertexLastTime = jinja2.Template(
        """
{% if TOUCH_VERTEX_LAST_COMPUTE_KERNEL is not none %}
  auto& assignedParticles = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  [[maybe_unused]] const int numberOfCoalescedAssignedParticles = -1;

  {{TOUCH_VERTEX_LAST_COMPUTE_KERNEL}}
{% endif %}
"""
    )

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  std::list<globaldata::{{LOCAL_PARTICLE}}*> localParticles;
  std::vector<int>                           numberOfCoalescedLocalParticlesPerVertex(TWO_POWER_D, -1);
  std::vector<int>                           numberOfCoalescedActiveParticlesPerVertex(TWO_POWER_D, -1);
  std::vector<bool>                          localParticleIsContainedInCell;
  [[maybe_unused]] const auto                ownershipTolerance =
    peano4::particles::GrabOwnershipSpatialSortingTolerance * tarch::la::max(marker.getH())
    * tarch::la::NUMERICAL_ZERO_DIFFERENCE;

  for (int vertex = 0; vertex < TWO_POWER_D; vertex++) {
    for (auto* const particle : fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(vertex)) {
      _activeParticles.push_back(particle);
    }
    for (auto* const particle : fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(vertex)) {
      std::bitset<TWO_POWER_D> adjacentCellMask{};
      adjacentCellMask.set(TWO_POWER_D - vertex - 1);
      const bool isLocal = (adjacentCellMask & particle->getContainedInAdjacentCell()).any();
      assertion(
        isLocal
          == marker.isContained(
            peano4::particles::periodicImageTowardsCell(particle->getX(), marker),
            ownershipTolerance
          ),
        isLocal,
        marker.isContained(peano4::particles::periodicImageTowardsCell(particle->getX(), marker), ownershipTolerance),
        marker.toString(),
        particle->toString(),
        vertex,
        adjacentCellMask
      );
      localParticleIsContainedInCell.push_back(isLocal);
      localParticles.push_back(particle);
    }
  }

{% if PARTICLE_PARTICLE_INTERACTION_KERNEL is not none %}
  [[maybe_unused]] auto& activeParticles = _activeParticles;
  {{PARTICLE_PARTICLE_INTERACTION_KERNEL}}

  // Interaction kernels may move particles and invalidate their cached cell ownership.
  for (int vertex = 0; vertex < TWO_POWER_D; vertex++) {
    const std::bitset<DIMENSIONS> assignedVertex(vertex);
    for (auto* const particle : fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(vertex)) {
      peano4::particles::updateContainedInAdjacentCellProperty(marker, assignedVertex, *particle);
    }
  }
{% endif %}
"""
    )

    __Template_TouchCellLastTime = jinja2.Template(
        """
  for (int vertex = 0; vertex < TWO_POWER_D; vertex++) {
    _activeParticles.erase(
      std::prev(
        _activeParticles.end(),
        fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(vertex).size()
      ),
      _activeParticles.end()
    );
  }
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
#include "peano4/particles/particles.h"
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
"""
        ).render(**self._dictionary)
