// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace exahype2::runner {
  constexpr std::size_t BlockIndent = 2;

  struct Format;
  class Table;
  class Report;

  std::size_t getDisplayWidth(const std::string& text);
  std::string truncateToDisplayWidth(const std::string& text, std::size_t columns);
  std::string padToWidth(const std::string& text, std::size_t columns);
  std::vector<std::string> wrapToDisplayWidth(
    const std::string& text,
    std::size_t        columns,
    std::size_t        continuationColumn
  );
  void writeToLog(const std::string& line);
} // namespace exahype2::runner

/**
 * Renders numbers, sizes and durations for the reports.
 */
struct exahype2::runner::Format {
  static std::string getScientific(double value);
  static std::string getMemory(unsigned long long bytes);
  static std::string getPercentage(double value);
  static std::string getWallClock(double seconds);
  static std::string getRate(double value, const char* unit);
  static std::string getGroupedNumber(unsigned long value);
  static std::string getDuration(double seconds);

  template <typename T>
  static INLINE_METHOD std::string getNumber(const T& value);
};

/**
 * Groups entries by section and aligns the labels of every section in one column.
 */
class exahype2::runner::Table final {
public:
  void add(const std::string& section, const std::string& label, const std::string& value);
  [[nodiscard]] std::vector<std::string> render() const;

private:
  struct Section {
    std::string                                      title{};
    std::vector<std::pair<std::string, std::string>> entries{};
  };

  std::vector<Section> _sections{};

  Section& getSection(const std::string& title);
};

/**
 * Writes the report lines inside a frame of thin lines. The first line or rule opens the frame
 * and close() closes it. A line wider than the frame breaks after a separator, an empty line
 * writes nothing, and a rule never follows a rule.
 */
class exahype2::runner::Report final {
public:
  using Writer = std::function<void(const std::string& line)>;

  Report() = default;
  explicit Report(const Writer& writer);

  void rule();
  void line(const std::string& text);
  void line(const std::string& text, std::size_t continuationColumn);
  void close();

private:
  Writer _writer{exahype2::runner::writeToLog};
  bool   _open{false};
  bool   _lastWasRule{false};
};

#include "exahype2/runner/Console.cpph"
