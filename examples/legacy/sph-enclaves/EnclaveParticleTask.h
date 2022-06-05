#ifndef _PEANO_ENCLAVE_PARTICLE_TASK_H_
#define _PEANO_ENCLAVE_PARTICLE_TASK_H_

#include "tarch/multicore/Tasks.h"
#include "peano4/datamanagement/VertexEnumerator.h"

#include <functional>
#include <forward_list>
#include <array>

namespace peano4 {
  template <class Particle>
  class EnclaveParticleTask;

  template <class Particle>
  class EnclaveParticleBookkeeping;
}

template <class Particle>
class peano4::EnclaveParticleTask: public tarch::multicore::Task {
  private:
    friend class EnclaveParticleBookkeeping<Particle>;

    static tarch::logging::Log _log;

    std::forward_list<Particle*> _activeParticles;
    std::forward_list<Particle*> _localParticles;
    const std::array<int, TwoPowerD> _vertexIds;

    std::function<void(
      std::forward_list<Particle*>& activeParticles,
      std::forward_list<Particle*>& localParticles
    )> _functor;

  public:
    EnclaveParticleTask(
      std::forward_list<Particle*>& activeParticles,
      std::forward_list<Particle*>& localParticles,
      const std::array<int, TwoPowerD>& vertexIds,
      std::function<void(
        std::forward_list<Particle*>& activeParticles,
        std::forward_list<Particle*>& localParticles
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
