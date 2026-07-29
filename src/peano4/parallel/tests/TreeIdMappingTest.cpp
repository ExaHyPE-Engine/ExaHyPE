// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TreeIdMappingTest.h"

#include "peano4/parallel/TreeIdMapping.h"
#include "tarch/mpi/Rank.h"

peano4::parallel::tests::TreeIdMappingTest::TreeIdMappingTest() :
  TestCase("peano4::parallel::tests::TreeIdMappingTest") {}

void peano4::parallel::tests::TreeIdMappingTest::testIdArithmetic() {
  const int numberOfRanks = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  const int myRank        = tarch::mpi::Rank::getInstance().getRank();

  constexpr std::array<int, 3> LocalTreeIds{0, 1, MAX_SPACETREES_PER_RANK - 1};
  for (const int localTreeId : LocalTreeIds) {
    const int globalTreeId = peano4::parallel::TreeIdMapping::getGlobalTreeId(localTreeId);
    validateEquals(peano4::parallel::TreeIdMapping::getLocalTreeId(globalTreeId), localTreeId);
    validateEquals(peano4::parallel::TreeIdMapping::getRank(globalTreeId), myRank);

    for (int rank = 0; rank < numberOfRanks; rank++) {
      const int treeId = localTreeId * numberOfRanks + rank;
      validateEquals(peano4::parallel::TreeIdMapping::getRank(treeId), rank);
      validateEquals(peano4::parallel::TreeIdMapping::getLocalTreeId(treeId), localTreeId);
    }
  }

  validate(peano4::parallel::TreeIdMapping::isGlobalMaster(0));
  validate(not peano4::parallel::TreeIdMapping::isGlobalMaster(1));
}

void peano4::parallel::tests::TreeIdMappingTest::testReserveAndDeregister() {
  auto&         treeIdMapping        = peano4::parallel::TreeIdMapping::getInstance();
  const int     myRank               = tarch::mpi::Rank::getInstance().getRank();
  const int     initialNumberOfTrees = treeIdMapping.getNumberOfRegisteredTrees();
  constexpr int NumberOfReservedIds  = 3;

  const auto reservedIds = treeIdMapping.reserveIds(NumberOfReservedIds);
  validateEquals(static_cast<int>(reservedIds.size()), NumberOfReservedIds);
  validateEquals(treeIdMapping.getNumberOfRegisteredTrees(), initialNumberOfTrees + NumberOfReservedIds);
  for (std::size_t i = 0; i < reservedIds.size(); i++) {
    validateEquals(peano4::parallel::TreeIdMapping::getRank(reservedIds[i]), myRank);
    if (i > 0) {
      validateWithParams2(reservedIds[i - 1] < reservedIds[i], reservedIds[i - 1], reservedIds[i]);
    }
  }

  const int additionalId = treeIdMapping.reserveId();
  validateEquals(peano4::parallel::TreeIdMapping::getRank(additionalId), myRank);
  validateEquals(treeIdMapping.getNumberOfRegisteredTrees(), initialNumberOfTrees + NumberOfReservedIds + 1);

  treeIdMapping.deregisterId(additionalId);
  for (const int reservedId : reservedIds) {
    treeIdMapping.deregisterId(reservedId);
  }
  validateEquals(treeIdMapping.getNumberOfRegisteredTrees(), initialNumberOfTrees);

  // A released slot is handed out again, so the numbering stays dense.
  const int reusedId = treeIdMapping.reserveId();
  validateEquals(reusedId, reservedIds.front());
  treeIdMapping.deregisterId(reusedId);
  validateEquals(treeIdMapping.getNumberOfRegisteredTrees(), initialNumberOfTrees);
}

void peano4::parallel::tests::TreeIdMappingTest::run() {
  testMethod(testIdArithmetic);
  testMethod(testReserveAndDeregister);
}
