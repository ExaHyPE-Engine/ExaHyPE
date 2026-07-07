// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Phase.h"

namespace runner {
  class FinalSummary;
} // namespace runner

class runner::FinalSummary final : public runner::Phase {
public:
  explicit FinalSummary(PhaseContext& context);

  void run() override;

private:
  PhaseContext& _context;
};
