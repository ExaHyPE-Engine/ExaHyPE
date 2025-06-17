// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "CSVReader.h"

#include <format>
#include <fstream>
#include <sys/stat.h>

tarch::reader::CSVReader::CSVReader(const std::string& filepath, const int skipFirstNLines) {
  std::ifstream csvFile(filepath.c_str());
  if (!csvFile.is_open()) {
    std::stringstream ss;
    ss << "Could not open CSV file at \"" << filepath << "\".";
    throw std::runtime_error(ss.str());
  }
  {
    std::string line;
    int         lineNumber = 0;
    while (getline(csvFile, line)) {
      ++lineNumber;
      if (lineNumber > skipFirstNLines)
        csvLines.push_back(line);
    }
  }
  numberOfRows = csvLines.size();
}

const std::string tarch::reader::CSVReader::getRow(const int rowIndex) {
  if (rowIndex >= numberOfRows) {
    std::stringstream ss;
    ss << "Illegal row index " << rowIndex << " for maximum number of rows " << numberOfRows << ".";
    throw std::runtime_error(ss.str());
  }

  return csvLines.at(rowIndex);
}

tarch::reader::CSVReader::~CSVReader() {
  numberOfRows = -1;
  csvLines.clear();
  csvLines.shrink_to_fit();
}
