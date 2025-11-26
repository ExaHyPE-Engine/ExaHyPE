// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Globals.h"
#include "tarch/mpi/Rank.h"

#include <bitset>

namespace peano4::grid {
  struct GridVertex;
} // namespace peano4::grid

struct peano4::grid::GridVertex {
  enum class State : std::int8_t { // std::uint8_t
    HangingVertex       = 0,
    New                 = 1,
    Unrefined           = 2,
    Refined             = 3,
    RefinementTriggered = 4,
    Refining            = 5,
    EraseTriggered      = 6,
    Erasing             = 7,
    Delete              = 8
  };

  std::uint8_t flags = 0;
  std::int8_t  state = static_cast<std::int8_t>(State::New);

  int level                             = 0;
  int numberOfAdjacentRefinedLocalCells = 0;

  std::array<int, TWO_POWER_D> adjacentRanks{};
  std::array<int, TWO_POWER_D> backupOfAdjacentRanks{};

  // Bit positions (right to left = least significant bit first):
  // [bit3][bit2][bit1][bit0]
  //   8     4     2     1   <-- decimal value of each bit
  static constexpr std::uint8_t HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep = 1 << 0; // = 0b00000001
  static constexpr std::uint8_t IsAntecessorOfRefinedVertexInCurrentTreeSweep       = 1 << 1; // = 0b00000010
  static constexpr std::uint8_t HasBeenParentOfSubtreeVertexInPreviousTreeSweep     = 1 << 2; // = 0b00000100
  static constexpr std::uint8_t IsParentOfSubtreeVertexInCurrentTreeSweep           = 1 << 3; // = 0b00001000

  GridVertex() noexcept = default;
  enum class ObjectConstruction { NoData };
  GridVertex(ObjectConstruction) noexcept:
    GridVertex() {}

  INLINE_METHOD State getState() const noexcept { return static_cast<State>(state); }
  INLINE_METHOD void  setState(State s) noexcept { state = static_cast<std::int8_t>(s); }

  // Checks if a given bit is set (true).
  // flags & bit → masks out all bits except the one we care about.
  // If result is non-zero → that bit is set → return true.
  INLINE_METHOD bool getFlag(std::uint8_t bit) const noexcept { return (flags & bit) != 0; }

  // Turns on a given bit (sets it to 1).
  // Bitwise OR (|=) sets the given bit to 1 while leaving all others unchanged.
  INLINE_METHOD void setFlag(std::uint8_t bit) noexcept { flags |= bit; }
  INLINE_METHOD void setFlag(std::uint8_t bit, bool v) noexcept { v ? setFlag(bit) : clearFlag(bit); }

  // Turns off a given bit (sets it to 0).
  // ~bit flips all bits (1→0, 0→1), making a mask with all bits = 1 except the one we want to clear.
  // Bitwise AND (&=) with that mask sets the target bit to 0 without touching the others.
  INLINE_METHOD void clearFlag(std::uint8_t bit) noexcept { flags &= ~bit; }

  std::string toString() const;

  /**
   * Factory mechanism
   *
   * We expect the calling code to tell us about the adjacent ranks of a
   * vertex. There is a routine Spacetree::getAdjacentRanksForNewVertex()
   * which allows you to distill adjacency information while you step down
   * within the tree and create new vertices. This is information we write
   * directly into the new data plus the backup of the old data. This means,
   * by the time we create a new vertex, anybody analysing the adjacency
   * information thinks that this data has always been there.
   *
   * @image html GridVertex.png
   *
   * @param adjacentRanks Typically the result from
   * Spacetree::getAdjacentRanksForNewVertex(coarseGridVertices, vertexPositionWithin3x3Patch).
   */
  static GridVertex create(
    int                                 level,
    GridVertex::State                   state,
    const std::array<int, TWO_POWER_D>& adjacentRanks,
    bool                                isNewFineGridVertex
  );

  static bool areAllVerticesRefined(GridVertex vertices[TWO_POWER_D]);

  static bool areAllVerticesUnrefined(GridVertex vertices[TWO_POWER_D]);

  /**
   * A vertex will be refined if it is already refined or currently refining.
   * It also will be refined if the erase is only triggered.
   */
  static bool willVertexBeRefined(const GridVertex& vertex);

  /**
   * A vertex has been refined if it is (already) refined or is erasing or
   * the erase has been triggered.
   */
  static bool hasVertexBeenRefined(const GridVertex& vertex);

  /**
   * A vertex is unrefined if it is hanging.
   *
   * @return bitset of vertices for which isVertexRefined() holds. If you wanna
   *   find out whether a cell is refined, you can compare the result to zero.
   *   You can also use isSpacetreeNodeRefined() instead.
   */
  static std::bitset<TWO_POWER_D> willVerticesBeRefined(GridVertex vertices[TWO_POWER_D]);
  static std::bitset<TWO_POWER_D> haveVerticesBeenRefined(GridVertex vertices[TWO_POWER_D]);

#ifdef WITH_MPI
  static MPI_Datatype Datatype;

  static MPI_Datatype getForkDatatype();
  static MPI_Datatype getJoinDatatype();
  static MPI_Datatype getBoundaryExchangeDatatype();
  static MPI_Datatype getMultiscaleDataExchangeDatatype();
  static MPI_Datatype getGlobalCommunicationDatatype();

  static void freeForkDatatype();
  static void freeJoinDatatype();
  static void freeBoundaryExchangeDatatype();
  static void freeMultiscaleDataExchangeDatatype();
  static void freeGlobalCommunicationDatatype();

  static void initDatatype();
  static void shutdownDatatype();

  static void send(const GridVertex& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(GridVertex& buffer, int source, int tag, MPI_Comm communicator);

  static void send(
    const GridVertex&     buffer,
    int                   destination,
    int                   tag,
    std::function<void()> startCommunicationFunctor,
    std::function<void()> waitFunctor,
    MPI_Comm              communicator
  );
  static void receive(
    GridVertex&           buffer,
    int                   source,
    int                   tag,
    std::function<void()> startCommunicationFunctor,
    std::function<void()> waitFunctor,
    MPI_Comm              communicator
  );

  static void sendAndPollDanglingMessages(
    const GridVertex& message,
    int               destination,
    int               tag,
    MPI_Comm          communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
  static void receiveAndPollDanglingMessages(
    GridVertex& message,
    int         source,
    int         tag,
    MPI_Comm    communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
#endif
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridVertex& vertex);
