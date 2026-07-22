// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace peano4::grid {
  struct GridVertex;
} // namespace peano4::grid

struct peano4::grid::GridVertex {
  enum class State : std::uint8_t {
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
  std::uint8_t state = static_cast<std::uint8_t>(State::New);

  int level                              = 0;
  int numberOfAdjacentRefinedLocalCells  = 0;
  int numberOfAdjacentRefinedRemoteCells = 0;
#if PEANO_DEBUG
  tarch::la::Vector<DIMENSIONS, double> x{};
#endif

  std::array<int, TWO_POWER_D> adjacentRanks{};
  std::array<int, TWO_POWER_D> backupOfAdjacentRanks{};

  static constexpr std::uint8_t HasBeenAntecessorOfRefinedVertexInPreviousTreeSweep = 1 << 0;
  static constexpr std::uint8_t IsAntecessorOfRefinedVertexInCurrentTreeSweep       = 1 << 1;
  static constexpr std::uint8_t HasBeenParentOfSubtreeVertexInPreviousTreeSweep     = 1 << 2;
  static constexpr std::uint8_t IsParentOfSubtreeVertexInCurrentTreeSweep           = 1 << 3;
  static constexpr std::uint8_t HasBeenVertexTypeHangingInPreviousTreeSweep         = 1 << 4;
  static constexpr std::uint8_t HasBeenSurroundedByRefinedCellsInPreviousTreeSweep  = 1 << 5;

  GridVertex() noexcept = default;

  INLINE_METHOD State getState() const noexcept { return static_cast<State>(state); }
  INLINE_METHOD void  setState(State value) noexcept { state = static_cast<std::uint8_t>(value); }

#if PEANO_DEBUG
  INLINE_METHOD const tarch::la::Vector<DIMENSIONS, double>& getX() const noexcept { return x; }
  INLINE_METHOD void   setX(const tarch::la::Vector<DIMENSIONS, double>& value) noexcept { x = value; }
  INLINE_METHOD double getX(int d) const noexcept { return x[d]; }
  INLINE_METHOD void   setX(int d, double value) noexcept { x[d] = value; }

  static tarch::la::Vector<DIMENSIONS, double> getVertexCoordinate(
    const tarch::la::Vector<DIMENSIONS, double>& cellX,
    const tarch::la::Vector<DIMENSIONS, double>& cellH,
    int                                          vertexIndex
  );
#endif

  INLINE_METHOD bool getFlag(std::uint8_t bit) const noexcept { return (flags & bit) != 0; }
  INLINE_METHOD void setFlag(std::uint8_t bit) noexcept { flags |= bit; }
  INLINE_METHOD void setFlag(std::uint8_t bit, bool value) noexcept { value ? setFlag(bit) : clearFlag(bit); }
  INLINE_METHOD void clearFlag(std::uint8_t bit) noexcept { flags &= ~bit; }

  std::string toString() const;

  static GridVertex create(
    int                                          level,
    GridVertex::State                            state,
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const std::array<int, TWO_POWER_D>&          adjacentRanks,
    bool                                         isNewFineGridVertex
  );

  static GridVertex create(
    int                                          level,
    GridVertex::State                            state,
    const tarch::la::Vector<DIMENSIONS, double>& cellX,
    const tarch::la::Vector<DIMENSIONS, double>& cellH,
    int                                          vertexIndex,
    const std::array<int, TWO_POWER_D>&          adjacentRanks,
    bool                                         isNewFineGridVertex
  );

  static bool areAllVerticesRefined(GridVertex vertices[TWO_POWER_D]);
  static bool areAllVerticesUnrefined(GridVertex vertices[TWO_POWER_D]);

  /**
   * Will the vertex be refined after this tree sweep.
   */
  static bool willVertexBeRefined(const GridVertex& vertex);

  /**
   * Has the vertex been refined before this tree sweep.
   */
  static bool hasVertexBeenRefined(const GridVertex& vertex);

  /**
   * @return One bit per vertex, set where willVertexBeRefined() holds. Compare
   *   the result to zero to learn whether a cell is refined, or use
   *   peano4::grid::SpacetreeHelper::isSpacetreeNodeRefined().
   */
  static std::bitset<TWO_POWER_D> willVerticesBeRefined(GridVertex vertices[TWO_POWER_D]);

  /**
   * @return One bit per vertex, set where hasVertexBeenRefined() holds.
   */
  static std::bitset<TWO_POWER_D> haveVerticesBeenRefined(GridVertex vertices[TWO_POWER_D]);

#ifdef PEANO_WITH_MPI
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
