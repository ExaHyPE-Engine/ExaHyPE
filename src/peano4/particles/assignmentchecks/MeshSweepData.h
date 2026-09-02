// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/particles/assignmentchecks/Event.h"
#include "peano4/particles/assignmentchecks/ParticleIdentifier.h"

namespace peano4::particles::assignmentchecks::internal {
  class MeshSweepData;
} // namespace peano4::particles::assignmentchecks::internal

class peano4::particles::assignmentchecks::internal::MeshSweepData final {
public:
  MeshSweepData(const std::string& meshSweepName);

  std::string getName() const;

private:
  const std::string _meshSweepName;
};
