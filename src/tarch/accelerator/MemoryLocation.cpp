// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryLocation.h"

std::string tarch::toString(MemoryLocation value) {
  switch (value) {
  case MemoryLocation::Heap:
    return "Heap";
  case MemoryLocation::Device:
    return "Device";
  case MemoryLocation::Pinned:
    return "Pinned";
  }
  return "<undef>";
}
