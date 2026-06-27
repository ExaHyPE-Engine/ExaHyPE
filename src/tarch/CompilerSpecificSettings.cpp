// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CompilerSpecificSettings.h"

#include <climits>
#include <limits>
#include <version>

#if defined(__APPLE__)
#include <Availability.h>
#endif

// --- C++ language level ------------------------------------------------------
// _MSVC_LANG mirrors __cplusplus for MSVC, whose __cplusplus stays at 199711L
// unless /Zc:__cplusplus is passed; everywhere else __cplusplus is authoritative.
#if defined(_MSVC_LANG)
#define PEANO_CPLUSPLUS _MSVC_LANG
#else
#define PEANO_CPLUSPLUS __cplusplus
#endif
#if PEANO_CPLUSPLUS < 202002L
#error "Peano must be compiled as C++20 or newer (e.g. -std=c++20 or /std:c++20)."
#endif
#undef PEANO_CPLUSPLUS

// --- Required C++20 standard-library features --------------------------------
#if !defined(__cpp_lib_filesystem) || __cpp_lib_filesystem < 201703L
#error "Peano needs a standard library providing <filesystem> (__cpp_lib_filesystem >= 201703L): libstdc++ >= 9 (we require >= 12), libc++ >= 9, or MSVC STL."
#endif
#if !defined(__cpp_lib_source_location) || __cpp_lib_source_location < 201907L
#error "Peano needs std::source_location (__cpp_lib_source_location >= 201907L): libstdc++ >= 11 or a recent libc++ (LLVM >= 16)."
#endif
#if !defined(__cpp_lib_ranges) || __cpp_lib_ranges < 201911L
#error "Peano needs the C++20 ranges library (__cpp_lib_ranges >= 201911L)."
#endif

// --- Known-bad standard-library versions -------------------------------------
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE < 12
#error "Peano requires libstdc++ >= 12 (as shipped with GCC >= 12). When building with Clang or Intel, load a GCC >= 12 toolchain so a recent libstdc++ is used."
#endif

// --- Apple platform availability ---------------------------------------------
// On Apple the <filesystem> headers exist but their symbols are unavailable
// below a macOS 10.15 deployment target.
#if defined(__APPLE__) && defined(__MAC_OS_X_VERSION_MIN_REQUIRED) && __MAC_OS_X_VERSION_MIN_REQUIRED < 101500
#error "Peano needs a macOS 10.15+ deployment target for std::filesystem (set CMAKE_OSX_DEPLOYMENT_TARGET)."
#endif

// --- Build-platform invariants -----------------------------------------------
static_assert(sizeof(void*) == 8, "Assuming 64-bit build; i.e., 8-byte pointers");
static_assert(std::numeric_limits<float>::is_iec559, "std::numeric_limits<float>::is_iec559");
static_assert(std::numeric_limits<double>::is_iec559, "std::numeric_limits<double>::is_iec55");
static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");
static_assert(sizeof(char) == 1, "sizeof(short) != 1");
static_assert(sizeof(short) == 2, "sizeof(short) != 2");
static_assert(sizeof(int) == 4, "sizeof(int) != 4");
static_assert(sizeof(long long) == 8, "sizeof(long long) != 8");
static_assert('A' == 65, "A != 65");
