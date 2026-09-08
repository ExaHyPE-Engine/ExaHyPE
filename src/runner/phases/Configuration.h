// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Phase.h"

namespace runner {
  class Configuration;
} // namespace runner

class runner::Configuration final : public runner::Phase {
public:
  explicit Configuration(runner::PhaseContext& context);

  void run() override;

private:
  runner::PhaseContext&    _context;
  std::vector<std::string> _banners;
  std::vector<std::string> _body;
};
