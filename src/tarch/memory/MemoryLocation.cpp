// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryLocation.h"

std::string toString(MemoryLocation value) {
  switch (value) {
  case MemoryLocation::Heap:
    return "heap";
  case MemoryLocation::ManagedSharedAcceleratorDeviceMemory:
    return "managed-shared-accelerator-device";
  case MemoryLocation::PinnedMemory:
    return "pinned";
    return "<undef>";
  }
}
