// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CPUFeatureDetection.h"

#if defined(__x86_64__) or defined(_M_X64) or defined(__i386) or defined(_M_IX86)
#define CPUFEAT_X86 1
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

#if defined(__linux__) and (defined(__aarch64__) or defined(_M_ARM64))
#include <asm/hwcap.h>
#include <sys/auxv.h>
#ifndef HWCAP_SVE
#define HWCAP_SVE (1 << 22) // Older kernel headers lack HWCAP_SVE.
#endif
#endif

namespace {
#ifdef CPUFEAT_X86
  void readCPUID(int (&registers)[4], const int leaf, const int subleaf) {
#if defined(_MSC_VER)
    __cpuidex(registers, leaf, subleaf);
#else
    __cpuid_count(leaf, subleaf, registers[0], registers[1], registers[2], registers[3]);
#endif
  }

  /**
   * Returns whether the operating system saves the SSE and the AVX registers on a context switch.
   */
  bool isAVXStateEnabled() {
#if defined(_MSC_VER)
    const unsigned long long enabledStates = _xgetbv(0);
#else
    std::uint32_t lowerBits;
    std::uint32_t upperBits;
    asm volatile("xgetbv" : "=a"(lowerBits), "=d"(upperBits) : "c"(0));
    const std::uint64_t enabledStates = (static_cast<std::uint64_t>(upperBits) << 32) | lowerBits;
#endif
    return (enabledStates & 0x6) == 0x6;
  }
#endif

  const tarch::CPUFeatures& getCachedCPUFeatures() {
    static const tarch::CPUFeatures features = tarch::CPUFeatures::detect();
    return features;
  }
} // namespace

/**
 * Returns the SIMD instruction sets that the CPU and the operating system support.
 */
tarch::CPUFeatures
tarch::CPUFeatures::detect() {
  tarch::CPUFeatures features{};

#ifdef CPUFEAT_X86
  int registers[4] = {};
  readCPUID(registers, 1, 0);

  const bool systemEnablesXSAVE = (registers[2] & (1 << 27)) != 0;
  const bool processorHasAVX    = (registers[2] & (1 << 28)) != 0;

  features.sse2 = (registers[3] & (1 << 26)) != 0;
  features.avx  = processorHasAVX and systemEnablesXSAVE and isAVXStateEnabled();

  if (features.avx) {
    readCPUID(registers, 7, 0);
    features.avx2   = (registers[1] & (1 << 5)) != 0;
    features.avx512 = (registers[1] & (1 << 16)) != 0;
  }
#elif defined(__aarch64__) or defined(_M_ARM64)
  features.neon = true;
#if defined(__linux__)
  features.sve = (getauxval(AT_HWCAP) & HWCAP_SVE) != 0;
#elif defined(__ARM_FEATURE_SVE)
  features.sve = true;
#endif
#elif defined(__ARM_NEON) or defined(__ARM_NEON__)
  features.neon = true;
#endif

  return features;
}

/**
 * Returns the names of the supported instruction sets separated by commas, or None.
 */
std::string
tarch::detectCPUFeatures() {
  const tarch::CPUFeatures& features = getCachedCPUFeatures();

  const std::pair<bool, const char*> namedFeatures[] = {
    {features.sse2, "SSE2"},
    {features.avx, "AVX"},
    {features.avx2, "AVX2"},
    {features.avx512, "AVX512"},
    {features.neon, "NEON"},
    {features.sve, "SVE"}
  };

  std::string result;
  for (const auto& [isSupported, name] : namedFeatures) {
    if (isSupported) {
      result += result.empty() ? "" : ", ";
      result += name;
    }
  }
  return result.empty() ? "None" : result;
}
