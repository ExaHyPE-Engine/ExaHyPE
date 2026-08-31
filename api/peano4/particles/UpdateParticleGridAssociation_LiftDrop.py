# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

from peano4.solversteps.ActionSet import ActionSet

from .AbstractUpdateParticleGridAssociation import AbstractUpdateParticleGridAssociation


class UpdateParticleGridAssociation_LiftDrop(AbstractUpdateParticleGridAssociation):
    """! The last vertex event lifts a particle until it lies within the owning
    vertex's adjacent cells, and the next traversal drops it to the finest level
    that accommodates its search radius.

    Add the action early in a step: Peano then drops before particle updates on
    descent and lifts after those updates on backtracking.
    """

    _Template_Sieve = jinja2.Template(
        """
  if (vertexdata::{{PARTICLES_CONTAINER}}::hasParticlesToBeSievedIntoVertices()) {
    const auto sievedParticles =
      vertexdata::{{PARTICLES_CONTAINER}}::getParticlesToBeSievedIntoVertex(marker);

    for (auto* const newParticle : sievedParticles) {
      ++_numberOfDropsFromSieveSet;
      fineGridVertex{{PARTICLES_CONTAINER}}.mergeWithParticle(
        newParticle,
        marker,
        _treeId
      );
    }

    assertion(
      fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),
      fineGridVertex{{PARTICLES_CONTAINER}}.toString()
    );
  }
"""
    )

    __Template_LiftOrReassignParticles = jinja2.Template(
        """
  auto particleIterator = fineGridVertex{{PARTICLES_CONTAINER}}.begin();
  while (particleIterator != fineGridVertex{{PARTICLES_CONTAINER}}.end()) {
    auto* const particle = *particleIterator;
    // A particle that left the domain through a periodic wall re-enters at
    // the opposite wall before the lift reads its position.
    if (PeriodicBC.any()) {
      particle->setX(
        toolbox::particles::applyPeriodicBoundaryConditions(particle->getX(), DomainOffset, DomainSize, PeriodicBC)
      );
    }
    const auto instruction = toolbox::particles::liftParticleAssociatedWithVertex(*particle, marker);

#if PEANO_ASSERTIONS
    const int particleId = particle->getPartid();
#else
    const int particleId = -1;
#endif
    const bool particleIsLocal =
      particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local;

    switch (instruction) {
      case toolbox::particles::ParticleReassociationInstructionKeep:
        toolbox::particles::updateContainedInAdjacentCellProperty(marker, *particle);
        ++particleIterator;
        break;

      case toolbox::particles::ParticleReassociationInstructionSieveGlobally:
        ++_numberOfLiftsIntoSieveSet;
        logDebug(
          "Lift particle " << particle->toString() << " globally from " << marker.toString()
                           << " on tree " << _treeId << "."
        );
        toolbox::particles::assignmentchecks::detachParticleFromVertex(
          "{{PARTICLE}}",
          particle->getX(),
          particleId,
          particleIsLocal,
          marker.getX(),
          marker.getH(),
          _treeId,
          "UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles"
        );
        toolbox::particles::assignmentchecks::assignParticleToSieveSet(
          "{{PARTICLE}}",
          particle->getX(),
          particleId,
          particleIsLocal,
          marker.getX(),
          marker.getH(),
          _treeId,
          "UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles"
        );
        particleIterator =
          fineGridVertex{{PARTICLES_CONTAINER}}.particleCanNotBeLiftedLocally(particleIterator);
        assertion(
          fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),
          fineGridVertex{{PARTICLES_CONTAINER}}.toString()
        );
        break;

      case 0:
      case 1:
      case 2:
      case 3:
#if DIMENSIONS == 3
      case 4:
      case 5:
      case 6:
      case 7:
#endif
      {
        ++_numberOfLifts;
        logDebug(
          "Lift particle " << particle->toString() << " from " << marker.toString()
                           << " to coarse vertex " << instruction << " in "
                           << coarseGridVertices{{PARTICLES_CONTAINER}}(instruction).toString()
                           << "."
        );
        const auto parentVertexPosition =
          peano4::datamanagement::reconstructXOfParentVertex(marker, instruction);
        const auto parentCellSize = 3.0 * marker.getH();
        particle->setCellH(parentCellSize);
        toolbox::particles::updateContainedInAdjacentCellProperty(
          parentVertexPosition,
          parentCellSize,
          *particle
        );
        toolbox::particles::assignmentchecks::detachParticleFromVertex(
          "{{PARTICLE}}",
          particle->getX(),
          particleId,
          particleIsLocal,
          marker.getX(),
          marker.getH(),
          _treeId,
          "UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles"
        );
        toolbox::particles::assignmentchecks::assignParticleToVertex(
          "{{PARTICLE}}",
          particle->getX(),
          particleId,
          particleIsLocal,
          parentVertexPosition,
          parentCellSize,
          _treeId,
          "UpdateParticleGridAssociation_LiftDrop::__Template_LiftOrReassignParticles",
          marker.getX(),
          marker.getH()
        );
        particleIterator = coarseGridVertices{{PARTICLES_CONTAINER}}(instruction).grabParticle(
          particleIterator,
          fineGridVertex{{PARTICLES_CONTAINER}}
        );
        assertion(
          fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),
          fineGridVertex{{PARTICLES_CONTAINER}}.toString(),
          coarseGridVertices{{PARTICLES_CONTAINER}}(instruction).toString()
        );
        assertion(
          coarseGridVertices{{PARTICLES_CONTAINER}}(instruction).isValid(),
          fineGridVertex{{PARTICLES_CONTAINER}}.toString(),
          coarseGridVertices{{PARTICLES_CONTAINER}}(instruction).toString()
        );
        break;
      }

      default:
        assertion(false, "Unsupported particle reassociation instruction.");
        break;
    }
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = self._Template_BeginTraversal.render(**self.d)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self._Template_Drop.render(**self.d)
            result += self._Template_Sieve.render(**self.d)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_LiftOrReassignParticles.render(**self.d)
        elif operation_name == ActionSet.OPERATION_DESTROY_VERTEX:
            result = self._Template_LiftAllParticles.render(**self.d)
        elif operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self._Template_EndTraversal.render(**self.d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
