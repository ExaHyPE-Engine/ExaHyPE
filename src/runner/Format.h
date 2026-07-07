// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <sstream>
#include <string>

namespace runner {
  struct Format {
    static std::string scientific(double value);
    static std::string memory(unsigned long long bytes);
    static std::string percentage(double value);
    static std::string wallClock(double seconds);
    static std::string rate(double value, const char* unit);
    static std::string grouped(unsigned long value);

    // Two most-significant units (e.g. "12m 03s", "5d 03h"); clamped so the width stays bounded.
    static std::string duration(double seconds);

    template <typename T>
    static std::string number(const T& value) {
      std::ostringstream result;
      result << value;
      return result.str();
    }
  };
} // namespace runner
