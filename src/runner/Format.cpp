// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Format.h"

#include "tarch/MemoryUsage.h"

#include <algorithm>
#include <iomanip>

namespace {
  // Durations of at least this many days render as ">Nd" so the formatted width stays bounded.
  constexpr int MaxDurationDays = 999;
} // namespace

std::string runner::Format::scientific(double value) {
  std::ostringstream result;
  result << std::scientific << std::setprecision(4) << value;
  return result.str();
}

std::string runner::Format::memory(unsigned long long bytes) {
  const auto         format = tarch::chooseMemoryUsageFormat(bytes);
  std::ostringstream result;
  result << std::fixed << std::setprecision(2) << tarch::convertMemorySize(bytes, format) << " "
         << tarch::toString(format);
  return result.str();
}

std::string runner::Format::percentage(double value) {
  std::ostringstream result;
  result << std::fixed << std::setprecision(2) << value << " %";
  return result.str();
}

std::string runner::Format::wallClock(double seconds) {
  std::ostringstream result;
  result << "(" << std::fixed << std::setprecision(2) << seconds << " s)";
  return result.str();
}

std::string runner::Format::rate(double value, const char* unit) { return scientific(value) + " " + unit + "/s"; }

std::string runner::Format::grouped(unsigned long value) {
  const auto  digits = std::to_string(value);
  std::string result;
  for (std::size_t i = 0; i < digits.size(); ++i) {
    if (i > 0 and (digits.size() - i) % 3 == 0) {
      result.push_back(',');
    }
    result.push_back(digits[i]);
  }
  return result;
}

std::string runner::Format::duration(double seconds) {
  // Clamp before the integer cast so a huge or infinite ETA neither overflows nor widens the field.
  const double capSeconds = static_cast<double>(MaxDurationDays + 1) * 24.0 * 3600.0;
  const auto   total      = static_cast<long>(std::min(std::max(seconds, 0.0), capSeconds) + 0.5);

  const long days    = total / (24 * 3600);
  const long hours   = (total % (24 * 3600)) / 3600;
  const long minutes = (total % 3600) / 60;
  const long secs    = total % 60;

  std::ostringstream result;
  if (days > MaxDurationDays) {
    result << ">" << MaxDurationDays << "d";
  } else if (days > 0) {
    result << days << "d " << std::setfill('0') << std::setw(2) << hours << "h";
  } else if (hours > 0) {
    result << hours << "h " << std::setfill('0') << std::setw(2) << minutes << "m";
  } else if (minutes > 0) {
    result << minutes << "m " << std::setfill('0') << std::setw(2) << secs << "s";
  } else {
    result << secs << "s";
  }
  return result.str();
}
