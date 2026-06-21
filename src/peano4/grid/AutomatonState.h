// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Globals.h"
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"

#include <type_traits>

namespace peano4::grid {
  struct AutomatonState;
} // namespace peano4::grid

struct peano4::grid::AutomatonState {
  int level = 0;

  tarch::la::Vector<DIMENSIONS, double> x{};
  tarch::la::Vector<DIMENSIONS, double> h{};

  // Does not have to be tarch::la::Vector as we don't do linear algebra on this
  std::array<std::int8_t, TWO_TIMES_D> accessNumber{};

  using flag_t = std::conditional_t<(DIMENSIONS <= 31), std::uint32_t, std::uint64_t>;
  flag_t flags = 0; // [0..DIMENSIONS-1] = even flags, [DIMENSIONS] = inverted

  AutomatonState() noexcept = default;

  static constexpr flag_t evenMaskAll() { return (flag_t(1) << DIMENSIONS) - flag_t(1); }
  static constexpr flag_t evenMask(int i) { return flag_t(1) << i; }
  static constexpr flag_t invertedMask() { return flag_t(1) << DIMENSIONS; }

  INLINE_METHOD bool getInverted() const noexcept { return (flags & invertedMask()) != 0; }
  INLINE_METHOD void setInverted(bool v) noexcept { setFlag(invertedMask(), v); }

  INLINE_METHOD bool getEvenFlag(int d) const noexcept { return (flags & evenMask(d)) != 0; }
  INLINE_METHOD void setEvenFlag(int d, bool v) noexcept { setFlag(evenMask(d), v); }
  INLINE_METHOD void flipEvenFlag(int d) noexcept { flags ^= evenMask(d); }

  INLINE_METHOD bool getFlag(flag_t bit) const noexcept { return (flags & bit) != 0; }
  INLINE_METHOD void setFlag(flag_t bit) noexcept { flags |= bit; }
  INLINE_METHOD void clearFlag(flag_t bit) noexcept { flags &= ~bit; }
  INLINE_METHOD void setFlag(flag_t bit, bool v) noexcept { v ? setFlag(bit) : clearFlag(bit); }

  std::string toString() const;

  /**
   * Takes a state (describing a node in the tree) and returns the
   * @f$ 3^d @f$ states on the next finer level along the Peano SFC. This
   * routine is basically the grammar generation of Bader et al. It relies
   * internally on a recursive call of the other refineState() routine
   * along the spatial dimensions.
   *
   * The array fineGrid is filled along the Peano SFC. So you don't have to
   * traverse the array with a zfor - a simple for is sufficient.
   */
  static void refineState(
    const AutomatonState&              coarseGrid,
    AutomatonState                     fineGrid[THREE_POWER_D],
    tarch::la::Vector<DIMENSIONS, int> fineGridPosition = tarch::la::Vector<DIMENSIONS, int>(0),
    int                                axis             = DIMENSIONS - 1
  );

#ifdef PEANO_WITH_MPI
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

  static void send(const AutomatonState& buffer, int destination, int tag, MPI_Comm communicator);
  static void receive(AutomatonState& buffer, int source, int tag, MPI_Comm communicator);

  static void send(
    const AutomatonState& buffer,
    int                   destination,
    int                   tag,
    std::function<void()> startCommunicationFunctor,
    std::function<void()> waitFunctor,
    MPI_Comm              communicator
  );
  static void receive(
    AutomatonState&       buffer,
    int                   source,
    int                   tag,
    std::function<void()> startCommunicationFunctor,
    std::function<void()> waitFunctor,
    MPI_Comm              communicator
  );

  static void sendAndPollDanglingMessages(
    const AutomatonState& message,
    int                   destination,
    int                   tag,
    MPI_Comm              communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
  static void receiveAndPollDanglingMessages(
    AutomatonState& message,
    int             source,
    int             tag,
    MPI_Comm        communicator = tarch::mpi::Rank::getInstance().getCommunicator()
  );
#endif
};

std::ostream& operator<<(std::ostream& out, const peano4::grid::AutomatonState& state);
