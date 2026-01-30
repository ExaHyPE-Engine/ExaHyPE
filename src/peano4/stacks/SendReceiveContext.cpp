// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "SendReceiveContext.h"

std::string toString(peano4::stacks::SendReceiveContext context) {
  switch (context) {
  case peano4::stacks::SendReceiveContext::BoundaryExchange:
    return "BoundaryExchange";
  case peano4::stacks::SendReceiveContext::MultiscaleExchange:
    return "MultiscaleExchange";
  case peano4::stacks::SendReceiveContext::ForkDomain:
    return "ForkDomain";
  case peano4::stacks::SendReceiveContext::JoinDomain:
    return "JoinDomain";
  case peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
    return "PeriodicBoundaryDataSwap";
  }
  return "<undef>";
}
