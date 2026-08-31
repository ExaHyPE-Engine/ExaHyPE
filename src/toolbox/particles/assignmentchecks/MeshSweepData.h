// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/particles/assignmentchecks/Event.h"
#include "toolbox/particles/assignmentchecks/ParticleIdentifier.h"

namespace toolbox::particles::assignmentchecks::internal {
  class MeshSweepData;
} // namespace toolbox::particles::assignmentchecks::internal

class toolbox::particles::assignmentchecks::internal::MeshSweepData final {
public:
  MeshSweepData(const std::string& meshSweepName);

  std::string getName() const;

private:
  const std::string _meshSweepName;
};
