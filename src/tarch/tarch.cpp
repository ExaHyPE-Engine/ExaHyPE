// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch.h"

#include "logging/Log.h"

#include <limits.h>
#include <iostream>

#if defined(COMPILER_HAS_PROC_STAT)
#include <unistd.h>
#endif

#include <cstdlib>
#include <cstring>
#if defined(COMPILER_HAS_SYSINFO) // for Linux OS
#include <sys/sysinfo.h>
#else // for Mac OS support
#include <sys/sysctl.h>
#include <sys/resource.h>
#endif

static_assert(sizeof(void*) == 8, "Assuming 64-bit build; i.e., 8-byte pointers");
static_assert(std::numeric_limits<float>::is_iec559, "std::numeric_limits<float>::is_iec559");
static_assert(std::numeric_limits<double>::is_iec559, "std::numeric_limits<double>::is_iec55");
static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
static_assert(sizeof(char) == 1, "sizeof(short) != 1");
static_assert(sizeof(short) == 2, "sizeof(short) != 2");
static_assert(sizeof(int) == 4, "sizeof(int) != 4");
static_assert(sizeof(long long) == 8, "sizeof(long long) != 8");
static_assert('A' == 65, "A != 65");

namespace {
  tarch::logging::Log _log("tarch");
} // namespace

int tarch::convertMemorySize(long long value, MemoryUsageFormat format) {
  constexpr long long MegaByte = 1024 * 1024;
  constexpr long long GigaByte = MegaByte * 1024;
  int           result   = 0;
  switch (format) {
    case tarch::MemoryUsageFormat::MByte:
    {
      long long usageMB ((value + (MegaByte / 2)) / MegaByte);
      if (usageMB > std::numeric_limits<int>::max()) {
        logError("convertMemorySize()", "Cannot cast result to return value: " << usageMB);
      }
      result = static_cast<int>(usageMB);
      break;
    }
    case tarch::MemoryUsageFormat::GByte:
    {
      long long usageGB ((value + (GigaByte / 2)) / GigaByte);
      if (usageGB > std::numeric_limits<int>::max()) {
        logError("convertMemorySize()", "Cannot cast result to return value: " << usageGB);
      }
      result = static_cast<int>(usageGB);
      break;
    }
  }
  return result;
}

#if defined(COMPILER_HAS_SYSINFO)

int tarch::getTotalMemory(MemoryUsageFormat format) {
  struct sysinfo info;
  sysinfo(&info);

  return convertMemorySize(info.totalram, format);
}

int tarch::getFreeMemory(MemoryUsageFormat format) {
  struct sysinfo info;
  sysinfo(&info);

  return convertMemorySize(info.freeram, format);
}

#else // defined(COMPILER_HAS_SYSINFO)

int tarch::getTotalMemory(MemoryUsageFormat format) {
  int mib[] = { CTL_HW, HW_MEMSIZE };
  int64_t physical_memory;
  size_t length;

  // Get the physical memory size
  length = sizeof(int64_t);
  sysctl(mib, 2, &physical_memory, &length, NULL, 0);
  return convertMemorySize(physical_memory, format);
}

int tarch::getFreeMemory(MemoryUsageFormat format) {
  struct rusage usage{};
  if (0 == getrusage(RUSAGE_SELF, &usage)) {
    auto freeMem = tarch::getTotalMemory(tarch::MemoryUsageFormat::MByte) - usage.ru_maxrss;
    return convertMemorySize(freeMem, format);
  } else {
    return 0;
  }
}

#endif // defined(COMPILER_HAS_SYSINFO)

int tarch::getMemoryUsage(MemoryUsageFormat format) {
  static tarch::logging::Log _log("tarch");

#if defined(COMPILER_HAS_PROC_STAT)
  char   work[4096];
  FILE*  f    = NULL;
  char*  pCh  = NULL;

  pid_t pid = getpid();

  sprintf(work, "/proc/%d/stat", (int)pid);
  f = fopen(work, "r");

  if (f == NULL) {
    logError("getMemoryUsage()", "Can't open file " << work);
    return(0);
  }
  if (fgets(work, sizeof(work), f) == NULL) {
    logError("getMemoryUsage","Error while reading from file");
  }
  fclose(f);
  strtok(work, " ");

  for (int i = 1; i < 23; i++) {
    pCh = strtok(NULL, " ");
  }

  std::size_t rawInput = atol(pCh);
#else
  std::size_t rawInput = 0;
#endif

  return convertMemorySize(rawInput, format);
}
