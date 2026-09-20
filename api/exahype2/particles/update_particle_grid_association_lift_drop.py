# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from exahype2.actionsets.action_set import ActionSet


class UpdateParticleGridAssociationLiftDrop(ActionSet):
    """! Maintains the association between particles and vertices.

    Each particle sits at the closest vertex on the finest level that accommodates its search
    radius.
    """

    DefaultDescendInvocationOrder = -65536

    def __init__(self, particle_set):
        super().__init__(descend_invocation_order=self.DefaultDescendInvocationOrder)
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name

    _TEMPLATE_BEGIN_TRAVERSAL = jinja2.Template("""
  _liftDropStatistics.clear();
""")

    _TEMPLATE_END_TRAVERSAL = jinja2.Template("""
  _liftDropStatistics.record<vertexdata::{{PARTICLES_CONTAINER}}>();
""")

    _TEMPLATE_DROP = jinja2.Template("""
  ::exahype2::particles::dropParticles(
    marker,
    fineGridVertex{{PARTICLES_CONTAINER}},
    coarseGridVertices{{PARTICLES_CONTAINER}},
    "{{PARTICLE}}",
    _treeId,
    _liftDropStatistics
  );
""")

    _TEMPLATE_SIEVE = jinja2.Template("""
  ::exahype2::particles::sieveParticlesIntoVertex(
    marker,
    fineGridVertex{{PARTICLES_CONTAINER}},
    _treeId,
    _liftDropStatistics
  );
""")

    _TEMPLATE_LIFT_OR_REASSIGN_PARTICLES = jinja2.Template("""
  ::exahype2::particles::liftOrReassignParticles(
    marker,
    fineGridVertex{{PARTICLES_CONTAINER}},
    coarseGridVertices{{PARTICLES_CONTAINER}},
    DomainOffset,
    DomainSize,
    PeriodicBoundaries,
    "{{PARTICLE}}",
    _treeId,
    _liftDropStatistics
  );
""")

    _TEMPLATE_LIFT_ALL_PARTICLES = jinja2.Template("""
  ::exahype2::particles::liftAllParticles(
    marker,
    fineGridVertex{{PARTICLES_CONTAINER}},
    coarseGridVertices{{PARTICLES_CONTAINER}},
    "{{PARTICLE}}",
    _treeId,
    _liftDropStatistics
  );
""")

    def operation_body(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = self._TEMPLATE_BEGIN_TRAVERSAL.render(**self.d)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self._TEMPLATE_DROP.render(**self.d)
            result += self._TEMPLATE_SIEVE.render(**self.d)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self._TEMPLATE_LIFT_OR_REASSIGN_PARTICLES.render(**self.d)
        elif operation_name == ActionSet.OPERATION_DESTROY_VERTEX:
            result = self._TEMPLATE_LIFT_ALL_PARTICLES.render(**self.d)
        elif operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self._TEMPLATE_END_TRAVERSAL.render(**self.d)
        return result

    def unprepare_traversal_body(self):
        """Advances the sieve exchange after all rank-local traversals finish."""
        return jinja2.Template("""
  vertexdata::{{PARTICLES_CONTAINER}}::finishedTraversal(DomainOffset, DomainSize, PeriodicBoundaries);
""").render(**self.d)

    def includes(self):
        return [
            "Constants.h",
            "exahype2/particles/GridAssociation.h",
            "vertexdata/{}.h".format(self.d["PARTICLES_CONTAINER"]),
        ]

    def get_constructor_body(self):
        return super().get_constructor_body() + "\n  _treeId = treeId;\n"

    def attributes(self):
        return """
  int                                       _treeId{-1};
  ::exahype2::particles::LiftDropStatistics _liftDropStatistics{};
"""
