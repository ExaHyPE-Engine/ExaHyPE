// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace peano4::grid {
  struct GridStatistics;
} // namespace peano4::grid

struct peano4::grid::GridStatistics {
  int maxDepth = 0;
  int minDepth = 0;

  int numberOfLocalUnrefinedCells  = 0;
  int numberOfRemoteUnrefinedCells = 0;

  int numberOfLocalRefinedCells  = 0;
  int numberOfRemoteRefinedCells = 0;

  int numberOfStationarySweeps = 0;

  std::uint8_t flags = 0;

  static constexpr std::uint8_t CoarseningHasBeenVetoed = 1 << 0;
  static constexpr std::uint8_t RemovedEmptySubtree     = 1 << 1;

  tarch::la::Vector<DIMENSIONS, double> minH{};

  GridStatistics() noexcept = default;
  enum class ObjectConstruction { NoData };
  GridStatistics(ObjectConstruction) noexcept:
    GridStatistics() {}

  inline bool getCoarseningHasBeenVetoed() const noexcept { return (flags & CoarseningHasBeenVetoed) != 0; }
  inline void setCoarseningHasBeenVetoed(bool v) noexcept {
    v ? (flags |= CoarseningHasBeenVetoed) : (flags &= ~CoarseningHasBeenVetoed);
  }

  inline bool getRemovedEmptySubtree() const noexcept { return (flags & RemovedEmptySubtree) != 0; }
  inline void setRemovedEmptySubtree(bool v) noexcept {
    v ? (flags |= RemovedEmptySubtree) : (flags &= ~RemovedEmptySubtree);
  }

  void        clear(bool isGlobalMasterTree);
  std::size_t getMemoryUsageInBytes() const;

  std::string toString() const;

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

  static void send(const GridStatistics& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(GridStatistics& buffer, int source, int tag, MPI_Comm communicator);

  static void send(
    const GridStatistics& buffer,
    int                   destination,
    int                   tag,
    std::function<void()> startCommunicationFunctor,
    std::function<void()> waitFunctor,
    MPI_Comm              communicator
  );
  static void receive(
    GridStatistics&       buffer,
    int                   source,
    int                   tag,
    std::function<void()> startCommunicationFunctor,
    std::function<void()> waitFunctor,
    MPI_Comm              communicator
  );

  static void sendAndPollDanglingMessages(
    const GridStatistics& message,
    int                   destination,
    int                   tag,
    MPI_Comm              communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
  static void receiveAndPollDanglingMessages(
    GridStatistics& message,
    int             source,
    int             tag,
    MPI_Comm        communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
#endif
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::GridStatistics& statistics);

peano4::grid::GridStatistics operator+(
  const peano4::grid::GridStatistics& lhs,
  const peano4::grid::GridStatistics& rhs
);
peano4::grid::GridStatistics operator+=(peano4::grid::GridStatistics& lhs, const peano4::grid::GridStatistics& rhs);
