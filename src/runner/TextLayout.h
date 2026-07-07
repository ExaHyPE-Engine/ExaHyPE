// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <cstddef>
#include <string>

namespace runner {
  // Width means terminal columns, counted as UTF-8 code points - correct for the one-column glyphs
  // the statistics box uses (ASCII, λ, ·, block elements).
  std::size_t displayWidth(const std::string& text);
  std::string truncateToDisplayWidth(const std::string& text, std::size_t columns);
  std::string padToWidth(const std::string& text, std::size_t columns);
  std::string fitToWidth(const std::string& text, std::size_t columns);
} // namespace runner
