// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CellTypeTest.h"

#include "peano4/grid/CellType.h"
#include "peano4/grid/VertexType.h"

peano4::grid::tests::CellTypeTest::CellTypeTest():
  TestCase("peano4::grid::tests::CellTypeTest") {}

void peano4::grid::tests::CellTypeTest::run() { testMethod(testGetCellType); }

void peano4::grid::tests::CellTypeTest::testGetCellType() {
  VertexType vertexTypes[TWO_POWER_D];

  auto fill = [&](VertexType t) {
    for (int i = 0; i < TWO_POWER_D; ++i) {
      vertexTypes[i] = t;
    }
  };

  auto fillAllNewOrHanging = [&]() {
    fill(VertexType::Hanging);
    vertexTypes[0] = VertexType::New; // Ensure at least one New
  };

  auto fillAllDeleteOrHanging = [&]() {
    fill(VertexType::Hanging);
    vertexTypes[0] = VertexType::Delete; // Ensure at least one Delete
  };

  // All vertices persistent -> cell persistent
  fill(VertexType::Persistent);
  validateEquals(CellType::Persistent, getCellType(vertexTypes));

  // All vertices new -> cell new
  fill(VertexType::New);
  validateEquals(CellType::New, getCellType(vertexTypes));

  // All vertices delete -> cell delete
  fill(VertexType::Delete);
  validateEquals(CellType::Delete, getCellType(vertexTypes));

  // All vertices are new or hanging (and not all hanging) -> cell new
  fillAllNewOrHanging();
  validateEquals(CellType::New, getCellType(vertexTypes));

  // All vertices are delete or hanging (and not all hanging) -> cell delete
  fillAllDeleteOrHanging();
  validateEquals(CellType::Delete, getCellType(vertexTypes));

  // Mixture of New and Delete (and maybe Hanging), but no Persistent -> cell persistent
  // (neither "all new/hanging" nor "all delete/hanging" holds)
  fill(VertexType::Hanging);
  vertexTypes[0] = VertexType::New;
  vertexTypes[1] = VertexType::Delete;
  validateEquals(CellType::Persistent, getCellType(vertexTypes));

  // Any Persistent vertex forces Persistent
  fillAllNewOrHanging();
  vertexTypes[2] = VertexType::Persistent;
  validateEquals(CellType::Persistent, getCellType(vertexTypes));

  // Persistent with deletes/hanging still persistent
  fillAllDeleteOrHanging();
  vertexTypes[3] = VertexType::Persistent;
  validateEquals(CellType::Persistent, getCellType(vertexTypes));
}
