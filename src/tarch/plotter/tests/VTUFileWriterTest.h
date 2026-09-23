// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/tests/TestCase.h"

namespace tarch::plotter::tests {
  class VTUFileWriterTest;
} // namespace tarch::plotter::tests

class tarch::plotter::tests::VTUFileWriterTest final : public tarch::tests::TestCase {
private:
  void test2DSimpleTriangle();
  void test3DSimpleHexahedron();
  void test2DParticlesWithData();
  void test3DCellData();
  void testCellTypeMetadata();

  /**
   * An untouched plotter produces a valid empty VTU file.
   */
  void testEmptyPlotter();

  /**
   * One subdomain holds data, another is empty.
   */
  void testMixedEmptySubdomains();

  /**
   * Multiple non-empty local subdomains are written as separate pieces.
   */
  void testMultipleNonEmptySubdomains();

  /**
   * Writing more data tuples than geometry elements must throw.
   */
  void testOversizedDataThrows();

  /**
   * Two non-empty subdomains with conflicting field schemas (same name, different
   * component count) must be rejected.
   */
  void testInconsistentSchemaThrows();

  /**
   * Ranks hold different non-zero subdomain counts (master 3, the rest 2). VTU
   * writes one .vtu per rank with one <Piece> per subdomain, so each rank's file
   * must carry exactly its own subdomain count - the per-rank analogue of the
   * collective writers' uneven-distribution test.
   */
  void testUnevenSubdomainCounts();

  /**
   * A data identifier with a character that would corrupt the XML is rejected up
   * front by FileWriter::validateIdentifier.
   */
  void testInvalidIdentifierThrows();

public:
  VTUFileWriterTest();
  ~VTUFileWriterTest() override = default;

  void run() override;
};
