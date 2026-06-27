// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "StackSizeLimitDetection.h"

#include "tarch/instrumentation/Log.h"
#include "tarch/MemoryUsage.h"

#if !defined(_WIN32)
#include <sys/resource.h>
#endif

#include <sstream>

namespace {
  std::string formatMemorySize(unsigned long long bytes) {
    const auto         format = tarch::chooseMemoryUsageFormat(bytes);
    std::ostringstream result;
    result << tarch::convertMemorySize(bytes, format) << " " << tarch::toString(format);
    return result.str();
  }

  const tarch::StackSizeLimit& getCachedStackSizeLimit() {
    static const tarch::StackSizeLimit stackSizeLimit = tarch::StackSizeLimit::detect();
    return stackSizeLimit;
  }
} // namespace

tarch::StackSizeLimit tarch::StackSizeLimit::detect() {
  StackSizeLimit result;

#if defined(_WIN32)
  return result;
#else
  struct rlimit stackLimit{};
  if (getrlimit(RLIMIT_STACK, &stackLimit) != 0) {
    return result;
  }

  result.detected  = true;
  result.unlimited = stackLimit.rlim_cur == RLIM_INFINITY;
  if (not result.unlimited) {
    result.bytes = static_cast<unsigned long long>(stackLimit.rlim_cur);
  }
  return result;
#endif
}

std::string tarch::detectStackSizeLimit() {
  const auto& stackSizeLimit = getCachedStackSizeLimit();
#if defined(_WIN32)
  logWarning("Stack size limit detection is not implemented on Windows");
  return "unknown";
#else
  constexpr unsigned long long RecommendedMinimumStackLimitInKiB = 0x200'000ULL;
  constexpr unsigned long long RecommendedMinimumStackLimit      = RecommendedMinimumStackLimitInKiB * KiB;

  if (not stackSizeLimit.detected) {
    logError("Stack size cannot be determined because getrlimit() failed");
    return "unknown";
  }

  if (stackSizeLimit.unlimited) {
    return "unlimited";
  }

  const auto currentLimitInKiB = stackSizeLimit.bytes / KiB;
  if (stackSizeLimit.bytes < RecommendedMinimumStackLimit) {
    logDebug(
      "Stack size of " << currentLimitInKiB << " KiB (=" << formatMemorySize(stackSizeLimit.bytes)
                       << ") is lower than recommended minimum of " << RecommendedMinimumStackLimitInKiB
                       << " KiB (=" << formatMemorySize(RecommendedMinimumStackLimit)
                       << "). You can increase the stack size by running: ulimit -Ss unlimited"
    );
  }
  return std::to_string(currentLimitInKiB) + " KiB (=" + formatMemorySize(stackSizeLimit.bytes) + ")";
#endif
}
