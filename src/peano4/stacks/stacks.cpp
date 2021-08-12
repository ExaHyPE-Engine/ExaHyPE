#include "stacks.h"


std::string toString( peano4::stacks::IOMode mode ) {
  switch (mode) {
    case peano4::stacks::IOMode::MPIReceive:
      return "receive";
    case peano4::stacks::IOMode::MPISend:
      return "send";
    case peano4::stacks::IOMode::None:
      return "none";
  }
  return "<undef>";
}

