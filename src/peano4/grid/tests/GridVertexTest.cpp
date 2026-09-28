// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "GridVertexTest.h"

#include "peano4/grid/GridVertex.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"

#include <mpi.h>

peano4::grid::tests::GridVertexTest::GridVertexTest() :
  TestCase("peano4::grid::tests::GridVertexTest") {}

void
peano4::grid::tests::GridVertexTest::run() {
  testMethod(testState);
  testMethod(testTheByteRepresentationCarriesEveryField);
  testMethod(testSendAndReceiveInARing);
}

void
peano4::grid::tests::GridVertexTest::testState() {
  peano4::grid::GridVertex vertex{};
  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(peano4::grid::GridVertex::State::Unrefined));
  validateEquals(vertex.level, 0);
  validateTrue(vertex.toString().find("Unrefined") != std::string::npos);

  vertex.setState(peano4::grid::GridVertex::State::Refined);
  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(peano4::grid::GridVertex::State::Refined));
  validateTrue(vertex.toString().find("Refined") != std::string::npos);

  vertex.setState(peano4::grid::GridVertex::State::HangingVertex);
  validateEquals(static_cast<int>(vertex.getState()), static_cast<int>(peano4::grid::GridVertex::State::HangingVertex));

  const std::string description = vertex.toString();
  validateTrue(description.find("GridVertex") != std::string::npos);
  validateTrue(description.find("HangingVertex") != std::string::npos);

  std::ostringstream stream;
  stream << vertex;
  validateEquals(stream.str(), description);
}

/**
 * A byte copy of a vertex reproduces every declared field.
 */
void
peano4::grid::tests::GridVertexTest::testTheByteRepresentationCarriesEveryField() {
  static_assert(std::is_trivially_copyable_v<peano4::grid::GridVertex>);
  static_assert(std::is_standard_layout_v<peano4::grid::GridVertex>);

  const peano4::grid::GridVertex defaultVertex{};
  validateEquals(
    static_cast<int>(defaultVertex.getState()),
    static_cast<int>(peano4::grid::GridVertex::State::Unrefined)
  );
  validateEquals(defaultVertex.level, 0);

  peano4::grid::GridVertex vertex{};
  vertex.setState(peano4::grid::GridVertex::State::Refined);
  vertex.level = 5;
#if PEANO_DEBUG
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    vertex.setX(axis, 0.25 * (axis + 1));
  }
#endif

  std::array<std::byte, sizeof(peano4::grid::GridVertex)> bytes{};
  std::memcpy(bytes.data(), &vertex, sizeof(vertex));
  peano4::grid::GridVertex copiedVertex{};
  std::memcpy(&copiedVertex, bytes.data(), sizeof(copiedVertex));

  validateEquals(static_cast<int>(copiedVertex.getState()), static_cast<int>(vertex.getState()));
  validateEquals(copiedVertex.level, vertex.level);
#if PEANO_DEBUG
  for (int axis = 0; axis < DIMENSIONS; axis++) {
    validateNumericalEquals(copiedVertex.getX(axis), vertex.getX(axis));
  }
#endif
}

void
peano4::grid::tests::GridVertexTest::testSendAndReceiveInARing() {
  constexpr int Tag = 4321;

  const auto&    rank          = tarch::mpi::Rank::getInstance();
  const MPI_Comm communicator  = rank.getCommunicator();
  const int      localRank     = rank.getRank();
  const int      numberOfRanks = rank.getNumberOfRanks();

  peano4::grid::GridVertex sentVertex;
  sentVertex.setState(peano4::grid::GridVertex::State::Refined);
  sentVertex.level = 7 + localRank;
#if PEANO_DEBUG
  for (int d = 0; d < DIMENSIONS; d++) {
    sentVertex.setX(d, 0.125 * (d + 1) + 0.001 * localRank);
  }
#endif
  const int successor   = (localRank + 1) % numberOfRanks;
  const int predecessor = (localRank - 1 + numberOfRanks) % numberOfRanks;

  peano4::grid::GridVertex receivedVertex;
  const int                result = tarch::mpi::sendrecv(
    &sentVertex,
    static_cast<int>(sizeof(peano4::grid::GridVertex)),
    MPI_BYTE,
    successor,
    Tag,
    &receivedVertex,
    static_cast<int>(sizeof(peano4::grid::GridVertex)),
    MPI_BYTE,
    predecessor,
    Tag,
    communicator
  );
  validateEquals(result, MPI_SUCCESS);

  validateEquals(
    static_cast<int>(receivedVertex.getState()),
    static_cast<int>(peano4::grid::GridVertex::State::Refined)
  );
  validateEquals(receivedVertex.level, 7 + predecessor);
#if PEANO_DEBUG
  for (int d = 0; d < DIMENSIONS; d++) {
    validateNumericalEquals(receivedVertex.getX(d), 0.125 * (d + 1) + 0.001 * predecessor);
  }
#endif
}
