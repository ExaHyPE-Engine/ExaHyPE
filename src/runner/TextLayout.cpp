// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TextLayout.h"

namespace {
  std::size_t wrapOffset(const std::string& prefix, const std::size_t indentationBytes) {
    const auto separator = prefix.rfind(" · ");
    if (separator != std::string::npos and separator > indentationBytes) {
      return separator;
    }

    const auto whitespace = prefix.find_last_of(' ');
    return whitespace != std::string::npos and whitespace > indentationBytes ? whitespace : prefix.size();
  }
} // namespace

/**
 * Counts UTF-8 code points as terminal columns. This is valid for the report's one-column ASCII,
 * λ, ·, and block glyphs; combining and wide characters are outside the supported input.
 */
std::size_t runner::displayWidth(const std::string& text) {
  std::size_t columns = 0;
  for (const unsigned char character : text) {
    if ((character & 0xC0) != 0x80) {
      ++columns;
    }
  }
  return columns;
}

std::string runner::truncateToDisplayWidth(const std::string& text, const std::size_t columns) {
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

std::string runner::padToWidth(const std::string& text, const std::size_t columns) {
  const auto width = runner::displayWidth(text);
  return width < columns ? text + std::string(columns - width, ' ') : text;
}

std::string runner::fitToWidth(const std::string& text, const std::size_t columns) {
  return runner::padToWidth(runner::truncateToDisplayWidth(text, columns), columns);
}

std::vector<std::string> runner::wrapToDisplayWidth(const std::string& text, const std::size_t columns) {
  if (columns == 0 or runner::displayWidth(text) <= columns) {
    return {text};
  }

  const auto firstNonSpace = text.find_first_not_of(' ');
  const auto indentation   = firstNonSpace == std::string::npos ? std::string() : text.substr(0, firstNonSpace);
  const auto continuationIndentation = runner::displayWidth(indentation) < columns ? indentation : std::string();

  std::vector<std::string> lines;
  auto                     remaining = text;
  while (runner::displayWidth(remaining) > columns) {
    const auto prefix                  = runner::truncateToDisplayWidth(remaining, columns);
    const auto currentIndentationBytes = remaining.find_first_not_of(' ');
    const auto indentationBytes =
      currentIndentationBytes == std::string::npos ? std::size_t{0} : currentIndentationBytes;
    const auto offset = wrapOffset(prefix, indentationBytes);
    lines.push_back(remaining.substr(0, offset));
    remaining.erase(0, offset);
    const auto nextNonSpace = remaining.find_first_not_of(' ');
    remaining.erase(0, nextNonSpace == std::string::npos ? remaining.size() : nextNonSpace);
    if (not remaining.empty()) {
      remaining.insert(0, continuationIndentation);
    }
  }
  if (not remaining.empty()) {
    lines.push_back(remaining);
  }
  return lines;
}
