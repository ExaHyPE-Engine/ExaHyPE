// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "peano4/grid/GridVertex.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/Request.h"
#include "tarch/testing/UnitTest.h"

#include <mpi.h>

UNIT_TEST("A vertex returns its last state and its description contains this state") {
  GIVEN("A default vertex and the states unrefined, refined and hanging") {
    constexpr auto Unrefined     = peano4::grid::GridVertex::State::Unrefined;
    constexpr auto Refined       = peano4::grid::GridVertex::State::Refined;
    constexpr auto HangingVertex = peano4::grid::GridVertex::State::HangingVertex;

    peano4::grid::GridVertex vertex{};

    THEN("The default vertex is unrefined on level zero and its description contains the state") {
      VALIDATE_EQUALS(static_cast<int>(vertex.getState()), static_cast<int>(Unrefined));
      VALIDATE_EQUALS(vertex.level, 0);
      VALIDATE(vertex.toString().find("Unrefined") != std::string::npos);
    }

    WHEN("The test sets the vertex to refined") {
      vertex.setState(Refined);
      const auto        stateAfterTheRefinement       = vertex.getState();
      const std::string descriptionAfterTheRefinement = vertex.toString();

      THEN("The vertex is refined and its description contains the state") {
        VALIDATE_EQUALS(static_cast<int>(stateAfterTheRefinement), static_cast<int>(Refined));
        VALIDATE(descriptionAfterTheRefinement.find("Refined") != std::string::npos);
      }

      AND_WHEN("The test sets the vertex to hanging and writes it into a stream") {
        vertex.setState(HangingVertex);
        const auto         state       = vertex.getState();
        const std::string  description = vertex.toString();
        std::ostringstream stream;
        stream << vertex;

        THEN("The vertex hangs, the description contains its type and the state and the stream receives it") {
          VALIDATE_EQUALS(static_cast<int>(state), static_cast<int>(HangingVertex));
          VALIDATE(description.find("GridVertex") != std::string::npos);
          VALIDATE(description.find("HangingVertex") != std::string::npos);
          VALIDATE_EQUALS(stream.str(), description);
        }
      }
    }
  }
}

UNIT_TEST("A byte copy of a vertex reproduces every declared field") {
  static_assert(std::is_trivially_copyable_v<peano4::grid::GridVertex>);
  static_assert(std::is_standard_layout_v<peano4::grid::GridVertex>);

  GIVEN("A default vertex") {
    const peano4::grid::GridVertex defaultVertex{};

    THEN("The default vertex is unrefined on level zero") {
      VALIDATE_EQUALS(
        static_cast<int>(defaultVertex.getState()),
        static_cast<int>(peano4::grid::GridVertex::State::Unrefined)
      );
      VALIDATE_EQUALS(defaultVertex.level, 0);
    }
  }

  GIVEN("A refined vertex on level five that stores a position in a debug build") {
    peano4::grid::GridVertex vertex{};
    vertex.setState(peano4::grid::GridVertex::State::Refined);
    vertex.level = 5;
#if PEANO_DEBUG
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      vertex.setX(axis, 0.25 * (axis + 1));
    }
#endif

    WHEN("The test copies the vertex into a byte array and from there into another vertex") {
      std::array<std::byte, sizeof(peano4::grid::GridVertex)> bytes{};
      std::memcpy(bytes.data(), &vertex, sizeof(vertex));
      peano4::grid::GridVertex copiedVertex{};
      std::memcpy(&copiedVertex, bytes.data(), sizeof(copiedVertex));

      THEN("The copy reproduces the state, the level and the position of the vertex") {
        VALIDATE_EQUALS(static_cast<int>(copiedVertex.getState()), static_cast<int>(vertex.getState()));
        VALIDATE_EQUALS(copiedVertex.level, vertex.level);
#if PEANO_DEBUG
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          VALIDATE_NUMERICAL_EQUALS(copiedVertex.getX(axis), vertex.getX(axis));
        }
#endif
      }
    }
  }
}

UNIT_TEST("A vertex sent around a ring of ranks as raw bytes arrives unchanged") {
  GIVEN("A refined vertex on every rank derived from the rank and the successor and the predecessor in the ring") {
    constexpr int Tag = 4321;

    const auto&    rank          = tarch::mpi::Rank::getInstance();
    const MPI_Comm communicator  = rank.getCommunicator();
    const int      localRank     = rank.getRank();
    const int      numberOfRanks = rank.getNumberOfRanks();

    peano4::grid::GridVertex sentVertex;
    sentVertex.setState(peano4::grid::GridVertex::State::Refined);
    sentVertex.level = 7 + localRank;
#if PEANO_DEBUG
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      sentVertex.setX(axis, 0.125 * (axis + 1) + 0.001 * localRank);
    }
#endif
    const int successor   = (localRank + 1) % numberOfRanks;
    const int predecessor = (localRank - 1 + numberOfRanks) % numberOfRanks;

    WHEN("Every rank sends its vertex to the successor and receives the vertex of the predecessor") {
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

      THEN("The transfer succeeds with the state, the level and the position of the vertex of the predecessor") {
        VALIDATE_EQUALS(result, MPI_SUCCESS);
        VALIDATE_EQUALS(
          static_cast<int>(receivedVertex.getState()),
          static_cast<int>(peano4::grid::GridVertex::State::Refined)
        );
        VALIDATE_EQUALS(receivedVertex.level, 7 + predecessor);
#if PEANO_DEBUG
        for (int axis = 0; axis < DIMENSIONS; axis++) {
          VALIDATE_NUMERICAL_EQUALS(receivedVertex.getX(axis), 0.125 * (axis + 1) + 0.001 * predecessor);
        }
#endif
      }
    }
  }
}
