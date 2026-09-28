// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFFileReader.h"

#ifdef PEANO_WITH_NETCDF

#include <netcdf.h>
#include <sys/stat.h>

int
tarch::reader::NetCDFFileReader::open(const std::string& filename) {
  // Check that file exists (nc_open does not do that apparently)
  struct stat buffer;
  if (stat(filename.c_str(), &buffer) != 0) {
    return -1;
  }

  int ncid = -1;
  if (nc_open(filename.c_str(), NC_NOWRITE, &ncid) != NC_NOERR) {
    return -1;
  }

  return ncid;
}

int
tarch::reader::NetCDFFileReader::close(int ncid) {
  if (nc_close(ncid) != NC_NOERR) {
    return -1;
  }
  return 0;
}

std::size_t
tarch::reader::NetCDFFileReader::getDimension(int ncid, const char* key) {
  int         dimidkey = -1;
  std::size_t dim      = 0;

  if (nc_inq_dimid(ncid, key, &dimidkey) != NC_NOERR) {
    return 0;
  }
  if (nc_inq_dimlen(ncid, dimidkey, &dim) != NC_NOERR) {
    return 0;
  }

  return dim;
}

int
tarch::reader::NetCDFFileReader::readVariable1D(int ncid, const char* key, double* variable) {
  int varidkey = -1;

  if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR) {
    return -1;
  }

  if (nc_get_var_double(ncid, varidkey, variable) != NC_NOERR) {
    return -1;
  }

  return 0;
}

int
tarch::reader::NetCDFFileReader::readVariable2D(int ncid, const char* key, double* variable) {
  int varidkey = -1;

  if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR) {
    return -1;
  }

  if (nc_get_var_double(ncid, varidkey, variable) != NC_NOERR) {
    return -1;
  }

  return 0;
}

int
tarch::reader::NetCDFFileReader::readVariable3D(int ncid, const char* key, double* variable) {
  int varidkey = -1;

  if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR) {
    return -1;
  }

  if (nc_get_var_double(ncid, varidkey, variable) != NC_NOERR) {
    return -1;
  }

  return 0;
}

#endif
