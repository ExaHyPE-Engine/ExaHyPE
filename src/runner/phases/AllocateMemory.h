// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Phase.h"

namespace runner {
  class AllocateMemory;
  class ReportBox;
} // namespace runner

/**
 * Allocates the pooled solver data in one batch between grid construction and
 * grid initialisation. Grid construction sizes the slot vectors only, so the
 * demand is known here and one line reports it before the buffers are
 * allocated. The phase broadcasts the allocation step, so every rank binds its
 * pools.
 */
class runner::AllocateMemory final : public runner::Phase {
public:
  explicit AllocateMemory(runner::PhaseContext& context);

  void run() override;

  static void executeOnEveryRank(runner::ReportBox& box);

private:
  runner::PhaseContext& _context;
};
