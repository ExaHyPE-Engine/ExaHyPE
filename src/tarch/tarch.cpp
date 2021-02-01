#include "tarch.h"
#include "tarch/logging/Log.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

#include <iostream>

#if defined(CompilerHasProcStat)
#include <unistd.h>
#endif

#include <cstdlib>
#include <cstring>
#include <sys/sysinfo.h>


namespace {
  int convertMemorySize(long value, tarch::MemoryUsageFormat format) {
    static tarch::logging::Log _log( "tarch" );
	int result;
    switch (format) {
      case tarch::MemoryUsageFormat::MByte:
        {
	      long megaByte = 1024 * 1024;
	      long usageMB ((value + (megaByte/2)) / megaByte );
	      const long maxInteger = std::numeric_limits<int>::max();
	      if (usageMB>maxInteger) {
	        logError( "convertMemorySize()", "cannot cast result to return value: " << usageMB );
	      }
	      result = static_cast<int>(usageMB);
	      }
	  }
    return result;
  }
}


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
