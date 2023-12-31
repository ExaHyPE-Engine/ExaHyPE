// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryLocation.h"

std::string toString(tarch::memory::MemoryLocation value) {
  switch (value) {
  case tarch::memory::MemoryLocation::Heap:
    return "heap";
  case tarch::memory::MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    return "managed-shared-accelerator-device";
  case tarch::memory::MemoryLocation::PinnedMemory:
    return "pinned";
  }
  return "<undef>";
}
