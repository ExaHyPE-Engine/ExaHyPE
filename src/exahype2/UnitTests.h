// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once
#ifndef _EXAHYPE2_UNIT_TESTS_H_
#define _EXAHYPE2_UNIT_TESTS_H_

#include "tarch/tests/TestCase.h"
#include "tarch/tests/TreeTestCaseCollection.h"

#include "exahype2/aderdg/tests/ADERDGTest.h"
#include "exahype2/dg/tests/CellIntegralTest.h"
#include "exahype2/dg/tests/DGUtilsTest.h"
#include "exahype2/dg/tests/RiemannTest.h"
#include "exahype2/dg/rusanov/tests/RiemannTest.h"
#include "exahype2/fv/rusanov/tests/ApplySplit1DRiemannToPatchTest.h"
#include "exahype2/fv/rusanov/tests/CopyPatchTest.h"

namespace exahype2 {
  /**
   * Please destroy after usage
   */
  tarch::tests::TestCase* getUnitTests();
}

#endif
