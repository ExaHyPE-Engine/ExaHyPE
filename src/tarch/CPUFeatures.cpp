// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CPUFeatures.h"

#include <sstream>
#include <string>
#include <vector>

// x86 detection headers
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#define CPUFEAT_X86 1
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#endif

// ARM detection headers (Linux specific for runtime SVE detection)
#if defined(__linux__) && (defined(__aarch64__) || defined(_M_ARM64))
#include <asm/hwcap.h>
#include <sys/auxv.h>
#ifndef HWCAP_SVE
#define HWCAP_SVE (1 << 22) // Ensure definition if older headers are used
#endif
#endif

#ifdef CPUFEAT_X86
static void run_cpuid(int regs[4], int leaf, int subleaf = 0) {
#if defined(_MSC_VER)
  __cpuidex(regs, leaf, subleaf);
#else
  __cpuid_count(leaf, subleaf, regs[0], regs[1], regs[2], regs[3]);
#endif
}

static bool check_xgetbv_avx_support() {
#if defined(_MSC_VER)
  unsigned long long mask = _xgetbv(0);
  return (mask & 0x6) == 0x6;
#else
  uint32_t eax, edx;
  asm volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(0));
  uint64_t xcr0 = ((uint64_t)edx << 32) | eax;
  return (xcr0 & 0x6) == 0x6;
#endif
}
#endif

tarch::CPUFeatures tarch::CPUFeatures::detect() {
  CPUFeatures f{};

#ifdef CPUFEAT_X86
  int regs[4] = {};

  run_cpuid(regs, 1);

  // Check XSAVE support (OSXSAVE) and AVX hardware support
  bool osXSAVE = regs[2] & (1 << 27);
  bool avxHW   = regs[2] & (1 << 28);

  f.sse2 = regs[3] & (1 << 26);

  // Check OS support for AVX (saving YMM registers)
  bool avxOS = false;
  if (osXSAVE && avxHW) {
    avxOS = check_xgetbv_avx_support();
  }

  f.avx = avxHW && osXSAVE && avxOS;

  // Extended features (leaf 7)
  if (f.avx) {
    run_cpuid(regs, 7, 0);
    f.avx2   = regs[1] & (1 << 5);
    f.avx512 = regs[1] & (1 << 16);
  }

#elif defined(__aarch64__) || defined(_M_ARM64)
  // --- ARM AArch64 ---
  f.neon = true;
  f.sse2 = true; // Conceptually similar

// SVE Detection
#if defined(__linux__)
  unsigned long hwcaps = getauxval(AT_HWCAP);
  if (hwcaps & HWCAP_SVE) {
    f.sve = true;
  }
#else
#if defined(__ARM_FEATURE_SVE)
  f.sve = true;
#endif
#endif

#elif defined(__ARM_NEON) || defined(__ARM_NEON__)
  // --- 32-bit ARM or other fallback ---
  f.neon = true;
  f.sse2 = true;
#endif

  return f;
}

static const tarch::CPUFeatures& get_cached_features() {
  static tarch::CPUFeatures f = tarch::CPUFeatures::detect();
  return f;
}

std::string tarch::getCPUFeatures() {
  const auto&              f = get_cached_features();
  std::vector<std::string> features;

  if (f.sse2)
    features.push_back("SSE2");
  if (f.avx)
    features.push_back("AVX");
  if (f.avx2)
    features.push_back("AVX2");
  if (f.avx512)
    features.push_back("AVX512");
  if (f.neon)
    features.push_back("NEON");
  if (f.sve)
    features.push_back("SVE");

  if (features.empty()) {
    return "None";
  }

  std::ostringstream os;
  for (size_t i = 0; i < features.size(); ++i) {
    os << features[i];
    if (i + 1 < features.size()) {
      os << ", ";
    }
  }
  return os.str();
}
