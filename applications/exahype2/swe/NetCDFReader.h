// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

namespace applications::exahype2::swe {
  class NetCDFReader;
} // namespace applications::exahype2::swe

class applications::exahype2::swe::NetCDFReader {
public:
  /**
   * @brief Opens a netCDF file, returns ncid.
   *
   * @param filename path of file to be opened.
   * @return id of netCDF file for further operations.
   */
  int open(const std::string& filename);

  /**
   * @brief Closes netCDF file
   *
   * @param ncid Pointer to location where returned netCDF ID is to be stored.
   * @return 0 if successful, -1 otherwise.
   */
  int close(int ncid);

  /**
   * @brief Gets the dimension of a variable.
   *
   * It is assumed that the dimension inquired is greater 0.
   *
   * @param ncid Pointer to location where returned netCDF ID is to be stored.
   * @param key name of the variable.
   * @return dimension or 0 if an error occurred.
   */
  std::size_t getDimension(int ncid, const char* key);

  /**
   * @brief Reads the global attribute name of type int and returns an array
   * containing all the associated values.
   *
   * @param ncid id of the netCDF file.
   * @param name name of the attribute.
   * @return int* array with values stored in the attribute or nullptr if
   * unsuccessful.
   */
  int* getGlobalAttInt(int ncid, const char* name);

  /**
   * @brief Reads the global attribute name of type float and returns an array
   * containing all the associated values.
   *
   * @param ncid id of the netCDF file.
   * @param name name of the attribute.
   * @return float* array with the values stored in the attribute or nullptr if
   * unsuccessful.
   */
  float* getGlobalAttFloat(int ncid, const char* name);

  /**
   * @brief Reads the global attribute name of type double and returns an array
   * containing all the associated values.
   *
   * @param ncid id of the netCDF file.
   * @param name name of the attribute.
   * @return double* array with the values stored in the attribute or nullptr if
   * unsuccessful.
   */
  double* getGlobalAttDouble(int ncid, const char* name);

  /**
   * @brief Reads the global attribute name of type text and returns an array
   * containing all the associated values.
   *
   * @param ncid id of the netCDF file.
   * @param name name of the attribute.
   * @return char* array with the values stored in the attribute or nullptr if
   * unsuccessful.
   */
  char* getGlobalAttText(int ncid, const char* name);

  /**
   * @brief Reads contents of a 1D double variable from netCDF file.
   *
   * @param ncid Pointer to location where returned netCDF ID is to be stored.
   * @param key variable name for which data is loaded.
   * @param variable Array where data is stored.
   * @return 0 if successful, -1 otherwise.
   */
  int readVariable1D(int ncid, const char* key, double* variable);

  /**
   * @brief Reads contents of a 2D double variable from netCDF file into 2D
   * array.
   *
   * @param ncid Pointer to location where returned netCDF ID is to be stored.
   * @param key variable name for which data is loaded.
   * @param variable Array where data is stored.
   * @param rows count of rows of the array.
   * @param cols count of columns of the array.
   * @return 0 if successful, -1 otherwise.
   */
  int readVariable2D(
    int         ncid,
    const char* key,
    double**    variable,
    std::size_t rows,
    std::size_t cols
  );

  /**
   * @brief Reads contents of a 2D double variable from netCDF file into 1D
   * array.
   *
   * @param ncid Pointer to location where returned netCDF ID is to be stored.
   * @param key variable name for which data is loaded.
   * @param variable Array where data is stored.
   * @return 0 if successful, -1 otherwise.
   */
  int readVariable2D(int ncid, const char* key, double* variable);

  /**
   * @brief Reads contents of a 2D double variable from netCDF file into 2D
   * array starting at a specified first-dimension value.
   *
   * @param ncid Pointer to location where returned netCDF ID is to be stored.
   * @param key variable name for which data is loaded.
   * @param variable Array where data is stored.
   * @param rows count of rows of the array.
   * @param cols count of columns of the array.
   * @param start value of the first dimension from which to start reading.
   * @return 0 if successful, -1 otherwise.
   */
  int readVariable2DHyperslab(
    int         ncid,
    const char* key,
    double**    variable,
    std::size_t rows,
    std::size_t cols,
    std::size_t start
  );

  /**
   * @brief Reads contents of a 3D double variable from netCDF file into 1D
   * array starting at a specified first-dimension value.
   *
   * @param ncid Pointer to location where returned netCDF ID is to be stored.
   * @param key variable name for which data is loaded.
   * @param variable Array where data is stored.
   * @param rows count of rows of the array.
   * @param cols count of columns of the array.
   * @param start value of the first dimension from which to start reading.
   * @param end value of first dimension at which to stop reading.
   * @return 0 if successful, -1 otherwise.
   */
  int readVariable3DHyperslab(
    int         ncid,
    const char* key,
    double*     variable,
    std::size_t rows,
    std::size_t cols,
    std::size_t start,
    std::size_t end
  );
};
