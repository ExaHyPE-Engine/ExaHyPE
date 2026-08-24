// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace runner {
  std::size_t              displayWidth(const std::string& text);
  std::string              truncateToDisplayWidth(const std::string& text, const std::size_t columns);
  std::string              padToWidth(const std::string& text, const std::size_t columns);
  std::string              fitToWidth(const std::string& text, const std::size_t columns);
  std::vector<std::string> wrapToDisplayWidth(const std::string& text, const std::size_t columns);
} // namespace runner
