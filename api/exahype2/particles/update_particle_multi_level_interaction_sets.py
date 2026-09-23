# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

import jinja2

from exahype2.actionsets.action_set import ActionSet


def _dedented(code):
    """Returns ``code`` without its common indentation, so a template indents it."""
    return None if code is None else textwrap.dedent(code).strip()


class UpdateParticleMultiLevelInteractionSets(ActionSet):
    """! Runs particle interactions across cells and mesh levels.

    On descent, each cell appends the particles of its adjacent vertices to
    the active list inherited from coarser levels. The local list contains
    the particles of the cell's adjacent vertices, and the aligned
    localParticleIsContainedInCell flags identify particles owned by this
    cell. The appended active ranges are removed again on ascent.

    The coalesced-length vectors exposed to kernels hold -1. Select
    active_particle_set to read a second particle species while updating the
    local species.

    Vertex kernels receive assignedParticles and
    numberOfCoalescedAssignedParticles. The cell kernel receives marker,
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
        self._dictionary["PARTICLE_PARTICLE_INTERACTION_KERNEL"] = _dedented(
            particle_particle_interaction_kernel
        )
        self._dictionary["TOUCH_VERTEX_FIRST_COMPUTE_KERNEL"] = _dedented(
            touch_vertex_first_time_compute_particle_update_kernel
        )
        self._dictionary["TOUCH_VERTEX_LAST_COMPUTE_KERNEL"] = _dedented(
            touch_vertex_last_time_compute_particle_update_kernel
        )
        self._dictionary["PREPARE_TRAVERSAL_KERNEL"] = prepare_traversal_kernel
        self._dictionary["UNPREPARE_TRAVERSAL_KERNEL"] = unprepare_traversal_kernel

    _TEMPLATE_TOUCH_VERTEX_FIRST_TIME = jinja2.Template("""
{%- if TOUCH_VERTEX_FIRST_COMPUTE_KERNEL is not none %}
  [[maybe_unused]] auto&     assignedParticles                  = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  [[maybe_unused]] const int numberOfCoalescedAssignedParticles = -1;

  {{TOUCH_VERTEX_FIRST_COMPUTE_KERNEL | indent(2)}}
{%- endif %}
""")

    _TEMPLATE_TOUCH_VERTEX_LAST_TIME = jinja2.Template("""
{%- if TOUCH_VERTEX_LAST_COMPUTE_KERNEL is not none %}
  [[maybe_unused]] auto&     assignedParticles                  = fineGridVertex{{LOCAL_PARTICLES_CONTAINER}};
  [[maybe_unused]] const int numberOfCoalescedAssignedParticles = -1;

  {{TOUCH_VERTEX_LAST_COMPUTE_KERNEL | indent(2)}}
{%- endif %}
""")

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = jinja2.Template("""
  std::list<globaldata::{{LOCAL_PARTICLE}}*> localParticles;
  std::vector<int>                           numberOfCoalescedLocalParticlesPerVertex(peano4::VerticesPerCell, -1);
  std::vector<int>                           numberOfCoalescedActiveParticlesPerVertex(peano4::VerticesPerCell, -1);
  std::vector<bool>                          localParticleIsContainedInCell;

  _numberOfCollectedActiveParticles.push_back(
    ::exahype2::particles::collectParticlesOfCell(
      marker,
      fineGridVertices{{ACTIVE_PARTICLES_CONTAINER}},
      fineGridVertices{{LOCAL_PARTICLES_CONTAINER}},
      _activeParticles,
      localParticles,
      localParticleIsContainedInCell
    )
  );
{%- if PARTICLE_PARTICLE_INTERACTION_KERNEL is not none %}

  [[maybe_unused]] auto& activeParticles = _activeParticles;
  {{PARTICLE_PARTICLE_INTERACTION_KERNEL | indent(2)}}

  ::exahype2::particles::updateContainedInAdjacentCellOfLocalParticles(
    marker,
    fineGridVertices{{LOCAL_PARTICLES_CONTAINER}}
  );
{%- endif %}
""")

    _TEMPLATE_TOUCH_CELL_LAST_TIME = jinja2.Template("""
  ::exahype2::particles::releaseParticlesOfCell(_numberOfCollectedActiveParticles.back(), _activeParticles);
  _numberOfCollectedActiveParticles.pop_back();
""")

    def operation_body(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self._TEMPLATE_TOUCH_CELL_FIRST_TIME.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self._TEMPLATE_TOUCH_CELL_LAST_TIME.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self._TEMPLATE_TOUCH_VERTEX_FIRST_TIME.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self._TEMPLATE_TOUCH_VERTEX_LAST_TIME.render(**self._dictionary)
        return result

    def prepare_traversal_body(self):
        return self._dictionary["PREPARE_TRAVERSAL_KERNEL"]

    def unprepare_traversal_body(self):
        return self._dictionary["UNPREPARE_TRAVERSAL_KERNEL"]

    def includes(self):
        return [
            "exahype2/particles/InteractionSets.h",
            "globaldata/{}.h".format(self._dictionary["LOCAL_PARTICLE"]),
            "globaldata/{}.h".format(self._dictionary["ACTIVE_PARTICLE"]),
            "vertexdata/{}.h".format(self._dictionary["LOCAL_PARTICLES_CONTAINER"]),
            "vertexdata/{}.h".format(self._dictionary["ACTIVE_PARTICLES_CONTAINER"]),
        ]

    def attributes(self):
        return jinja2.Template("""
  std::list<globaldata::{{ACTIVE_PARTICLE}}*> _activeParticles{};
  std::vector<int>                            _numberOfCollectedActiveParticles{};
""").render(**self._dictionary)
