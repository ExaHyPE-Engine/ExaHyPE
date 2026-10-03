// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CompilerSpecificSettings.h"

#if defined(__APPLE__)
#include <Availability.h>
#endif

// clang-format off
// The Microsoft compiler reports the language level in _MSVC_LANG, because its __cplusplus stays at
// 199711L without /Zc:__cplusplus.
#if defined(_MSVC_LANG)
#define PEANO_CPLUSPLUS _MSVC_LANG
#else
#define PEANO_CPLUSPLUS __cplusplus
#endif
#if PEANO_CPLUSPLUS < 202002L
#error "Peano must be compiled as C++20 or newer (e.g. -std=c++20 or /std:c++20)."
#endif
#undef PEANO_CPLUSPLUS

#if not defined(__cpp_lib_filesystem) or __cpp_lib_filesystem < 201703L
#error "Peano needs a standard library providing <filesystem> (__cpp_lib_filesystem >= 201703L): libstdc++ >= 9 (we require >= 12), libc++ >= 9, or MSVC STL."
#endif
#if not defined(__cpp_lib_source_location) or __cpp_lib_source_location < 201907L
#error "Peano needs std::source_location (__cpp_lib_source_location >= 201907L): libstdc++ >= 11 or a recent libc++ (LLVM >= 16)."
#endif
#if not defined(__cpp_lib_ranges) or __cpp_lib_ranges < 201911L
#error "Peano needs the C++20 ranges library (__cpp_lib_ranges >= 201911L)."
#endif

#if defined(_GLIBCXX_RELEASE) and _GLIBCXX_RELEASE < 12
#error "Peano requires libstdc++ >= 12 (as shipped with GCC >= 12). When building with Clang or Intel, load a GCC >= 12 toolchain so a recent libstdc++ is used."
#endif

// On Apple, the <filesystem> headers exist below a macOS 10.15 deployment target, but their symbols
// do not.
#if defined(__APPLE__) and defined(__MAC_OS_X_VERSION_MIN_REQUIRED) and __MAC_OS_X_VERSION_MIN_REQUIRED < 101500
#error "Peano needs a macOS 10.15+ deployment target for std::filesystem (set CMAKE_OSX_DEPLOYMENT_TARGET)."
#endif
// clang-format on

static_assert(sizeof(void*) == 8, "Assuming a 64-bit build with 8-byte pointers");
static_assert(std::numeric_limits<float>::is_iec559, "Float must be an IEEE 754 type");
static_assert(std::numeric_limits<double>::is_iec559, "Double must be an IEEE 754 type");
static_assert(CHAR_BIT == 8, "A byte must have 8 bits");
static_assert(sizeof(char) == 1, "Char must be one byte");
static_assert(sizeof(short) == 2, "Short must be two bytes");
static_assert(sizeof(int) == 4, "Int must be four bytes");
static_assert(sizeof(long long) == 8, "Long long must be eight bytes");
static_assert('A' == 65, "The execution character set must be ASCII");
