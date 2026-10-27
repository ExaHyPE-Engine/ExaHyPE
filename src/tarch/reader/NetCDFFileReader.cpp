// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFileReader.h"

#ifdef PEANO_WITH_NETCDF

#include <netcdf.h>

/**
 * Returns the id of the opened netCDF file, or -1 on failure.
 */
int
tarch::reader::NetCDFFileReader::open(const std::string& filename) {
  int ncid = -1;
  if (nc_open(filename.c_str(), NC_NOWRITE, &ncid) != NC_NOERR) {
    return -1;
  }
  return ncid;
}

int
tarch::reader::NetCDFFileReader::close(const int ncid) {
  if (nc_close(ncid) != NC_NOERR) {
    return -1;
  }
  return 0;
}

/**
 * Returns the length of the dimension key, or 0 on failure.
 */
std::size_t
tarch::reader::NetCDFFileReader::getDimension(const int ncid, const char* const key) {
  int         dimensionId = -1;
  std::size_t length      = 0;
  if (nc_inq_dimid(ncid, key, &dimensionId) != NC_NOERR) {
    return 0;
  }
  if (nc_inq_dimlen(ncid, dimensionId, &length) != NC_NOERR) {
    return 0;
  }
  return length;
}

/**
 * Copies every value of the variable key into values. The caller provides room for all of them.
 * Returns 0, or -1 on failure.
 */
int
tarch::reader::NetCDFFileReader::readVariable(const int ncid, const char* const key, double* const values) {
  int variableId = -1;
  if (nc_inq_varid(ncid, key, &variableId) != NC_NOERR) {
    return -1;
  }
  if (nc_get_var_double(ncid, variableId, values) != NC_NOERR) {
    return -1;
  }
  return 0;
}

#endif
