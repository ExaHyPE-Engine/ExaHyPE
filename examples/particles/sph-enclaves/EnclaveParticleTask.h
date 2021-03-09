#ifndef _PEANO_ENCLAVE_PARTICLE_TASK_H_
#define _PEANO_ENCLAVE_PARTICLE_TASK_H_

#include "tarch/multicore/Tasks.h"
#include "peano4/datamanagement/VertexEnumerator.h"

#include <functional>
#include <forward_list>

namespace peano4 {
  template <class Particle, class VTC>
  class EnclaveParticleTask;

  template <class Particle, class VTC>
  class EnclaveParticleBookkeeping;
}

template <class Particle, class VTC>
class peano4::EnclaveParticleTask: public tarch::multicore::Task {
  private:
    friend class EnclaveParticleBookkeeping<Particle, VTC>;

    static tarch::logging::Log _log;

    const std::forward_list<Particle*> _activeParticles;
    const std::forward_list<Particle*> _localParticles;
    const peano4::datamanagement::VertexEnumerator<VTC> _taskCounters;

    std::function<void(
      const std::forward_list<Particle*>& activeParticles,
      const std::forward_list<Particle*>& localParticles
    )> _functor;

  public:
    EnclaveParticleTask(
      const std::forward_list<Particle*>& activeParticles,
      const std::forward_list<Particle*>& localParticles,
      const peano4::datamanagement::VertexEnumerator<VTC>& taskCounters,
      std::function<void(
        const std::forward_list<Particle*>& activeParticles,
        const std::forward_list<Particle*>& localParticles
      )> functor
    );

    EnclaveParticleTask(const EnclaveParticleTask& other) = delete;
    EnclaveParticleTask(const EnclaveParticleTask&& other) = delete;

    virtual ~EnclaveParticleTask() = default;

    bool run() override;

    // nop
    void prefetch() override;
};

#include "EnclaveParticleTask.cpph"

#endif
