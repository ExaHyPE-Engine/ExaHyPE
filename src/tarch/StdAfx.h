// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef __cplusplus
// NVC++ 25.7 requires <bit> before <algorithm> and <array> to expose std::bit_cast.
#if defined(__NVCOMPILER)
#include <bit>
#endif
#include <algorithm>
#include <array>
#include <atomic>
#if !defined(__NVCOMPILER)
#include <bit>
#endif
#include <bitset>
#include <cassert>
#include <cctype>
#include <cfenv>
#include <charconv>
#include <chrono>
#include <climits>
#include <cmath>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <deque>
#include <filesystem>
#include <fstream>
#include <functional>
#include <initializer_list>
#include <iomanip>
#include <iosfwd>
#include <iostream>
#include <iterator>
#include <limits>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <new>
#include <numbers>
#include <numeric>
#include <optional>
#include <ostream>
#include <queue>
#include <random>
#include <ranges>
#include <set>
#include <shared_mutex>
#include <source_location>
#include <sstream>
#include <stdexcept>
#if __has_include(<stdfloat>)
#include <stdfloat>
#endif
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>
#include <version>
#endif

// Valid in both C and C++.
#include <stdio.h>
#include <string.h>
