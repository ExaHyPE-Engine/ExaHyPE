// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch.h"

#include "logging/Log.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits.h>
#if defined(COMPILER_HAS_PROC_STAT)
#include <unistd.h>
#endif
#if defined(COMPILER_HAS_SYSINFO) // for Linux OS
#include <sys/sysinfo.h>
#else // for Mac OS support
#include <sys/resource.h>
#include <sys/sysctl.h>
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

std::string tarch::toString(MemoryUsageFormat format) {
  switch (format) {
  case MemoryUsageFormat::KiB:
    return "KiB";
  case MemoryUsageFormat::MiB:
    return "MiB";
  case MemoryUsageFormat::GiB:
    return "GiB";
  }
  return "<undef>";
}

double tarch::convertMemorySize(unsigned long long bytes, MemoryUsageFormat format) {
  switch (format) {
  case MemoryUsageFormat::KiB:
    return static_cast<double>(bytes) / KiB;
  case MemoryUsageFormat::MiB:
    return static_cast<double>(bytes) / MiB;
  case MemoryUsageFormat::GiB:
    return static_cast<double>(bytes) / GiB;
  }
  return 0.0;
}

#if defined(COMPILER_HAS_SYSINFO)

double tarch::getTotalMemory(MemoryUsageFormat format) {
  struct sysinfo info;
  sysinfo(&info);
  return convertMemorySize(info.totalram, format);
}

double tarch::getFreeMemory(MemoryUsageFormat format) {
  struct sysinfo info;
  sysinfo(&info);
  return convertMemorySize(info.freeram, format);
}

#else // defined(COMPILER_HAS_SYSINFO)

double tarch::getTotalMemory(MemoryUsageFormat format) {
  int     mib[] = {CTL_HW, HW_MEMSIZE};
  int64_t physical_memory;
  size_t  length;

  // Get the physical memory size
  length = sizeof(int64_t);
  sysctl(mib, 2, &physical_memory, &length, NULL, 0);
  return convertMemorySize(physical_memory, format);
}

double tarch::getFreeMemory(MemoryUsageFormat format) {
  struct rusage usage{};
  if (0 == getrusage(RUSAGE_SELF, &usage)) {
    auto freeMem = tarch::getTotalMemory(tarch::MemoryUsageFormat::MByte) - usage.ru_maxrss;
    return convertMemorySize(freeMem, format);
  } else {
    return 0;
  }
}

#endif // defined(COMPILER_HAS_SYSINFO)

double tarch::getMemoryUsage(MemoryUsageFormat format) {
#if defined(COMPILER_HAS_PROC_STAT)
  char  work[4096];
  FILE* f   = NULL;
  char* pCh = NULL;

  pid_t pid = getpid();

  sprintf(work, "/proc/%d/stat", (int)pid);
  f = fopen(work, "r");

  if (f == NULL) {
    logError("getMemoryUsage()", "Can't open file " << work);
    return (0);
  }
  if (fgets(work, sizeof(work), f) == NULL) {
    logError("getMemoryUsage", "Error while reading from file");
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
