// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"

#include <string>

#ifdef WITH_MPI
#include <mpi.h>
#endif

namespace peano4::stacks {
  /**
   * There are three different scenarios when we merge data:
   *
   * - We exchange data as we are at a proper parallel boundary. As we work
   *   with a non-overlapping domain decomposition and handle the individual
   *   levels separately, this can only happen for vertices and faces. It
   *   never happens for cells.
   * - We exchange data as we are at a periodic boundary. Again, this happens
   *   only for vertices and faces.
   * - We exchange data between different levels of the tree, where the
   *   parent cell is hosted on another tree than the child. In this case,
   *   we also exchange cell data.
   *
   * There are two more modes: join and fork. It is important to note that
   * these differ from the aforementioned modi, as forks and joins are
   * realised as copies. We do not merge existing data structures, but we
   * copy them. As such, the context fork and the context join do arise when
   * we discuss data exchange. They do never arise when we speak about data
   * merges. If you have a case distinction within a merge, those two modi
   * should be left out (or equipped with assertion). They should be never
   * entered.
   */
  enum class SendReceiveContext {
    BoundaryExchange,
    MultiscaleExchange,
    ForkDomain,
    JoinDomain,
    PeriodicBoundaryDataSwap
  };

#if defined(WITH_MPI)
  template <class T>
  static inline MPI_Datatype translateContextIntoDatatype(SendReceiveContext context) {
    switch (context) {
    case SendReceiveContext::BoundaryExchange:
      return T::getBoundaryExchangeDatatype();
    case SendReceiveContext::MultiscaleExchange:
      return T::getMultiscaleDataExchangeDatatype();
    case SendReceiveContext::ForkDomain:
      return T::getForkDatatype();
    case SendReceiveContext::JoinDomain:
      return T::getJoinDatatype();
    case SendReceiveContext::PeriodicBoundaryDataSwap:
      assertionMsg(false, "Periodic boundary data is never exchanged via MPI");
      return T::getGlobalCommunicationDatatype();
    }
    assertionMsg(false, "No valid case statement found. This may not happen");
    return T::getGlobalCommunicationDatatype();
  }
#endif
} // namespace peano4::stacks

std::string toString(peano4::stacks::SendReceiveContext context);
