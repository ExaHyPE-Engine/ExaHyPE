# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.solversteps.ActionSet import ActionSet


class AbstractUpdateParticleGridAssociation(ActionSet):
    """Maintains the dual-tree association between particles and vertices.

    Each particle is stored at the closest vertex on the finest level that can
    accommodate its search radius. The subclasses implement different sorting
    strategies for this relation:

    peano4.particles.UpdateParticleGridAssociation_Reassign,
    peano4.particles.UpdateParticleGridAssociation_LiftDrop, or
    peano4.particles.UpdateParticleGridAssociation_BucketSort.

    @image html dual-tree.png

    Changing a particle position or search radius invalidates its association
    until a sorting traversal completes. Neighbourhood queries are invalid during
    this interval. The action set invokes finishedTraversal() after every rank has
    completed its local trees so the sieve exchange advances to the next sweep.

    ## Domain decomposition

    A caller combines a sorting action with
    peano4.particles.UpdateParallelState. Sorting requires at least two sweeps:
    the first lifts misplaced particles or inserts them into a sieve, and following
    sweeps exchange and drop them. A horizontal tree cut can require one additional
    sweep per crossed level.

    A caller gives sorting a low descend_invocation_order. Drops then precede
    particle updates during descent, while reassignment and lifts follow updates
    during backtracking.

    ## Particle interactions during sorting

    Persistent vertices next to an AMR boundary retain particles whose finer
    destination would be a hanging vertex. These particles therefore participate
    in multilevel interactions.

    @image html SievingAlongAMRBoundaries.png

    A particle that drops during an interaction sweep can contribute on both
    levels. A caller disables interactions during such a sweep unless the
    interaction update can remove the contribution accumulated before the drop.

    ## Sieving

    Particles use a type-wide toolbox::particles::SieveParticles instance when
    they cross a horizontal tree cut or move farther than a local lift can handle.
    Bucket sorting uses the sieve for every transition; the other variants use it
    only when a transition cannot remain on the current tree.

    Sorting changes particle-list storage. A continuous particle memory pool must
    gather and resort its lists around this action. See
    @ref toolbox_particles_memorypool and @ref page_toolbox_particles_mesh_consistency.
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
        particle->getX(),
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

    """Drops fitting particles from a local parent vertex.

    A vertical tree cut can make a parent vertex remote, so only local parent
    vertices may provide particles. A drop into a horizontal cut is copied to
    the next sweep's sieve because the child tree is unavailable locally.

    @image html UpdateParticleGridAssociation_LiftDrop.png
    """
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
          fineGridVertex{{PARTICLES_CONTAINER}}.particlesCanNotBeDroppedLocallySoRollOverForNextMeshSweep(
            particle
          );
          ++particleIterator;
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
            _spacetreeId,
            "AbstractUpdateParticleGridAssociation::_Template_Drop"
          );
          toolbox::particles::assignmentchecks::assignParticleToVertex(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            marker.getX(),
            marker.getH(),
            _spacetreeId,
            "AbstractUpdateParticleGridAssociation::_Template_Drop",
            parentVertexPosition,
            3.0 * marker.getH()
          );
          assertion(
            fineGridVertex{{PARTICLES_CONTAINER}}.isValid(),
            fineGridVertex{{PARTICLES_CONTAINER}}.toString(),
            parentParticles.toString(),
            _spacetreeId
          );
          assertion(
            parentParticles.isValid(),
            fineGridVertex{{PARTICLES_CONTAINER}}.toString(),
            parentParticles.toString(),
            _spacetreeId
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

    """Lifts every particle before its vertex is destroyed.

    A local parent vertex receives the particles directly. Particles whose
    parent vertex is remote enter the sieve for the next traversal.
    """
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
        _spacetreeId,
        "AbstractUpdateParticleGridAssociation::_Template_LiftAllParticles (parent vertex is local)"
      );
      toolbox::particles::assignmentchecks::assignParticleToVertex(
        "{{PARTICLE}}",
        particle->getX(),
        particleId,
        particleIsLocal,
        parentVertexPosition,
        parentCellSize,
        _spacetreeId,
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
        _spacetreeId,
        "AbstractUpdateParticleGridAssociation::_Template_LiftAllParticles"
      );
      toolbox::particles::assignmentchecks::assignParticleToSieveSet(
        "{{PARTICLE}}",
        particle->getX(),
        particleId,
        particleIsLocal,
        marker.getX(),
        marker.getH(),
        _spacetreeId,
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
        """Provides particle sorting and generated-record dependencies."""
        return jinja2.Template(
            """
#include "tarch/multicore/Lock.h"

#include "toolbox/particles/MultiscaleTransitions.h"
#include "toolbox/particles/assignmentchecks/TracingAPI.h"

#include "vertexdata/{{PARTICLES_CONTAINER}}.h"
#include "globaldata/{{PARTICLE}}.h"

#include "Constants.h"
"""
        ).render(**self.d)

    def get_constructor_body(self):
        return super().get_constructor_body() + "\n  _spacetreeId = treeNumber;\n"

    def get_attributes(self):
        return jinja2.Template(
            """
  int _spacetreeId;
  int _numberOfParticleReassignmentsOnCurrentLevel;
  int _numberOfLifts;
  int _numberOfDrops;
  int _numberOfLiftsIntoSieveSet;
  int _numberOfDropsFromSieveSet;
  int _numberOfDropsIntoHorizontalTreeDecomposition;
"""
        ).render(**self.d)

    def get_body_of_unprepareTraversal(self):
        """Advances the sieve exchange after all rank-local traversals finish."""
        return jinja2.Template(
            """
  vertexdata::{{PARTICLES_CONTAINER}}::finishedTraversal(DomainOffset, DomainSize, PeriodicBC);
"""
        ).render(**self.d)
