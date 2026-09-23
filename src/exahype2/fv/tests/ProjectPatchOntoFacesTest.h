// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace exahype2::fv::tests {
  class ProjectPatchOntoFacesTest;
} // namespace exahype2::fv::tests

class exahype2::fv::tests::ProjectPatchOntoFacesTest final : public tarch::tests::TestCase {
public:
  ProjectPatchOntoFacesTest();
  ProjectPatchOntoFacesTest(const ProjectPatchOntoFacesTest&)            = delete;
  ProjectPatchOntoFacesTest(ProjectPatchOntoFacesTest&&)                 = delete;
  ProjectPatchOntoFacesTest& operator=(const ProjectPatchOntoFacesTest&) = delete;
  ProjectPatchOntoFacesTest& operator=(ProjectPatchOntoFacesTest&&)      = delete;
  ~ProjectPatchOntoFacesTest() override                                  = default;

  void run() override;

private:
  void testProjectionFillsTheHalfOfEveryFaceTheCellOwns();
  void testProjectionStampsTheFaceLabels();
};
