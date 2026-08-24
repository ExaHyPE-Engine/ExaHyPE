# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2

from peano4.solversteps.ActionSet import ActionSet

from .AbstractUpdateParticleGridAssociation import AbstractUpdateParticleGridAssociation
from .UpdateParticleGridAssociation_LiftDrop import (
    UpdateParticleGridAssociation_LiftDrop,
)


class UpdateParticleGridAssociation_Reassign(AbstractUpdateParticleGridAssociation):
    """!

    Associate moving particles with spacetree vertices.

    Drops run in touchVertexFirstTime(). Same-level reassignment and lifts run
    in touchCellLastTime(). Particle positions therefore have to remain fixed
    after touchVertexFirstTime() until the traversal ends.

    A lift that crosses a tree boundary enters the sieve because its destination
    level is unavailable during the local traversal. Sieve particles are
    exchanged after the traversal and reconsidered in the next sweep. Particle
    sorting can therefore require more than one traversal, including when a
    particle crosses multiple cells in one time step.

    ## Hanging vertices

    Hanging vertices release their particles when they are destroyed. Setting
    drop_into_hanging_vertices to false suppresses drops into newly created
    hanging vertices, but the destruction lift remains necessary because a
    same-level reassignment can still target one.

    ## Where and how to use

    Add this action set to every traversal that can encounter its particle set.
    Adding it before particle-update action sets makes drops precede user code
    and, because Peano reverses action-set order during backtracking, makes
    reassignment and lifts follow user code.

    The action changes the particle-list layout and is incompatible with a
    continuous particle memory pool unless the traversal explicitly gathers
    and resorts the storage. See @ref toolbox_particles_memorypool.

    @param drop_into_hanging_vertices Enable first-touch drops into hanging
      vertices.
    """

    def __init__(self, particle_set, guard="true", drop_into_hanging_vertices=True):
        super().__init__(particle_set, guard)
        self._drop_into_hanging_vertices = drop_into_hanging_vertices

    _Template_Sieve = UpdateParticleGridAssociation_LiftDrop._Template_Sieve

    _Template_ReassignAndLiftWithinCell = jinja2.Template(
        """
  for (int sourceVertex = 0; sourceVertex < TWO_POWER_D; ++sourceVertex) {
    auto& sourceParticles = fineGridVertices{{PARTICLES_CONTAINER}}(sourceVertex);
    auto particleIterator = sourceParticles.begin();
    const auto sourceVertexPosition =
      marker.getOffset()
      + tarch::la::multiplyComponents(
        tarch::la::Vector<DIMENSIONS, double>(std::bitset<DIMENSIONS>(sourceVertex)),
        marker.getH()
      );
    while (particleIterator != sourceParticles.end()) {
      auto* const particle = *particleIterator;
      const auto instruction =
        toolbox::particles::getParticleReassociationInstructionWithinCellWithIntraCellReassignment(
          *particle,
          marker,
          sourceVertex
        );

#if PEANO_ASSERTIONS
      const int particleId = particle->getPartid();
#else
      const int particleId = -1;
#endif
      const bool particleIsLocal =
        particle->getParallelState() == globaldata::{{PARTICLE}}::ParallelState::Local;

      switch (instruction) {
        case toolbox::particles::ParticleReassociationInstruction_Keep:
          toolbox::particles::updateContainedInAdjacentCellProperty(
            marker,
            std::bitset<DIMENSIONS>(sourceVertex),
            *particle
          );
          ++particleIterator;
          break;
        case toolbox::particles::ParticleReassociationInstruction_SieveGlobally:
          logDebug(
            "Particle " << particle->toString()
                        << " enters the global sieve from " << marker.toString()
          );
          ++_numberOfLiftsIntoSieveSet;
          toolbox::particles::assignmentchecks::detachParticleFromVertex(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            sourceVertexPosition,
            marker.getH(),
            _spacetreeId,
            "UpdateParticleGridAssociation_Reassign::_Template_ReassignAndLiftWithinCell"
          );
          toolbox::particles::assignmentchecks::assignParticleToSieveSet(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            sourceVertexPosition,
            marker.getH(),
            _spacetreeId,
            "UpdateParticleGridAssociation_Reassign::_Template_ReassignAndLiftWithinCell"
          );
          particleIterator = sourceParticles.particleCanNotBeLiftedLocally(particleIterator);
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
          const auto targetVertexPosition =
            marker.getOffset()
            + tarch::la::multiplyComponents(
              tarch::la::Vector<DIMENSIONS, double>(std::bitset<DIMENSIONS>(instruction)),
              marker.getH()
            );
          toolbox::particles::updateContainedInAdjacentCellProperty(
            targetVertexPosition,
            marker.getH(),
            *particle
          );
          toolbox::particles::assignmentchecks::detachParticleFromVertex(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            sourceVertexPosition,
            marker.getH(),
            _spacetreeId,
            "UpdateParticleGridAssociation_Reassign::_Template_ReassignAndLiftWithinCell"
          );
          toolbox::particles::assignmentchecks::assignParticleToVertex(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            targetVertexPosition,
            marker.getH(),
            _spacetreeId,
            "UpdateParticleGridAssociation_Reassign::_Template_ReassignAndLiftWithinCell",
            sourceVertexPosition,
            marker.getH(),
            false,
            true
          );
          particleIterator = fineGridVertices{{PARTICLES_CONTAINER}}(instruction).grabParticle(
            particleIterator,
            sourceParticles
          );
          ++_numberOfParticleReassignmentsOnCurrentLevel;
        } break;
        case 0 + TWO_POWER_D:
        case 1 + TWO_POWER_D:
        case 2 + TWO_POWER_D:
        case 3 + TWO_POWER_D:
#if DIMENSIONS == 3
        case 4 + TWO_POWER_D:
        case 5 + TWO_POWER_D:
        case 6 + TWO_POWER_D:
        case 7 + TWO_POWER_D:
#endif
        {
          const int coarseVertex = instruction - TWO_POWER_D;
          const auto coarseCellSize = 3.0 * marker.getH();
          const auto coarseCellOffset =
            marker.getInvokingParentCellsCentre() - 0.5 * coarseCellSize;
          const auto targetVertexPosition =
            coarseCellOffset
            + tarch::la::multiplyComponents(
              tarch::la::Vector<DIMENSIONS, double>(std::bitset<DIMENSIONS>(coarseVertex)),
              coarseCellSize
            );
          particle->setCellH(coarseCellSize);
          toolbox::particles::updateContainedInAdjacentCellProperty(
            targetVertexPosition,
            coarseCellSize,
            *particle
          );
          toolbox::particles::assignmentchecks::detachParticleFromVertex(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            sourceVertexPosition,
            marker.getH(),
            _spacetreeId,
            "UpdateParticleGridAssociation_Reassign::_Template_ReassignAndLiftWithinCell"
          );
          toolbox::particles::assignmentchecks::assignParticleToVertex(
            "{{PARTICLE}}",
            particle->getX(),
            particleId,
            particleIsLocal,
            targetVertexPosition,
            coarseCellSize,
            _spacetreeId,
            "UpdateParticleGridAssociation_Reassign::_Template_ReassignAndLiftWithinCell",
            sourceVertexPosition,
            marker.getH()
          );
          particleIterator = coarseGridVertices{{PARTICLES_CONTAINER}}(coarseVertex).grabParticle(
            particleIterator,
            sourceParticles
          );
          ++_numberOfLifts;
        } break;
        default:
          assertion(false, "Unsupported particle reassociation instruction.", instruction);
          ++particleIterator;
          break;
      }
    }
  }
"""
    )

    def get_body_of_operation(self, operation_name):
        """Render the operation selected by the traversal event."""
        result = ""
        if operation_name == ActionSet.OPERATION_BEGIN_TRAVERSAL:
            result = self._Template_BeginTraversal.render(**self.d)
        elif (
            self._drop_into_hanging_vertices
            and operation_name == ActionSet.OPERATION_TOUCH_HANGING_VERTEX_FIRST_TIME
        ):
            result = self._Template_Drop.render(**self.d)
            result += self._Template_Sieve.render(**self.d)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME:
            result = self._Template_Drop.render(**self.d)
            result += self._Template_Sieve.render(**self.d)
        elif operation_name == ActionSet.OPERATION_TOUCH_VERTEX_LAST_TIME:
            result = self._Template_ValidateParticleLevelAssociation.render(**self.d)
        elif operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self._Template_ReassignAndLiftWithinCell.render(**self.d)
        elif operation_name == ActionSet.OPERATION_DESTROY_VERTEX:
            result = self._Template_LiftAllParticles.render(**self.d)
        elif operation_name == ActionSet.OPERATION_END_TRAVERSAL:
            result = self._Template_EndTraversal.render(**self.d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
