// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TextLayout.h"

std::size_t runner::displayWidth(const std::string& text) {
  std::size_t columns = 0;
  for (const unsigned char character : text) {
    if ((character & 0xC0) != 0x80) {
      ++columns;
    }
  }
  return columns;
}

std::string runner::truncateToDisplayWidth(const std::string& text, std::size_t columns) {
  std::size_t seen = 0;
  for (std::size_t i = 0; i < text.size(); ++i) {
    if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80) {
      if (seen == columns) {
        return text.substr(0, i);
      }
      ++seen;
    }
  }
  return text;
}

std::string runner::padToWidth(const std::string& text, std::size_t columns) {
  const auto width = displayWidth(text);
  return width < columns ? text + std::string(columns - width, ' ') : text;
}

std::string runner::fitToWidth(const std::string& text, std::size_t columns) {
  return padToWidth(truncateToDisplayWidth(text, columns), columns);
}
