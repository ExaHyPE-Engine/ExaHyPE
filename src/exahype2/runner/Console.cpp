// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Console.h"

#include "tarch/Assertions.h"
#include "tarch/MemoryUsage.h"
#include "tarch/instrumentation/Info.h"

namespace {
  /**
   * Columns of the text inside the frame; the frame adds two columns on either side.
   */
  constexpr std::size_t ReportWidth = 93;

  /**
   * Durations of more than this many days render as a bounded text, so the field width stays fixed.
   */
  constexpr int MaxDurationDays = 999;

  constexpr std::string_view Separator = " · ";

  struct LineBreak {
    std::size_t keptBytes{0};
    std::size_t skippedBytes{0};
  };

  /**
   * Returns the break of a line after the last separator that fits, else at the last space, else
   * at the width. The separator stays on the broken line, so it ends with " ·".
   */
  LineBreak getLineBreak(const std::string& prefix, const std::size_t indentationBytes) {
    const auto separator = prefix.rfind(Separator);
    if (separator != std::string::npos and separator > indentationBytes) {
      return {separator + Separator.size() - 1, separator + Separator.size()};
    }
    const auto whitespace = prefix.find_last_of(' ');
    if (whitespace != std::string::npos and whitespace > indentationBytes) {
      return {whitespace, whitespace + 1};
    }
    return {prefix.size(), prefix.size()};
  }

  std::string getBorder(const std::string_view left, const std::string_view right) {
    std::string border(left);
    for (std::size_t column = 0; column < ReportWidth + 2; ++column) {
      border += "─";
    }
    border += right;
    return border;
  }
} // namespace

std::string
exahype2::runner::Format::getScientific(const double value) {
  std::ostringstream result;
  result << std::scientific << std::setprecision(4) << value;
  return result.str();
}

std::string
exahype2::runner::Format::getMemory(const unsigned long long bytes) {
  const auto         format = tarch::chooseMemoryUsageFormat(bytes);
  std::ostringstream result;
  result << std::fixed << std::setprecision(2) << tarch::convertMemorySize(bytes, format) << " "
         << tarch::toString(format);
  return result.str();
}

std::string
exahype2::runner::Format::getPercentage(const double value) {
  std::ostringstream result;
  result << std::fixed << std::setprecision(2) << value << " %";
  return result.str();
}

std::string
exahype2::runner::Format::getWallClock(const double seconds) {
  std::ostringstream result;
  result << "(" << std::fixed << std::setprecision(4) << seconds << " s)";
  return result.str();
}

std::string
exahype2::runner::Format::getRate(const double value, const char* const unit) {
  return getScientific(value) + " " + unit + "/s";
}

std::string
exahype2::runner::Format::getGroupedNumber(const unsigned long value) {
  const auto  digits = std::to_string(value);
  std::string result;
  for (std::size_t digitIndex = 0; digitIndex < digits.size(); ++digitIndex) {
    if (digitIndex > 0 and (digits.size() - digitIndex) % 3 == 0) {
      result.push_back(',');
    }
    result.push_back(digits[digitIndex]);
  }
  return result;
}

/**
 * Renders the two most significant units of a duration and clamps long ones, so the width stays bounded.
 */
std::string
exahype2::runner::Format::getDuration(const double seconds) {
  // The clamp precedes the integer cast, so a huge or infinite duration neither overflows nor widens the field.
  constexpr long SecondsPerHour = 3600L;
  constexpr long SecondsPerDay  = 24L * SecondsPerHour;
  const auto     capSeconds     = static_cast<double>((MaxDurationDays + 1) * SecondsPerDay);
  const long     total          = std::lround(std::min(std::max(seconds, 0.0), capSeconds));

  const long days             = total / SecondsPerDay;
  const long hours            = (total % SecondsPerDay) / SecondsPerHour;
  const long minutes          = (total % SecondsPerHour) / 60;
  const long remainingSeconds = total % 60;

  std::ostringstream result;
  if (days > MaxDurationDays) {
    result << ">" << MaxDurationDays << "d";
  } else if (days > 0) {
    result << days << "d " << std::setfill('0') << std::setw(2) << hours << "h";
  } else if (hours > 0) {
    result << hours << "h " << std::setfill('0') << std::setw(2) << minutes << "m";
  } else if (minutes > 0) {
    result << minutes << "m " << std::setfill('0') << std::setw(2) << remainingSeconds << "s";
  } else {
    result << remainingSeconds << "s";
  }
  return result.str();
}

/**
 * Counts UTF-8 code points as terminal columns. This is valid for the report's Latin letters, the
 * glyphs λ, · and → and the frame glyphs; combining and wide characters are outside the supported
 * input.
 */
std::size_t
exahype2::runner::getDisplayWidth(const std::string& text) {
  std::size_t columns = 0;
  for (const unsigned char character : text) {
    if ((character & 0xC0) != 0x80) {
      ++columns;
    }
  }
  return columns;
}

std::string
exahype2::runner::truncateToDisplayWidth(const std::string& text, const std::size_t columns) {
  std::size_t seenColumns = 0;
  for (std::size_t byteIndex = 0; byteIndex < text.size(); ++byteIndex) {
    if ((static_cast<unsigned char>(text[byteIndex]) & 0xC0) != 0x80) {
      if (seenColumns == columns) {
        return text.substr(0, byteIndex);
      }
      ++seenColumns;
    }
  }
  return text;
}

std::string
exahype2::runner::padToWidth(const std::string& text, const std::size_t columns) {
  const auto width = exahype2::runner::getDisplayWidth(text);
  return width < columns ? text + std::string(columns - width, ' ') : text;
}

/**
 * Breaks a line wider than columns into lines of at most columns. Every continuation starts at
 * continuationColumn, or at the first column when continuationColumn leaves no room for text.
 */
std::vector<std::string>
exahype2::runner::wrapToDisplayWidth(
  const std::string& text,
  const std::size_t  columns,
  const std::size_t  continuationColumn
) {
  if (columns == 0 or exahype2::runner::getDisplayWidth(text) <= columns) {
    return {text};
  }
  const auto               continuation = std::string(continuationColumn < columns ? continuationColumn : 0, ' ');
  std::vector<std::string> lines;
  auto                     remaining = text;
  while (exahype2::runner::getDisplayWidth(remaining) > columns) {
    const auto prefix           = exahype2::runner::truncateToDisplayWidth(remaining, columns);
    const auto firstNonSpace    = remaining.find_first_not_of(' ');
    const auto indentationBytes = firstNonSpace == std::string::npos ? std::size_t{0} : firstNonSpace;
    const auto lineBreak        = getLineBreak(prefix, indentationBytes);
    assertion(lineBreak.skippedBytes > indentationBytes, text, columns);
    lines.push_back(remaining.substr(0, lineBreak.keptBytes));
    remaining.erase(0, lineBreak.skippedBytes);
    const auto nextNonSpace = remaining.find_first_not_of(' ');
    remaining.erase(0, nextNonSpace == std::string::npos ? remaining.size() : nextNonSpace);
    if (not remaining.empty()) {
      remaining.insert(0, continuation);
    }
  }
  if (not remaining.empty()) {
    lines.push_back(remaining);
  }
  return lines;
}

exahype2::runner::Table::Section&
exahype2::runner::Table::getSection(const std::string& title) {
  for (auto& section : _sections) {
    if (section.title == title) {
      return section;
    }
  }
  _sections.push_back(exahype2::runner::Table::Section{title, {}});
  return _sections.back();
}

void
exahype2::runner::Table::add(const std::string& section, const std::string& label, const std::string& value) {
  getSection(section).entries.emplace_back(label, value);
}

std::vector<std::string>
exahype2::runner::Table::render() const {
  auto labelWidth = std::size_t{0};
  for (const auto& section : _sections) {
    for (const auto& entry : section.entries) {
      labelWidth = std::max(labelWidth, entry.first.size());
    }
  }
  std::vector<std::string> lines;
  for (const auto& section : _sections) {
    if (section.entries.empty()) {
      continue;
    }
    lines.push_back(std::string(exahype2::runner::BlockIndent, ' ') + section.title);
    for (const auto& [label, value] : section.entries) {
      std::ostringstream line;
      line << std::string(2 * exahype2::runner::BlockIndent, ' ') << std::left
           << std::setw(static_cast<int>(labelWidth)) << label << "  " << value;
      lines.push_back(line.str());
    }
  }
  return lines;
}

void
exahype2::runner::writeToLog(const std::string& line) {
  logInfo(line);
}

exahype2::runner::Report::Report(const Writer& writer) :
  _writer(writer) {
  assertion(_writer != nullptr);
}

/**
 * Writes a thin rule across the frame; a closed frame opens with its top border.
 */
void
exahype2::runner::Report::rule() {
  if (_lastWasRule) {
    return;
  }
  _writer(_open ? getBorder("├", "┤") : getBorder("┌", "┐"));
  _open        = true;
  _lastWasRule = true;
}

/**
 * Continues a broken line at its own indentation.
 */
void
exahype2::runner::Report::line(const std::string& text) {
  const auto firstNonSpace = text.find_first_not_of(' ');
  line(text, firstNonSpace == std::string::npos ? 0 : firstNonSpace);
}

void
exahype2::runner::Report::line(const std::string& text, const std::size_t continuationColumn) {
  if (text.empty()) {
    return;
  }
  if (not _open) {
    rule();
  }
  for (const auto& wrappedLine : exahype2::runner::wrapToDisplayWidth(text, ReportWidth, continuationColumn)) {
    assertion(exahype2::runner::getDisplayWidth(wrappedLine) <= ReportWidth, wrappedLine);
    _writer("│ " + exahype2::runner::padToWidth(wrappedLine, ReportWidth) + " │");
  }
  _lastWasRule = false;
}

/**
 * Writes the bottom border of an open frame, so the next line or rule opens a new frame.
 */
void
exahype2::runner::Report::close() {
  if (_open) {
    _writer(getBorder("└", "┘"));
  }
  _open        = false;
  _lastWasRule = false;
}
