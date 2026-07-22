// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryUsage.h"

#include "tarch/instrumentation/Log.h"

#if defined(COMPILER_HAS_PROC_STAT)
#include <unistd.h>
#endif
#if defined(COMPILER_HAS_SYSINFO) // for Linux OS
#include <sys/sysinfo.h>
#else // for Mac OS support
#include <mach/mach.h>
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

std::uint64_t tarch::getTotalMemoryInBytes() {
  struct sysinfo info;
  sysinfo(&info);
  // sysinfo reports memory in multiples of mem_unit bytes, not bytes.
  return static_cast<std::uint64_t>(info.totalram) * info.mem_unit;
}

std::uint64_t tarch::getFreeMemoryInBytes() {
  struct sysinfo info;
  sysinfo(&info);
  // sysinfo reports memory in multiples of mem_unit bytes, not bytes.
  return static_cast<std::uint64_t>(info.freeram) * info.mem_unit;
}

#else // defined(COMPILER_HAS_SYSINFO)

std::uint64_t tarch::getTotalMemoryInBytes() {
  int     mib[] = {CTL_HW, HW_MEMSIZE};
  int64_t physical_memory;
  size_t  length;

  // Get the physical memory size
  length = sizeof(int64_t);
  sysctl(mib, 2, &physical_memory, &length, nullptr, 0);
  return static_cast<std::uint64_t>(physical_memory);
}

std::uint64_t tarch::getFreeMemoryInBytes() {
  // System-wide free physical memory is the count of free pages from the Mach VM statistics (strictly
  // free, mirroring the Linux freeram above).
  const mach_port_t      host     = mach_host_self();
  vm_size_t              pageSize = 0;
  vm_statistics64_data_t stats;
  mach_msg_type_number_t count = HOST_VM_INFO64_COUNT;

  const bool ok = host_page_size(host, &pageSize) == KERN_SUCCESS
    and host_statistics64(host, HOST_VM_INFO64, reinterpret_cast<host_info64_t>(&stats), &count) == KERN_SUCCESS;
  mach_port_deallocate(mach_task_self(), host);
  if (not ok) {
    return 0;
  }
  return static_cast<std::uint64_t>(stats.free_count) * pageSize;
}

#endif // defined(COMPILER_HAS_SYSINFO)

double tarch::getTotalMemory(const MemoryUsageFormat format) {
  return convertMemorySize(getTotalMemoryInBytes(), format);
}

double tarch::getFreeMemory(const MemoryUsageFormat format) {
  return convertMemorySize(getFreeMemoryInBytes(), format);
}

std::uint64_t tarch::getMemoryUsageInBytes() {
#if defined(COMPILER_HAS_PROC_STAT)
  char  work[4096];
  FILE* f   = nullptr;
  char* pCh = nullptr;

  pid_t pid = getpid();

  sprintf(work, "/proc/%d/stat", (int)pid);
  f = fopen(work, "r");

  if (f == nullptr) {
    logError("Can't open file " << work);
    return (0);
  }
  if (fgets(work, sizeof(work), f) == nullptr) {
    logError("Error while reading from file");
  }
  fclose(f);

  // Field 24 of /proc/<pid>/stat is the resident set size in pages (field 23 is the virtual size).
  // Field 2 (comm) is parenthesised and may itself contain spaces, so start parsing after its closing ')'.
  char* afterComm = strrchr(work, ')');
  if (afterComm == nullptr) {
    logError("Unexpected format in " << work);
    return 0;
  }
  pCh = strtok(afterComm + 1, " "); // field 3 (state)
  for (int i = 3; i < 24 and pCh != nullptr; i++) {
    pCh = strtok(nullptr, " ");
  }
  if (pCh == nullptr) {
    logError("Could not read resident set size from /proc stat");
    return 0;
  }

  std::size_t residentPages = atol(pCh);
  std::size_t rawInput      = residentPages * static_cast<std::size_t>(sysconf(_SC_PAGESIZE));
#elif defined(__APPLE__)
  mach_task_basic_info_data_t taskInfo;
  mach_msg_type_number_t      count    = MACH_TASK_BASIC_INFO_COUNT;
  std::size_t                 rawInput = 0;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&taskInfo), &count)
      == KERN_SUCCESS) {
    rawInput = taskInfo.resident_size;
  }
#else
  std::size_t rawInput = 0;
#endif

  return static_cast<std::uint64_t>(rawInput);
}

double tarch::getMemoryUsage(const MemoryUsageFormat format) {
  return convertMemorySize(getMemoryUsageInBytes(), format);
}
