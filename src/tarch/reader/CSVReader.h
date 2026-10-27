// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch::reader {
  class CSVReader;
} // namespace tarch::reader

class tarch::reader::CSVReader final {
public:
  explicit CSVReader(const std::string& filepath, int skipFirstNLines = 0);

  [[nodiscard]] const std::string& getRow(int rowIndex) const;
  [[nodiscard]] int getNumberOfRows() const;

  template <class T>
  [[nodiscard]] static INLINE_METHOD T extractFundamentalTypeFromRow(
    char               separator,
    int                valueIndex,
    const std::string& row
  );

private:
  std::vector<std::string> _rows{};
};

#include "tarch/reader/CSVReader.cpph"
