#include "TracingAPI.h"



namespace {
  tarch::logging::Log _log( "toolbox::particles::assignmentchecks" );
}


#if PeanoDebug>0

void toolbox::particles::assignmentchecks::endMeshSweep() {
  logInfo( "endMeshSweep()", "end mesh sweep and flush xxx events" );
}


void toolbox::particles::assignmentchecks::eraseParticle(
  const std::string&                             particleName,
  const tarch::la::Vector< Dimensions, double >& particleX,
  int                                            treeId
) {
  logInfo( "eraseParticle()", "erase " << particleName << " particle at " << particleX << " on tree " << treeId );
}

void toolbox::particles::assignmentchecks::assignParticleToVertex(
        const std::string&                         particleName,
        const tarch::la::Vector< Dimensions, double >& particleX,
        const tarch::la::Vector< Dimensions, double >& vertexX,
        int                                        vertexLevel,
        int                                        treeId
      ) {}

void toolbox::particles::assignmentchecks::removeParticleFromVertex(
        const std::string&                         particleName,
        const tarch::la::Vector< Dimensions, double >& particleX,
        const tarch::la::Vector< Dimensions, double >& vertexX,
        int                                        vertexLevel,
        int                                        treeId
      ) {}

void toolbox::particles::assignmentchecks::assignParticleToSieveSet(
        const std::string&                         particleName,
        const tarch::la::Vector< Dimensions, double >& particleX,
        int                                        treeId
      ) {}


#endif
