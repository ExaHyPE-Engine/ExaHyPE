// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryUsage.h"

#include "tarch/instrumentation/Log.h"

#include <cstdlib>
#include <cstring>
#include <iostream>
#if defined(COMPILER_HAS_PROC_STAT)
#include <unistd.h>
#endif
#if defined(COMPILER_HAS_SYSINFO) // for Linux OS
#include <sys/sysinfo.h>
#else // for Mac OS support
#include <sys/resource.h>
#include <sys/sysctl.h>
#endif

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
  // Total physical memory in bytes, as in getTotalMemory() above.
  int     mib[] = {CTL_HW, HW_MEMSIZE};
  int64_t physical_memory = 0;
  size_t  length          = sizeof(int64_t);
  sysctl(mib, 2, &physical_memory, &length, NULL, 0);

  struct rusage usage{};
  if (0 == getrusage(RUSAGE_SELF, &usage)) {
    // convertMemorySize() expects bytes; ru_maxrss is reported in bytes on
    // macOS (unlike Linux, where it is in kilobytes).
    return convertMemorySize(physical_memory - usage.ru_maxrss, format);
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
    logError("Can't open file " << work);
    return (0);
  }
  if (fgets(work, sizeof(work), f) == NULL) {
    logError("Error while reading from file");
  }
  fclose(f);
  strtok(work, " ");

  // Field 24 of /proc/<pid>/stat is the resident set size in pages (field 23 is the virtual
  // size). We want the actually consumed (resident) memory, so read field 24 and scale by the
  // page size to obtain bytes.
  for (int i = 1; i < 24; i++) {
    pCh = strtok(NULL, " ");
  }

  std::size_t residentPages = atol(pCh);
  std::size_t rawInput      = residentPages * static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
#else
  std::size_t rawInput = 0;
#endif

  return convertMemorySize(rawInput, format);
}
