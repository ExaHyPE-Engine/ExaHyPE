// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "DeviceCopies.h"

#include "tarch/Assertions.h"

namespace {
  /**
   * Returns the uploads of every translation unit that defines a device copy.
   */
  std::vector<std::function<void()>>& getUploads() {
    static std::vector<std::function<void()>> uploads;
    return uploads;
  }
} // namespace

/**
 * Adds upload to the uploads that uploadDeviceCopies() calls and returns true.
 */
bool
tarch::accelerator::addDeviceCopyUpload(const std::function<void()> upload) {
  ASSERTION(static_cast<bool>(upload));
  getUploads().push_back(upload);
  return true;
}

/**
 * Calls every added upload. A caller calls this function after every change of a host value.
 */
void
tarch::accelerator::uploadDeviceCopies() {
  for (const auto& upload : getUploads()) {
    upload();
  }
}
