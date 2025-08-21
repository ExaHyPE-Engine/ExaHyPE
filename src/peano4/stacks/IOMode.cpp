// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "IOMode.h"

std::string toString(peano4::stacks::IOMode mode) {
  switch (mode) {
  case peano4::stacks::IOMode::MPIReceive:
    return "MPIReceive";
  case peano4::stacks::IOMode::MPISend:
    return "MPISend";
  case peano4::stacks::IOMode::None:
    return "None";
  }
  return "<undef>";
}
