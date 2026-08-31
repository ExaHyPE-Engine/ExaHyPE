# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.solversteps.ActionSet import ActionSet


class AbstractUpdateParticleGridAssociation(ActionSet):
    """Maintains the association between particles and vertices.

    Each particle is stored at the closest vertex on the finest level that can
    accommodate its search radius, and a change of a particle position or search
    radius invalidates that association until a sorting traversal completes.
    Sorting changes particle-list storage, so a continuous particle memory pool
    has to gather and resort its lists around this action.
    """

    DefaultDescendInvocationOrder = -65536

    def __init__(self, particle_set, guard="true"):
        super().__init__(descend_invocation_order=self.DefaultDescendInvocationOrder)
        self._particle_set = particle_set
        self.d = {}
        self.d["PARTICLE"] = particle_set.particle_model.name
        self.d["PARTICLES_CONTAINER"] = particle_set.name
        self.d["GUARD"] = guard

    _Template_BeginTraversal = jinja2.Template(
        """
  _numberOfParticleReassignmentsOnCurrentLevel = 0;
  _numberOfLifts = 0;
  _numberOfDrops = 0;
  _numberOfLiftsIntoSieveSet = 0;
  _numberOfDropsFromSieveSet = 0;
  _numberOfDropsIntoHorizontalTreeDecomposition = 0;
"""
    )

    _Template_EndTraversal = jinja2.Template(
        """
  vertexdata::{{PARTICLES_CONTAINER}}::updateLiftDropStatistics(
    _numberOfLifts,
    _numberOfDrops,
    _numberOfParticleReassignmentsOnCurrentLevel,
    _numberOfLiftsIntoSieveSet,
    _numberOfDropsFromSieveSet,
    _numberOfDropsIntoHorizontalTreeDecomposition
  );
"""
    )

    _Template_ValidateParticleLevelAssociation = jinja2.Template(
        """
  [[maybe_unused]] const auto ownershipTolerance =
    toolbox::particles::ReleaseOwnershipSpatialSortingTolerance * tarch::la::max(marker.getH())
    * tarch::la::NUMERICAL_ZERO_DIFFERENCE;
  for ([[maybe_unused]] const auto* const particle : fineGridVertex{{PARTICLES_CONTAINER}}) {
    assertion(
      marker.isContainedInAdjacentCells(
        toolbox::particles::periodicImageTowardsVertex(particle->getX(), marker),
        0.5,
        ownershipTolerance
      ),
      marker.getX(),
      marker.toString(),
      particle->toString()
    );
  }
"""
    )

    # A vertical tree cut can make a parent vertex remote, so only a local parent
    # vertex hands out particles.
    _Template_Drop = jinja2.Template(
        """
  for (int parentVertex = 0; parentVertex < TWO_POWER_D; parentVertex++) {
    if (marker.isParentVertexLocal(parentVertex)) {
      auto& parentParticles = coarseGridVertices{{PARTICLES_CONTAINER}}(parentVertex);
      auto particleIterator = parentParticles.begin();
      while (particleIterator != parentParticles.end()) {
        auto* const particle = *particleIterator;
#if PEANO_ASSERTIONS
        const int particleId = particle->getPartid();
#else
        const int particleId = -1;
#endif
        const bool particleIsLocal =
          particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local;
        const bool particleFitsFineLevel = toolbox::particles::dropParticle(*particle, marker);

        if (particleFitsFineLevel and marker.isParentOfSubtree()
            and toolbox::particles::particleWillBeDroppedFurther(*particle, marker)) {
          ++_numberOfDropsIntoHorizontalTreeDecomposition;
          // The sieve set takes ownership of the particle, so the coarse
          // container has to release it and the loop advances to the next one.
          particleIterator = parentParticles.particleCanNotBeLiftedLocally(particleIterator);
        } else if (particleFitsFineLevel) {
          ++_numberOfDrops;
          particle->setCellH(marker.getH());
          toolbox::particles::updateContainedInAdjacentCellProperty(marker, *particle);
          const auto parentVertexPosition =
            peano4::datamanagement::reconstructXOfParentVertex(marker, parentVertex);
          toolbox::particles::assignmentchecks::detachParticleFromVertex(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            parentVertexPosition,
            3.0 * marker.getH(),
            _treeId,
            "AbstractUpdateParticleGridAssociation::_Template_Drop"
          );
          toolbox::particles::assignmentchecks::assignParticleToVertex(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            marker.getX(),
            marker.getH(),
            _treeId,
            "AbstractUpdateParticleGridAssociation::_Template_Drop",
            parentVertexPosition,
            3.0 * marker.getH()
          );
          assertion(
            fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),
            fineGridVertex{{PARTICLES_CONTAINER}}.toString(),
            parentParticles.toString(),
            _treeId
          );
          assertion(
            parentParticles.isValid(),
            fineGridVertex{{PARTICLES_CONTAINER}}.toString(),
            parentParticles.toString(),
            _treeId
          );
          logDebug(
            "Drop particle " << particle->toString() << " from " << parentParticles.toString()
                             << " into " << marker.toString()
          );
          particleIterator = fineGridVertex{{PARTICLES_CONTAINER}}.grabParticle(
            particleIterator,
            parentParticles
          );
        } else {
          ++particleIterator;
        }
      }
    }
  }
"""
    )

    # A local parent vertex receives the particles directly. A particle whose
    # parent vertex is remote enters the sieve for the next traversal.
    _Template_LiftAllParticles = jinja2.Template(
        """
  std::bitset<DIMENSIONS> parentVertex;
  for (int dimension = 0; dimension < DIMENSIONS; dimension++) {
    parentVertex[dimension] = marker.getRelativePositionWithinFatherCell()(dimension) >= 2;
  }
  const int parentVertexNumber = static_cast<int>(parentVertex.to_ulong());

  if (marker.isParentVertexLocal(parentVertexNumber)) {
    auto& parentParticles = coarseGridVertices{{PARTICLES_CONTAINER}}(parentVertexNumber);
    assertion(
      parentParticles.isValid(),
      parentParticles.toString(),
      fineGridVertex{{PARTICLES_CONTAINER}}.toString()
    );
    assertion(
      fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),
      parentParticles.toString(),
      fineGridVertex{{PARTICLES_CONTAINER}}.toString()
    );
    _numberOfLifts += fineGridVertex{{PARTICLES_CONTAINER}}.size();
    const auto parentVertexPosition =
      peano4::datamanagement::reconstructXOfParentVertex(marker, parentVertexNumber);
    const auto parentCellSize = 3.0 * marker.getH();
    for (auto* const particle : fineGridVertex{{PARTICLES_CONTAINER}}) {
#if PEANO_ASSERTIONS
      const int particleId = particle->getPartid();
#else
      const int particleId = -1;
#endif
      const bool particleIsLocal =
        particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local;
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
        "AbstractUpdateParticleGridAssociation::_Template_LiftAllParticles (parent vertex is local)"
      );
      toolbox::particles::assignmentchecks::assignParticleToVertex(
        "{{PARTICLE}}",
        particle->getX(),
        particleId,
        particleIsLocal,
        parentVertexPosition,
        parentCellSize,
        _treeId,
        "AbstractUpdateParticleGridAssociation::_Template_LiftAllParticles (parent vertex is local)",
        marker.getX(),
        marker.getH()
      );
    }
    parentParticles.grabParticles(fineGridVertex{{PARTICLES_CONTAINER}});
  } else {
    _numberOfLiftsIntoSieveSet += fineGridVertex{{PARTICLES_CONTAINER}}.size();
    for (const auto* const particle : fineGridVertex{{PARTICLES_CONTAINER}}) {
#if PEANO_ASSERTIONS
      const int particleId = particle->getPartid();
#else
      const int particleId = -1;
#endif
      const bool particleIsLocal =
        particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local;
      toolbox::particles::assignmentchecks::detachParticleFromVertex(
        "{{PARTICLE}}",
        particle->getX(),
        particleId,
        particleIsLocal,
        marker.getX(),
        marker.getH(),
        _treeId,
        "AbstractUpdateParticleGridAssociation::_Template_LiftAllParticles"
      );
      toolbox::particles::assignmentchecks::assignParticleToSieveSet(
        "{{PARTICLE}}",
        particle->getX(),
        particleId,
        particleIsLocal,
        marker.getX(),
        marker.getH(),
        _treeId,
        "AbstractUpdateParticleGridAssociation::_Template_LiftAllParticles"
      );
    }
    fineGridVertex{{PARTICLES_CONTAINER}}.particlesCanNotBeLiftedLocally();
    assertion(
      fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),
      fineGridVertex{{PARTICLES_CONTAINER}}.toString()
    );
  }
"""
    )

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector<peano4::grid::GridControlEvent>{};\n"

    def user_should_modify_template(self):
        return False

    def get_includes(self):
        return jinja2.Template(
            """
#include "toolbox/particles/MultiscaleTransitions.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"

#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"

#include "Constants.h"
"""
        ).render(**self.d)

    def get_constructor_body(self):
        return super().get_constructor_body() + "\n  _treeId = treeNumber;\n"

    def get_attributes(self):
        return jinja2.Template(
            """
  int _treeId{-1};
  int _numberOfParticleReassignmentsOnCurrentLevel{0};
  int _numberOfLifts{0};
  int _numberOfDrops{0};
  int _numberOfLiftsIntoSieveSet{0};
  int _numberOfDropsFromSieveSet{0};
  int _numberOfDropsIntoHorizontalTreeDecomposition{0};
"""
        ).render(**self.d)

    def get_body_of_unprepareTraversal(self):
        """Advances the sieve exchange after all rank-local traversals finish."""
        return jinja2.Template(
            """
  vertexdata::{{PARTICLES_CONTAINER}}::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);
"""
        ).render(**self.d)
