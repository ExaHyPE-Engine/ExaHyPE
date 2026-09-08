// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org

#include "MeshSweepData.h"

peano4::particles::assignmentchecks::internal::MeshSweepData::MeshSweepData(
  const std::string& meshSweepName
):
  _meshSweepName(meshSweepName) {}

std::string peano4::particles::assignmentchecks::internal::MeshSweepData::
  getName() const {
  return _meshSweepName;
}
