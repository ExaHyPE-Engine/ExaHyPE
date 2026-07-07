// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace runner {
  /** Indent (spaces) of a line inside the report box; nested entries use a multiple of it. */
  constexpr std::size_t BlockIndent = 2;

  /** Label/value entries grouped into named sections, rendered as lines with aligned labels. */
  class Table final {
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
   * The single "|* ... *|" frame all phases share. Every line is truncated and padded to the fixed
   * width, so it stays rectangular; setWidth() must be called before the first output.
   */
  class ReportBox final {
  public:
    void setWidth(int width);
    int  width() const { return _width; }

    void rule();
    void line(const std::string& text);
    void blank();

  private:
    int  _width          = 0;     // inner content width, fixed once the box is open
    bool _started        = false; // suppresses a blank above the very first rule
    bool _lastWasRule    = false; // collapses consecutive rules
    bool _lastWasBlank   = false; // collapses consecutive blank lines
    bool _blankAfterRule = false; // a rule was just drawn -> pad the top of the next section

    void emit(const std::string& text);
  };
} // namespace runner
