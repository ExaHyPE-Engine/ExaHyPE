// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReportBox.h"

#include "runner/TextLayout.h"
#include "tarch/instrumentation/Info.h"

// ---------------------------------------------------------------------------
// Table
// ---------------------------------------------------------------------------
runner::Table::Section& runner::Table::sectionFor(const std::string& title) {
  for (auto& section : _sections) {
    if (section.title == title) {
      return section;
    }
  }
  _sections.push_back(Section{title, {}});
  return _sections.back();
}

void runner::Table::add(const std::string& section, const std::string& label, const std::string& value) {
  sectionFor(section).entries.emplace_back(label, value);
}

std::vector<std::string> runner::Table::render() const {
  std::size_t labelWidth = 0;
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
    lines.push_back(std::string(BlockIndent, ' ') + section.title);
    for (const auto& [label, value] : section.entries) {
      std::ostringstream line;
      line << std::string(2 * BlockIndent, ' ') << std::left << std::setw(static_cast<int>(labelWidth)) << label << "  "
           << value;
      lines.push_back(line.str());
    }
  }
  return lines;
}

// ---------------------------------------------------------------------------
// ReportBox
// ---------------------------------------------------------------------------
void runner::ReportBox::setWidth(int width) { _width = width; }

void runner::ReportBox::emit(const std::string& text) {
  logInfo("|* " + fitToWidth(text, static_cast<std::size_t>(_width)) + " *|");
  _started = true;
}

// Every rule is framed by a blank line on each side, so each section gets uniform
// top/bottom padding. The padding is centralised here rather than sprinkled through the callers.
void runner::ReportBox::rule() {
  if (_lastWasRule) {
    return;
  }
  if (_started and not _lastWasBlank) { // pad the bottom of the section this rule closes
    emit("");
    _lastWasBlank = true;
  }
  logInfo("|" + std::string(static_cast<std::size_t>(_width) + 4, '*') + "|");
  _started        = true;
  _lastWasRule    = true;
  _lastWasBlank   = false;
  _blankAfterRule = true;
}

void runner::ReportBox::blank() {
  if (_lastWasBlank) {
    return;
  }
  emit("");
  _lastWasBlank   = true;
  _lastWasRule    = false;
  _blankAfterRule = false;
}

void runner::ReportBox::line(const std::string& text) {
  if (text.empty()) {
    blank();
    return;
  }
  if (_blankAfterRule) { // pad the top of the section the preceding rule opened
    emit("");
  }
  emit(text);
  _lastWasRule    = false;
  _lastWasBlank   = false;
  _blankAfterRule = false;
}
