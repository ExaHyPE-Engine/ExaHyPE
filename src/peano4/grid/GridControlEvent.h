// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

namespace peano4::grid {
  struct GridControlEvent;
} // namespace peano4::grid

struct peano4::grid::GridControlEvent {
public:
  enum class RefinementControl : std::uint8_t { Refine = 0, Erase = 1 };

  std::int8_t refinementControl = static_cast<std::uint8_t>(RefinementControl::Refine);

  tarch::la::Vector<DIMENSIONS, double> offset{};
  tarch::la::Vector<DIMENSIONS, double> width{};
  tarch::la::Vector<DIMENSIONS, double> h{};

  GridControlEvent() noexcept = default;
  enum class ObjectConstruction { NoData };
  GridControlEvent(ObjectConstruction) noexcept:
    GridControlEvent() {}

  INLINE_METHOD RefinementControl getRefinementControl() const noexcept {
    return static_cast<RefinementControl>(refinementControl);
  }
  INLINE_METHOD void setRefinementControl(RefinementControl rc) noexcept { refinementControl = static_cast<std::uint8_t>(rc); }

  std::string toString() const;

  bool operator<(const GridControlEvent& other) const noexcept;

  static void merge(std::vector<GridControlEvent>& events, double tolerance = 0.0);

#ifdef WITH_MPI
  static MPI_Datatype Datatype;
  int                 senderDestinationRank = -1;

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

  static void send(const GridControlEvent& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(GridControlEvent& buffer, int source, int tag, MPI_Comm communicator);

  static void send(
    const GridControlEvent& buffer,
    int                     destination,
    int                     tag,
    std::function<void()>   startCommunicationFunctor,
    std::function<void()>   waitFunctor,
    MPI_Comm                communicator
  );
  static void receive(
    GridControlEvent&     buffer,
    int                   source,
    int                   tag,
    std::function<void()> startCommunicationFunctor,
    std::function<void()> waitFunctor,
    MPI_Comm              communicator
  );

  static void sendAndPollDanglingMessages(
    const GridControlEvent& message,
    int                     destination,
    int                     tag,
    MPI_Comm                communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
  static void receiveAndPollDanglingMessages(
    GridControlEvent& message,
    int               source,
    int               tag,
    MPI_Comm          communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
#endif
};

std::ostream& operator<<(std::ostream& os, const peano4::grid::GridControlEvent& event);
