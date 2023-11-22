// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <functional>

#include "tarch/multicore/Tasks.h"


namespace swift2 {
  /**
   * A task tied logically to a vertex within the mesh
   *
   * Vertex tasks can either be spawned by touchVertexFirstTime() or
   * touchVertexLastTime(). As they are Swift tasks, they need a unique
   * identifier such that we can construct dependency graphs. The task itself
   * has to be generated on the heap via a new, and you may never ever create
   * such a task for a vertex adjacent to the domain boundary if you are
   * thinking of a touchVertexLastTime() task.
   *
   * The task backs up the active and the local set. When it runs, it
   * invokes the functor as passed. As we don't know the particle type at
   * this point, active and passive set are written down as templates over an
   * unknown particle type. This task does not exploit any vectorisation in any
   * way.
   */
  template <typename ParticleSet>
  class VertexTask: public tarch::multicore::Task {
  public:
    typedef std::function<void(ParticleSet& assignedParticles)> Functor;

    /**
     * Construct a vertex task
     *
     * Memorize the three parameters, and grab a new task number from the
     * multicore component via
     */
    VertexTask(
      const ParticleSet&  assignedParticles,
	  Functor             functor,
      bool               FuseAndPortToGPU
    );
    ~VertexTask() = default;

    virtual bool run() override;

  private:
    static int TaskTypeId;

    ParticleSet _assignedParticles;
    Functor     _functor;
  };
} // namespace swift2


#include "VertexTask.cpph"
