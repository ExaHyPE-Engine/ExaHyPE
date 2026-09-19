# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.actionsets.action_set import ActionSet

from .update_particle_grid_association_lift_drop import (
    UpdateParticleGridAssociationLiftDrop,
)


class UpdateParallelState(ActionSet):
    """Particle positions remain fixed between this action's first- and last-touch
    callbacks. Add the action after insertion and sorting callbacks whenever a
    traversal can change particle positions or ownership.

    @image html UpdateParallelState_merge.png
    """

    DefaultDescendInvocationOrder = (
        UpdateParticleGridAssociationLiftDrop.DefaultDescendInvocationOrder + 1
    )

    def __init__(self, particle_set):
        super().__init__(descend_invocation_order=self.DefaultDescendInvocationOrder)
        self._dictionary = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
        }

    _TEMPLATE_BEGIN_TRAVERSAL = jinja2.Template("""
  _parallelStateStatistics.clear();
""")

    _TEMPLATE_END_TRAVERSAL = jinja2.Template("""
  _parallelStateStatistics.record<vertexdata::{{PARTICLES_CONTAINER}}>();
""")

    _TEMPLATE_TOUCH_VERTEX_FIRST_TIME = jinja2.Template("""
  ::exahype2::particles::updateParallelStateOfParticles(
    marker,
    fineGridVertex{{PARTICLES_CONTAINER}},
    "{{PARTICLE}}",
    _treeId,
    _parallelStateStatistics
  );
""")

    _TEMPLATE_TOUCH_VERTEX_LAST_TIME = jinja2.Template("""
  ::exahype2::particles::deleteVirtualParticles(
    marker,
    fineGridVertex{{PARTICLES_CONTAINER}},
    "{{PARTICLE}}",
    _treeId,
    _parallelStateStatistics
  );
""")

    def get_constructor_body(self):
        return "  _treeId = treeId;\n"

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            return self._TEMPLATE_BEGIN_TRAVERSAL.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            return self._TEMPLATE_TOUCH_VERTEX_FIRST_TIME.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            return self._TEMPLATE_TOUCH_VERTEX_LAST_TIME.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            return self._TEMPLATE_END_TRAVERSAL.render(**self._dictionary)
        return ""

    def includes(self):
        return [
            "exahype2/particles/ParallelState.h",
            "vertexdata/{}.h".format(self._dictionary["PARTICLES_CONTAINER"]),
        ]

    def attributes(self):
        return """
  int                                            _treeId{-1};
  ::exahype2::particles::ParallelStateStatistics _parallelStateStatistics{};
"""

    def prepare_traversal_body(self):
        return jinja2.Template("""
  vertexdata::{{PARTICLES_CONTAINER}}::clearParticleStateStatistics();
""").render(**self._dictionary)
