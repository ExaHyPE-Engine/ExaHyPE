# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

from peano4.solversteps.ActionSet import ActionSet


class UpdateParticle_SingleLevelInteraction(ActionSet):
    """! Update particles with neighbours on the same mesh level.

    The cell kernel receives activeParticles and localParticles from all
    adjacent vertices, together with marker and an aligned
    localParticleIsContainedInCell vector.  The Boolean vector distinguishes
    particles owned by the current cell from particles in its adjacent halo.
    Both coalesced-length vectors contain -1 because this action makes no
    continuous-storage guarantee.

    Vertex kernels receive assignedParticles and
    numberOfCoalescedAssignedParticles, which is also -1.  Select
    active_particle_set to read a second particle species while updating the
    local species.  The action constructs all collections per event and keeps
    no pointers between cells, so position changes and reassociation by other
    action sets do not invalidate persistent state.

    Cell ownership overlaps by a floating-point tolerance at cell boundaries.
    A kernel that updates each particle once must therefore maintain an update
    flag on the particle.
    """

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
        self._dictionary["PARTICLE_CELL_UPDATE_KERNEL"] = particle_cell_update_kernel
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
{% if PARTICLE_CELL_UPDATE_KERNEL is not none %}
  std::list<globaldata::{{LOCAL_PARTICLE}}*> localParticles;
  std::list<globaldata::{{ACTIVE_PARTICLE}}*> activeParticles;
  const std::vector<int>                      numberOfCoalescedLocalParticlesPerVertex(TWO_POWER_D, -1);
  const std::vector<int>                      numberOfCoalescedActiveParticlesPerVertex(TWO_POWER_D, -1);
  std::vector<bool>                           localParticleIsContainedInCell;

  [[maybe_unused]] const auto ownershipTolerance = toolbox::particles::GrabOwnershipSpatialSortingTolerance
    * tarch::la::max(marker.getH()) * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
  for (int vertex = 0; vertex < TWO_POWER_D; vertex++) {
    for (auto* const particle : fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}}(vertex)) {
      activeParticles.push_front(particle);
    }
    for (auto* const particle : fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}(vertex)) {
      std::bitset<TWO_POWER_D> adjacentCellMask{};
      adjacentCellMask.set(TWO_POWER_D - vertex - 1);
      const bool particleIsContainedInCell =
        (adjacentCellMask & particle->getContainedInAdjacentCell()).any();
      assertion(
        particleIsContainedInCell == marker.isContained(particle->getX(), ownershipTolerance),
        particleIsContainedInCell,
        marker.isContained(particle->getX(), ownershipTolerance),
        marker.toString(),
        particle->toString()
      );
      localParticleIsContainedInCell.push_back(particleIsContainedInCell);
      localParticles.push_front(particle);
    }
  }
  std::reverse(localParticleIsContainedInCell.begin(), localParticleIsContainedInCell.end());

  {{PARTICLE_CELL_UPDATE_KERNEL}}

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

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCellFirstTime.render(**self._dictionary)
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
#include "toolbox/particles/particles.h"
#include "vertexdata/{{LOCAL_PARTICLES_CONTAINER}}.h"
{% if ACTIVE_PARTICLES_CONTAINER != LOCAL_PARTICLES_CONTAINER %}
#include "vertexdata/{{ACTIVE_PARTICLES_CONTAINER}}.h"
{% endif %}

{{ADDITIONAL_INCLUDES}}
"""
        ).render(**self._dictionary)
