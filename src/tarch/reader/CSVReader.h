// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::reader {
  class CSVReader;
} // namespace tarch::reader

class tarch::reader::CSVReader {
private:
  std::vector<std::string> csvLines;
  int                      numberOfRows = -1;

public:
  /**
   * @brief Creates a reader for CSV files.
   *
   * @param filepath path to CSV file.
   * @param skipFirstNLines int indicating how many initial lines to skip. Defaults to 0.
   */
  CSVReader(const std::string& filepath, const int skipFirstNLines = 0);
  ~CSVReader();

  /**
   * @brief Returns the line of the CSV file indicated by the row parameter.
   *
   * @param row index of row to return. Starts at 0.
   *
   * @return the CSV line as a const std::string.
   */
  std::string getRow(const int row) const;

  /**
   * @brief Returns the number of rows contained in the CSV file. Available rows are [0, numberOfRows).
   *
   * @return number of rows in CSV file as int.
   */
  int getNumberOfRows() const;

  /**
   * @brief Helper function to extract fundamental/primitive values from a CSV line.
   *
   * @param separator the separator used in the CSV file.
   * @param valueIndex the index of the value to return. Starts at 0.
   * @param row the row to extract a value from
   *
   * @return extracted value of type T.
   */
  template <class T>
  static T extractFundamentalTypeFromRow(const char separator, const int valueIndex, const std::string row) {
    T extractedValue;
    if (std::is_fundamental_v<T> == false) {
      throw std::runtime_error("Template type is not fundamental.");
    }
    // Length of string till last found match
    size_t lastMatch = 0;
    // Length of string till next found match
    size_t nextMatch = 0;
    // Count posts of the fence
    int numberOfMatches = 0;

    // Loop over separator occurrences in row and get substring of value, if possible
    while ((nextMatch = row.find(separator, lastMatch)) != std::string::npos) {
      ++numberOfMatches;
      if (numberOfMatches > valueIndex) {
        const std::string value = row.substr(lastMatch, nextMatch - lastMatch);
        std::stringstream(value) >> extractedValue;
        return extractedValue;
      }

      // If value not yet between lastMatch and nextMatch, increment lastMatch
      lastMatch = nextMatch + 1;
    }

    std::stringstream ss;
    ss << "No match found for valueIndex " << valueIndex << " and separator \"" << separator << "\" in row \"" << row
       << "\".";
    throw std::runtime_error(ss.str());
  }
};
