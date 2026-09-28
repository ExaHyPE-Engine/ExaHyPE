// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_NETCDF

namespace tarch::reader {
  class NetCDFFileReader;
} // namespace tarch::reader

class tarch::reader::NetCDFFileReader {
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
   * @brief Reads contents of a 1D double variable from netCDF file.
   *
   * @param ncid Pointer to location where returned netCDF ID is to be stored.
   * @param key variable name for which data is loaded.
   * @param variable Array where data is stored.
   * @return 0 if successful, -1 otherwise.
   */
  int readVariable1D(int ncid, const char* key, double* variable);

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
   * @brief Reads contents of a 3D double variable from netCDF file into 1D
   * array.
   *
   * @param ncid Pointer to location where returned netCDF ID is to be stored.
   * @param key variable name for which data is loaded.
   * @param variable Array where data is stored.
   * @return 0 if successful, -1 otherwise.
   */
  int readVariable3D(int ncid, const char* key, double* variable);
};

#endif
