// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace runner {
  constexpr std::size_t BlockIndent = 2;
  class Table;
  class ReportBox;
} // namespace runner

/** Groups entries by section and aligns labels across rendered rows. */
class runner::Table final {
public:
  void                     add(const std::string& section, const std::string& label, const std::string& value);
  std::vector<std::string> render() const;

private:
  struct Section {
    std::string                                      title;
    std::vector<std::pair<std::string, std::string>> entries;
  };
  std::vector<Section> _sections;
  Section&             sectionFor(const std::string& title);
};

/**
 * Writes the shared, fixed-width report frame. Long lines wrap, repeated rules and blanks collapse,
 * and each ruled section receives one surrounding blank line.
 */
class runner::ReportBox final {
public:
  void rule();
  void line(const std::string& text);
  void blank();

private:
  bool _started        = false;
  bool _lastWasRule    = false;
  bool _lastWasBlank   = false;
  bool _blankAfterRule = false;

  void emit(const std::string& text);
};
