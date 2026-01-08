// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class VTUFileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::VTUFileWriterTest final: public tarch::tests::TestCase {
private:
  void test2DSimpleTriangle();
  void test3DSimpleHexahedron();
  void test2DParticlesWithData();
  void test3DCellData();
  void test2DAMRGrid();
  void test3DAMRGrid();

  /**
   * Checks if an untouched plotter produces a valid empty VTU file.
   */
  void testEmptyPlotter();

  /**
   * Checks if we can handle one rank having data and another (simulated subdomain) being empty.
   */
  void testMixedEmptySubdomains();

  /**
   * Verifies that the plotter throws an exception if we try to write
   * more data tuples than there are geometry elements.
   */
  void testOversizedDataThrows();

  /**
   * Verifies consistency check: if we define a field on one subdomain,
   * others must match the schema (even if empty, though empty usually skips validation).
   * We test a case where two non-empty subdomains have conflicting fields.
   */
  void testInconsistentSchemaThrows();

public:
  VTUFileWriterTest();
  ~VTUFileWriterTest() override = default;

  void run() override;
};
