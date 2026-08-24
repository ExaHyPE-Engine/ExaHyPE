// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "ReportBox.h"

#include "runner/TextLayout.h"
#include "tarch/instrumentation/Info.h"

namespace {
  constexpr std::size_t ReportWidth = 100;
} // namespace

runner::Table::Section& runner::Table::sectionFor(const std::string& title) {
  for (auto& section : _sections) {
    if (section.title == title) {
      return section;
    }
  }
  _sections.push_back(runner::Table::Section{title, {}});
  return _sections.back();
}

void runner::Table::add(const std::string& section, const std::string& label, const std::string& value) {
  sectionFor(section).entries.emplace_back(label, value);
}

std::vector<std::string> runner::Table::render() const {
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
    lines.push_back(std::string(runner::BlockIndent, ' ') + section.title);
    for (const auto& [label, value] : section.entries) {
      std::ostringstream line;
      line << std::string(2 * runner::BlockIndent, ' ') << std::left << std::setw(static_cast<int>(labelWidth)) << label
           << "  " << value;
      lines.push_back(line.str());
    }
  }
  return lines;
}

void runner::ReportBox::emit(const std::string& text) {
  logInfo("|* " + runner::fitToWidth(text, ReportWidth) + " *|");
  _started = true;
}

/**
 * Adds one blank line on each side of a section boundary. Keeping this spacing in the frame object
 * gives all reporting phases the same layout.
 */
void runner::ReportBox::rule() {
  if (_lastWasRule) {
    return;
  }
  if (_started and not _lastWasBlank) {
    emit("");
    _lastWasBlank = true;
  }
  logInfo("|" + std::string(ReportWidth + 4, '*') + "|");
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
  if (_blankAfterRule) {
    emit("");
  }
  for (const auto& wrappedLine : runner::wrapToDisplayWidth(text, ReportWidth)) {
    emit(wrappedLine);
  }
  _lastWasRule    = false;
  _lastWasBlank   = false;
  _blankAfterRule = false;
}
