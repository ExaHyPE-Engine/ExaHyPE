# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.particles.AbstractUpdateParticleGridAssociation import (
    AbstractUpdateParticleGridAssociation,
)
from peano4.solversteps.ActionSet import ActionSet


class UpdateParallelState(ActionSet):
    """Maintains local and virtual particle ownership across tree boundaries.

    First-touch processing marks local particles that left a tree as virtual.
    Last-touch processing records local particle identities and removes virtual
    copies; the next boundary exchange recreates current copies from their owner.
    ``ParticleSet.merge()`` ignores incoming virtual copies to prevent replicas
    from being echoed between neighbouring trees.

    Particle positions remain fixed between this action's first- and last-touch
    callbacks. Add the action after insertion and sorting callbacks whenever a
    traversal can change particle positions or ownership.

    @image html UpdateParallelState_merge.png
    """

    DefaultDescendInvocationOrder = (
        AbstractUpdateParticleGridAssociation.DefaultDescendInvocationOrder + 1
    )

    def __init__(self, particle_set):
        super().__init__(descend_invocation_order=self.DefaultDescendInvocationOrder)
        self._dictionary = {
            "PARTICLE": particle_set.particle_model.name,
            "PARTICLES_CONTAINER": particle_set.name,
        }

    __Template_BeginTraversal = jinja2.Template(
        """
  _numberOfRemainingLocalParticles = 0;
  _remainingLocalParticleNumbers.clear();
  _numberOfExpiredHaloParticles = 0;
  _numberOfParticlesThatHaveLeftTheirDomain = 0;
"""
    )

    __Template_EndTraversal = jinja2.Template(
        """
  vertexdata::{{PARTICLES_CONTAINER}}::updateNumberOfLocalAndExpiredParticles(
    _numberOfRemainingLocalParticles,
    _numberOfExpiredHaloParticles,
    _numberOfParticlesThatHaveLeftTheirDomain,
    _remainingLocalParticleNumbers
  );
"""
    )

    __Template_TouchVertexFirstTime = jinja2.Template(
        """
  for (auto* const particle : fineGridVertex{{PARTICLES_CONTAINER}}) {
    const bool particleWillBeLocal =
      toolbox::particles::particleAssignedToVertexWillBeLocal(particle->getX(), marker);
    if (
      not particleWillBeLocal
      and particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local
    ) {
      particle->setParallelState(globaldata::{{PARTICLE}}::ParallelState::Virtual);
      logDebug(
        "Particle " << particle->toString() << " left tree " << _spacetreeId
                    << " through " << marker.toString() << "."
      );
      ++_numberOfParticlesThatHaveLeftTheirDomain;

#if PEANO_ASSERTIONS
      const int particleId = particle->getPartid();
#else
      const int particleId = -1;
#endif

      toolbox::particles::assignmentchecks::detachParticleFromVertex(
        "{{PARTICLE}}",
        particle->getX(),
        particleId,
        true,
        marker.getX(),
        marker.getH(),
        _spacetreeId,
        "UpdateParallelState::__Template_TouchVertexFirstTime"
      );
      toolbox::particles::assignmentchecks::assignParticleToVertex(
        "{{PARTICLE}}",
        particle->getX(),
        particleId,
        false,
        marker.getX(),
        marker.getH(),
        _spacetreeId,
        "UpdateParallelState::__Template_TouchVertexFirstTime",
        marker.getX(),
        marker.getH()
      );
    }
  }
"""
    )

    __Template_TouchVertexLastTime = jinja2.Template(
        """
  auto particleIterator = fineGridVertex{{PARTICLES_CONTAINER}}.begin();
  while (particleIterator != fineGridVertex{{PARTICLES_CONTAINER}}.end()) {
    auto* const particle = *particleIterator;
    if (particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local) {
      logDebug("Particle " << particle->toString() << " remains local.");
      ++_numberOfRemainingLocalParticles;
      _remainingLocalParticleNumbers.insert({
        particle->getNumber(0),
        particle->getNumber(1)
      });
      ++particleIterator;
    } else {
#if PEANO_ASSERTIONS
      const int particleId = particle->getPartid();
#else
      const int particleId = -1;
#endif

      logDebug(
        "Remove virtual particle " << particle->toString() << " from "
                                   << marker.toString() << " on tree " << _spacetreeId << "."
      );
      toolbox::particles::assignmentchecks::detachParticleFromVertex(
        "{{PARTICLE}}",
        particle->getX(),
        particleId,
        false,
        marker.getX(),
        marker.getH(),
        _spacetreeId,
        "UpdateParallelState::__Template_TouchVertexLastTime"
      );
      toolbox::particles::assignmentchecks::eraseParticle(
        "{{PARTICLE}}",
        particle->getX(),
        particleId,
        false,
        marker.getX(),
        marker.getH(),
        _spacetreeId,
        "UpdateParallelState::__Template_TouchVertexLastTime"
      );
      ++_numberOfExpiredHaloParticles;
      particleIterator =
        fineGridVertex{{PARTICLES_CONTAINER}}.deleteParticle(particleIterator);
    }
  }
"""
    )

    def get_constructor_body(self):
        return "  _spacetreeId = treeNumber;\n"

    def get_body_of_operation(self, operation_name):
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            return self.__Template_BeginTraversal.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            return self.__Template_TouchVertexFirstTime.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            return self.__Template_TouchVertexLastTime.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            return self.__Template_EndTraversal.render(**self._dictionary)
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector<peano4::grid::GridControlEvent>{};\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return jinja2.Template(
            """
#include "toolbox/particles/MultiscaleTransitions.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"

#include "globaldata/{{PARTICLE}}.h"
#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
"""
        ).render(**self._dictionary)

    def get_attributes(self):
        return """
  int _spacetreeId;
  int _numberOfRemainingLocalParticles;
  std::set<std::array<int, 2>> _remainingLocalParticleNumbers;
  int _numberOfExpiredHaloParticles;
  int _numberOfParticlesThatHaveLeftTheirDomain;
"""

    def get_body_of_prepareTraversal(self):
        return jinja2.Template(
            """
  vertexdata::{{PARTICLES_CONTAINER}}::clearParticleStateStatistics();
"""
        ).render(**self._dictionary)
