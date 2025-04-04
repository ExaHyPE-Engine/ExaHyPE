// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "repositories/SolverRepository.h"

#include "tarch/multicore/Task.h"

#include "peano4/datamanagement/CellMarker.h"

#include "exahype2/EnclaveBookkeeping.h"
#include "exahype2/EnclaveTask.h"

{{SOLVER_INCLUDES}}

#include <vector>
#include <memory>

{% for item in NAMESPACE -%}
  namespace {{ item }} {

{%- endfor %}
  class {{CLASSNAME}};

{% for item in NAMESPACE -%}
  }
{%- endfor %}

/**
 * Single task that can also take multiple tasks and deploy them to the GPU
 *
 * @author Tobias Weinzierl
 */
class {{NAMESPACE | join("::")}}::{{CLASSNAME}} final : public ::exahype2::EnclaveTask<{{STORAGE_PRECISION}}> {
  private:
    static tarch::logging::Log  _log;

    /**
     * This is a unique static id which we use for task fusion
     * to distinguish different task types.
     */
    static int _enclaveTaskTypeId;

  public:
    static int getEnclaveTaskTypeId();

    /**
     * Simple call to compute kernels.
     *
     * The finite volume solver assumes that _inputValues are reconstructed
     * values, i.e., a temporary data field. So we don't have to allocate
     * anything but we have to free stuff after the run. This is done here in
     * the Finite Volume solver.
     *
     * @return Max eigenvalue over cell
     */
    static double applyKernelToCell(
      const ::peano4::datamanagement::CellMarker& marker,
      double                                      timeStamp,
      double                                      timeStepSize,
      {{STORAGE_PRECISION}}* __restrict__         reconstructedPatch,
      {{STORAGE_PRECISION}}* __restrict__         targetPatch
    );

    /**
     * Task constructor
     *
     * The task construction injects a call to applyKernelToCell() into
     * the generic enclave superclass. We assume that reconstructedPatch
     * is created on the heap before - typically in the actual action
     * set. As applyKernelToCell() erases the
     * reconstructedPatch argument, we don't have to call a free() in the
     * task constructor of the functor that we pass to the enclave
     * task (superclass). Likewise, the superclass EnclaveTask takes
     * care of the allocation of the output data if no output (nullptr)
     * is specified.
     */
    {{CLASSNAME}}(
      const ::peano4::datamanagement::CellMarker& marker,
      double                                      timeStamp,
      double                                      timeStepSize,
      {{STORAGE_PRECISION}}* __restrict__         oldQWithHalo,
      {{STORAGE_PRECISION}}* __restrict__         newQ
    );

    ~{{CLASSNAME}}() override final = default;

    virtual bool canFuse() const override;

    {% if STATELESS_PDE_TERMS %}
    virtual bool fuse(const std::list<Task*>& otherTasks, int targetDevice = tarch::accelerator::Device::HostDevice) override;
    {% endif %}
};

{# Empty line here #}
