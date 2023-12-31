// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "memory.h"

#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/logging/Log.h"
#include "tarch/memory/MemoryManager.h"

#if defined(CompilerHasProcStat)
#include <unistd.h>
#endif

#include <cstdlib>
#include <cstring>
#if defined(CompilerHasSysinfo) // for Linux OS
#include <sys/sysinfo.h>
#else // for Mac OS support
#include <sys/resource.h>
#include <sys/sysctl.h>
#endif

int tarch::convertMemorySize(long long value, tarch::memory::MemoryUsageFormat format) {
  static tarch::logging::Log _log("tarch::memory");
  static constexpr int       MegaByte = 1024 * 1024;
  static constexpr int       GigaByte = MegaByte * 1024;
  int                        result   = 0;
  switch (format) {
  case tarch::memory::MemoryUsageFormat::MByte:
    result = static_cast<int>((value + (MegaByte / 2)) / MegaByte);
    if (result > std::numeric_limits<int>::max()) {
      logError("convertMemorySize()", "Cannot cast result to return value: " << result);
    }
    break;
  case tarch::memory::MemoryUsageFormat::GByte:
    result = static_cast<int>((value + (GigaByte / 2)) / GigaByte);
    if (result > std::numeric_limits<int>::max()) {
      logError("convertMemorySize()", "Cannot cast result to return value: " << result);
    }
    break;
  }
  return result;
}

#if defined(CompilerHasSysinfo)

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

#else

int tarch::getTotalMemory(MemoryUsageFormat format) {
  int         mib[]           = {CTL_HW, HW_MEMSIZE};
  int64_t     physical_memory = 0;
  std::size_t length          = 0;

  // Get the physical memory size
  length = sizeof(int64_t);
  sysctl(mib, 2, &physical_memory, &length, NULL, 0);
  return convertMemorySize(physical_memory, format);
}

int tarch::getFreeMemory(MemoryUsageFormat format) {
  struct rusage usage {};
  if (0 == getrusage(RUSAGE_SELF, &usage)) {
    long freeMem = tarch::memory::getTotalMemory(format) - usage.ru_maxrss;
    return convertMemorySize(freeMem, format);
  } else
    return 0;
}

#endif

int tarch::getMemoryUsage(MemoryUsageFormat format) {
  static tarch::logging::Log _log("tarch::memory");

#if defined(CompilerHasProcStat)
  char  work[4096]{};
  FILE* f;
  char* pCh = NULL;

  pid_t pid = getpid();

  sprintf(work, "/proc/%d/stat", (int)pid);
  f = fopen(work, "r");

  if (f == NULL) {
    logError("getMemoryUsage()", "Cannot open file " << work);
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

void tarch::memory::freeMemory(void* pointer, MemoryLocation location, int device) {
  // MemoryManager::getInstance().free(pointer, device);
  accelerator::freeRawData(pointer, location, device);
}
