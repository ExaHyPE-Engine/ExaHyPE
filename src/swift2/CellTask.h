// This file is part of the SWIFT2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <functional>

#include "tarch/multicore/Tasks.h"
#include "peano4/datamanagement/CellMarker.h"


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
   *
   * ## Design rationale
   *
   * One might initially think that this task class is not required, as we only
   * wrap the TaskWithCopyOfFunctor. However, we add a few more bits and pieces
   * such as the GPU-offload and fusion capabilities. Furthermore, it is nice to
   * see the copy semantics preserved explicitly, i.e. which objects are explicitly
   * preserved and handed over.
   */
  template <typename ParticleSet>
  class CellTask: public tarch::multicore::Task {
  public:
    typedef std::function<void(const peano4::datamanagement::CellMarker&  marker, ParticleSet& localParticles, ParticleSet& activeParticles)> Functor;

    /**
     * Construct a vertex task
     *
     * Memorize the three parameters, and grab a new task number from the
     * multicore component via
     */
    CellTask(
      const peano4::datamanagement::CellMarker&  marker,
      const ParticleSet&                         localParticles,
      const ParticleSet&                         activeParticles,
      Functor                                    functor,
      bool                                       fuseAndPortToGPU
    );
    ~CellTask();

    /**
     * Run the task
     *
     * This routine invokes the actual task functor. After that, it sets the
     * updated marker for the particles. We see that t
     */
    virtual bool run() override;

  private:
    static int TaskTypeId;

    const peano4::datamanagement::CellMarker  _marker;
    ParticleSet _localParticles;
    ParticleSet _activeParticles;
    Functor     _functor;
  };
} // namespace swift2


#include "CellTask.cpph"
