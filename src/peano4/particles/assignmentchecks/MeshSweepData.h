// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace peano4::particles::assignmentchecks {
  class MeshSweepData;
} // namespace peano4::particles::assignmentchecks

/**
 * Names one mesh sweep of the assignment history.
 */
class peano4::particles::assignmentchecks::MeshSweepData final {
public:
  MeshSweepData(const std::string& meshSweepName);

  [[nodiscard]] std::string getName() const;

private:
  const std::string _meshSweepName;
};
