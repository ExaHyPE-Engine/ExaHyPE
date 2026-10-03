// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CSVReader.h"

/**
 * Reads every line of the CSV file at filepath after the first skipFirstNLines lines.
 */
tarch::reader::CSVReader::CSVReader(const std::string& filepath, const int skipFirstNLines) {
  std::ifstream csvFile(filepath);
  if (not csvFile.is_open()) {
    throw std::runtime_error("Could not open CSV file at \"" + filepath + "\".");
  }

  std::string line;
  int         lineNumber = 0;
  while (std::getline(csvFile, line)) {
    lineNumber++;
    if (lineNumber > skipFirstNLines) {
      _rows.push_back(line);
    }
  }
}

/**
 * Returns the row with the index rowIndex, counted from zero.
 */
const std::string&
tarch::reader::CSVReader::getRow(const int rowIndex) const {
  if (rowIndex < 0 or rowIndex >= getNumberOfRows()) {
    throw std::runtime_error(
      "Illegal row index " + std::to_string(rowIndex) + " for " + std::to_string(getNumberOfRows()) + " rows."
    );
  }
  return _rows[static_cast<std::size_t>(rowIndex)];
}

int
tarch::reader::CSVReader::getNumberOfRows() const {
  return static_cast<int>(_rows.size());
}
