#include "tarch.h"

#include "config.h"
#include "tarch/logging/Log.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#include <limits.h>
#include <iostream>

#if defined(CompilerHasProcStat)
#include <unistd.h>
#endif

#include <cstdlib>
#include <cstring>
#if defined(CompilerHasSysinfo) // for Linux OS
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

void tarch::dumpStatistics() {
  std::stringstream msg;

  int numberOfRanks        = tarch::mpi::Rank::getInstance().getNumberOfRanks();
  int localNumberOfThreads = tarch::multicore::Core::getInstance().getNumberOfThreads();
  int localNumberOfDevices = tarch::accelerator::Device::getInstance().getNumberOfDevices();

  int globalNumberOfThreads = 0;
  int globalNumberOfDevices = 0;

#ifdef Parallel
  tarch::mpi::Rank::getInstance().reduce(&localNumberOfThreads, &globalNumberOfThreads, 1, MPI_INT, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), [&]() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  });

  tarch::mpi::Rank::getInstance().reduce(&localNumberOfDevices, &globalNumberOfDevices, 1, MPI_INT, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), [&]() -> void {
    tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
  });
#else
  globalNumberOfThreads = localNumberOfThreads;
  globalNumberOfDevices = localNumberOfDevices;
#endif

  msg << numberOfRanks << " Rank(s), ";
  msg << globalNumberOfThreads << " Core(s), ";
  msg << globalNumberOfDevices << " GPU(s)";

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    logInfo("dumpStatistics()", msg.str());
  }
}

#if defined(CompilerHasSysinfo)

int tarch::getTotalMemory(MemoryUsageFormat format) {
  struct sysinfo info;
  sysinfo( &info );

  return convertMemorySize(info.totalram,format);
}


int tarch::getFreeMemory(MemoryUsageFormat format) {
  struct sysinfo info;
  sysinfo( &info );

  return convertMemorySize(info.freeram,format);
}

#else

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
  if(0 == getrusage(RUSAGE_SELF, &usage)) {
    long freeMem = tarch::getTotalMemory( tarch::MemoryUsageFormat::MByte ) - usage.ru_maxrss;
    return convertMemorySize(freeMem, format);
  }
  else
    return 0;
}

#endif

int tarch::getMemoryUsage(MemoryUsageFormat format) {
  static tarch::logging::Log _log( "tarch" );

#if defined(CompilerHasProcStat)
  char   work[4096];
  FILE*  f;
  char*  pCh;

  pid_t pid = getpid();

  sprintf(work, "/proc/%d/stat", (int)pid);
  f = fopen(work, "r");

  if (f == NULL) {
    logError("getMemoryUsage()", "can't open file " << work );
    return(0);
  }
  if(fgets(work, sizeof(work), f) == NULL) {
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

  return convertMemorySize(rawInput,format);
}
