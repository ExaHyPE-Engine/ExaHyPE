// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::tests {
  class ProjectPatchTest;
} // namespace exahype2::tests

class exahype2::tests::ProjectPatchTest final : public tarch::tests::TestCase {
public:
  ProjectPatchTest();
  ProjectPatchTest(const ProjectPatchTest&)            = delete;
  ProjectPatchTest(ProjectPatchTest&&)                 = delete;
  ProjectPatchTest& operator=(const ProjectPatchTest&) = delete;
  ProjectPatchTest& operator=(ProjectPatchTest&&)      = delete;
  ~ProjectPatchTest() override                         = default;

  void run() override;

private:
  void testProjectPatchFillsFaces();
};
