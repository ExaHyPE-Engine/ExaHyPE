#include "EnclaveParticleTask.h"
#include "EnclaveParticleBookkeeping.h"

#include "tarch/multicore/Core.h"


tarch::logging::Log  peano4::EnclaveParticleTask::_log( "peano4::EnclaveParticleTask" );


peano4::EnclaveParticleTask::EnclaveParticleTask(
  const std::forward_list<enclavesph::globaldata::Particle*>& activeParticles,
  const std::forward_list<enclavesph::globaldata::Particle*>& localParticles,
  const peano4::datamanagement::VertexEnumerator<enclavesph::vertexdata::VertexTaskCounter>& taskCounters,
  std::function<void(
    const std::forward_list<enclavesph::globaldata::Particle*>& activeParticles,
    const std::forward_list<enclavesph::globaldata::Particle*>& localParticles
  )> functor
):
  tarch::multicore::Task(tarch::multicore::reserveTaskNumber(),tarch::multicore::Task::DefaultPriority),
  _activeParticles(activeParticles),
  _localParticles(localParticles),
  _taskCounters(taskCounters),
  _functor(functor)
{
  logTraceIn( "EnclaveParticleTask(...)" );
  logTraceOut( "EnclaveParticleTask(...)" );
}


bool peano4::EnclaveParticleTask::run() {
  logTraceIn( "run()" );

  _functor(_activeParticles, _localParticles);

  EnclaveParticleBookkeeping::getInstance().finishedTask(getTaskId(), _taskCounters);

  logTraceOut( "run()" );
  return false;
}


void peano4::EnclaveParticleTask::prefetch() {
  logTraceIn( "prefetch()" );
  logTraceOut( "prefetch()" );
}
