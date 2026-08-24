# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

import jinja2

from peano4.solversteps.ActionSet import ActionSet

from .AbstractUpdateParticleGridAssociation import AbstractUpdateParticleGridAssociation


class UpdateParticleGridAssociation_BucketSort(AbstractUpdateParticleGridAssociation):
    """! Incrementally associate particles with their closest vertices.

    A particle remains in its current bucket while that vertex is still the
    closest admissible one.  Particles that have crossed a bucket boundary or
    no longer fit the current mesh level move to the global sieve and are
    reinserted during the next traversal.

    Refinement would normally drop particles to a finer vertex.  Bucket sort
    instead sends these particles to the sieve, because changing the coarse
    and fine vertex lists within one event would break the bucket invariant.
    Each action set therefore owns a clone of the sieve containing local and
    virtual particles.  The clone permits destructive, lock-free extraction
    while the traversal inserts copies into the mesh.

    Only the current and finer vertex lists are modified.  Coarser lists keep
    their contiguous layout, so this action set can precede an action that
    gathers particles as described in
    @ref toolbox_particles_memorypool "Peano's memory pool discussion".
    """

    __Template_LiftParticles = jinja2.Template(
        """
  auto particleIterator = fineGridVertex{{PARTICLES_CONTAINER}}.begin();
  while (particleIterator != fineGridVertex{{PARTICLES_CONTAINER}}.end()) {
    auto* const particle = *particleIterator;
    const auto instruction = toolbox::particles::liftParticleAssociatedWithVertex(*particle, marker);

#if PEANO_ASSERTIONS
    const int particleId = particle->getPartid();
#else
    const int particleId = -1;
#endif
    const bool particleIsLocal =
      particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local;

    switch (instruction) {
      case toolbox::particles::ParticleReassociationInstruction_Keep:
        if (toolbox::particles::particleWillBeDroppedFurther(*particle, marker)) {
          ++_numberOfLiftsIntoSieveSet;
          logDebug(
            "Lift particle " << particle->toString()
                             << " into the sieve because the next traversal has to drop it."
          );
          toolbox::particles::assignmentchecks::detachParticleFromVertex(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            marker.getX(),
            marker.getH(),
            _spacetreeId,
            "UpdateParticleGridAssociation_BucketSort::__Template_LiftParticles"
          );
          toolbox::particles::assignmentchecks::assignParticleToSieveSet(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            marker.getX(),
            marker.getH(),
            _spacetreeId,
            "UpdateParticleGridAssociation_BucketSort::__Template_LiftParticles"
          );
          particleIterator =
            fineGridVertex{{PARTICLES_CONTAINER}}.particleCanNotBeLiftedLocally(particleIterator);
        } else {
          ++particleIterator;
        }
        break;

      case toolbox::particles::ParticleReassociationInstruction_SieveGlobally:
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
        ++_numberOfLiftsIntoSieveSet;
        logDebug(
          "Lift particle " << particle->toString() << " globally from " << marker.toString()
                           << "."
        );
        toolbox::particles::assignmentchecks::detachParticleFromVertex(
          "{{PARTICLE}}",
          particle->getX(),
          particleId,
          particleIsLocal,
          marker.getX(),
          marker.getH(),
          _spacetreeId,
          "UpdateParticleGridAssociation_BucketSort::__Template_LiftParticles"
        );
        toolbox::particles::assignmentchecks::assignParticleToSieveSet(
          "{{PARTICLE}}",
          particle->getX(),
          particleId,
          particleIsLocal,
          marker.getX(),
          marker.getH(),
          _spacetreeId,
          "UpdateParticleGridAssociation_BucketSort::__Template_LiftParticles"
        );
        particleIterator =
          fineGridVertex{{PARTICLES_CONTAINER}}.particleCanNotBeLiftedLocally(particleIterator);
        assertion(
          fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),
          fineGridVertex{{PARTICLES_CONTAINER}}.toString()
        );
        break;

      default:
        assertion(false, "Unsupported particle reassociation instruction.");
        break;
    }
  }
"""
    )

    _Template_Sieve = jinja2.Template(
        """
  if (_sieveParticleSet.hasParticlesToBeSievedIntoVertices()) {
    constexpr bool RemoveReturnedParticlesFromSet = true;
    constexpr bool OnlyReturnParticlesThatWillBeLocal = false;
    constexpr bool LockParticleSet = false;

    const auto sievedParticles = _sieveParticleSet.getParticlesToBeSievedIntoVertex(
      marker,
      RemoveReturnedParticlesFromSet,
      OnlyReturnParticlesThatWillBeLocal,
      LockParticleSet
    );

    for (const auto* const sievedParticle : sievedParticles) {
      ++_numberOfDropsFromSieveSet;
      fineGridVertex{{PARTICLES_CONTAINER}}.mergeWithParticle(
        new globaldata::{{PARTICLE}}(*sievedParticle),
        marker,
        _spacetreeId
      );
    }

    assertion(
      fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),
      fineGridVertex{{PARTICLES_CONTAINER}}.toString()
    );
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = self._Template_BeginTraversal.render(**self.d)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self._Template_Sieve.render(**self.d)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self.__Template_LiftParticles.render(**self.d)
        elif operation_name == ActionSet.OPERATION_DESTROY_VERTEX:
            result = self._Template_LiftAllParticles.render(**self.d)
        elif operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self._Template_EndTraversal.render(**self.d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_constructor_body(self):
        return (
            super().get_constructor_body()
            + jinja2.Template(
                """
  _sieveParticleSet = vertexdata::{{PARTICLES_CONTAINER}}::cloneParticlesToBeSieved();
"""
            ).render(**self.d)
        )

    def get_destructor_body(self):
        return (
            super().get_destructor_body()
            + jinja2.Template(
                """
  _sieveParticleSet.deleteParticles();
"""
            ).render(**self.d)
        )

    def get_attributes(self):
        return (
            super().get_attributes()
            + jinja2.Template(
                """
  toolbox::particles::SieveParticles<globaldata::{{PARTICLE}}> _sieveParticleSet;
"""
            ).render(**self.d)
        )
