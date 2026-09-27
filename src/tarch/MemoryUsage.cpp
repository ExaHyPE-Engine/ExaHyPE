// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MemoryUsage.h"

#include "tarch/instrumentation/Error.h"

#if defined(COMPILER_HAS_PROC_STAT)
#include <unistd.h>
#endif
#if defined(COMPILER_HAS_SYSINFO) // Linux
#include <sys/sysinfo.h>
#else // macOS
#include <mach/mach.h>
#include <sys/sysctl.h>
#endif

std::string
tarch::toString(const MemoryUsageFormat format) {
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

double
tarch::convertMemorySize(const unsigned long long bytes, const MemoryUsageFormat format) {
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

std::uint64_t
tarch::getTotalMemoryInBytes() {
  struct sysinfo info;
  sysinfo(&info);
  // The sysinfo fields count multiples of mem_unit bytes.
  return static_cast<std::uint64_t>(info.totalram) * info.mem_unit;
}

namespace {
  /** Returns one field of /proc/meminfo in KiB, or nothing when the kernel does not report it. */
  std::optional<std::uint64_t> readMemoryInformationInKiB(const std::string_view field) {
    std::ifstream memoryInformation("/proc/meminfo");
    std::string   line;
    while (std::getline(memoryInformation, line)) {
      if (line.size() > field.size() and line.starts_with(field) and line[field.size()] == ':') {
        std::istringstream value(line.substr(field.size() + 1));
        std::uint64_t      kibibytes = 0;
        if (value >> kibibytes) {
          return kibibytes;
        }
      }
    }
    return std::nullopt;
  }
} // namespace

/**
 * Returns the bytes a new allocation obtains without swapping. The kernel's estimate
 * MemAvailable adds the reclaimable page cache to the free memory; a kernel without the estimate
 * reports the free memory and the buffers.
 */
std::uint64_t
tarch::getAvailableMemoryInBytes() {
  if (const auto kibibytes = readMemoryInformationInKiB("MemAvailable")) {
    return *kibibytes * static_cast<std::uint64_t>(KiB);
  }
  struct sysinfo info;
  sysinfo(&info);
  // The sysinfo fields count multiples of mem_unit bytes.
  return (static_cast<std::uint64_t>(info.freeram) + info.bufferram) * info.mem_unit;
}

#else // defined(COMPILER_HAS_SYSINFO)

std::uint64_t
tarch::getTotalMemoryInBytes() {
  int     mib[] = {CTL_HW, HW_MEMSIZE};
  int64_t physicalMemory;
  size_t  length;

  length = sizeof(int64_t);
  sysctl(mib, 2, &physicalMemory, &length, nullptr, 0);
  return static_cast<std::uint64_t>(physicalMemory);
}

/** Returns the bytes of the free and the inactive pages, which a new allocation obtains without swapping. */
std::uint64_t
tarch::getAvailableMemoryInBytes() {
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
  return (static_cast<std::uint64_t>(stats.free_count) + stats.inactive_count) * pageSize;
}

#endif // defined(COMPILER_HAS_SYSINFO)

std::uint64_t
tarch::getMemoryUsageInBytes() {
#if defined(COMPILER_HAS_PROC_STAT)
  std::ifstream statFile("/proc/self/stat");
  std::string   line;
  if (not std::getline(statFile, line)) {
    logError("Cannot read /proc/self/stat");
    return 0;
  }
  // Field 24 of /proc/self/stat is the resident set size in pages. Field 2 is parenthesised and
  // may contain spaces, so the parsing starts after its closing bracket.
  const auto commandEnd = line.rfind(')');
  if (commandEnd == std::string::npos) {
    logError("Unexpected format of /proc/self/stat: " << line);
    return 0;
  }
  std::istringstream fields(line.substr(commandEnd + 1));
  std::string        skippedField;
  for (int fieldNumber = 3; fieldNumber < 24; fieldNumber++) {
    fields >> skippedField;
  }
  std::uint64_t residentPages = 0;
  if (not(fields >> residentPages)) {
    logError("Cannot read the resident set size from /proc/self/stat");
    return 0;
  }
  return residentPages * static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));
#elif defined(__APPLE__)
  mach_task_basic_info_data_t taskInfo;
  mach_msg_type_number_t      count = MACH_TASK_BASIC_INFO_COUNT;
  if (
    task_info(mach_task_self(), MACH_TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&taskInfo), &count) == KERN_SUCCESS
  ) {
    return static_cast<std::uint64_t>(taskInfo.resident_size);
  }
  return 0;
#else
  return 0;
#endif
}
