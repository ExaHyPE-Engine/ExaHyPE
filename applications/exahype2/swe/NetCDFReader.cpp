// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NetCDFReader.h"

#include <netcdf.h>
#include <sys/stat.h>

int applications::exahype2::swe::NetCDFReader::open(const std::string& filename
) {
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


int applications::exahype2::swe::NetCDFReader::close(int ncid) {
  if (nc_close(ncid) != NC_NOERR) {
    return -1;
  }
  return 0;
}


std::size_t applications::exahype2::swe::NetCDFReader::getDimension(
  int         ncid,
  const char* key
) {
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


int* applications::exahype2::swe::NetCDFReader::getGlobalAttInt(
  int         ncid,
  const char* name
) {
  std::size_t len = 0;
  if (nc_inq_attlen(ncid, NC_GLOBAL, name, &len) != NC_NOERR || len == 0) {
    return nullptr;
  }

  int* tmp = new int[len]{};
  if (nc_get_att_int(ncid, NC_GLOBAL, name, tmp) != NC_NOERR) {
    return nullptr;
  }

  return tmp;
}


float* applications::exahype2::swe::NetCDFReader::getGlobalAttFloat(
  int         ncid,
  const char* name
) {
  std::size_t len = 0;
  if (nc_inq_attlen(ncid, NC_GLOBAL, name, &len) != NC_NOERR || len == 0) {
    return nullptr;
  }

  float* tmp = new float[len]{};
  if (nc_get_att_float(ncid, NC_GLOBAL, name, tmp) != NC_NOERR) {
    return nullptr;
  }

  return tmp;
}


double* applications::exahype2::swe::NetCDFReader::getGlobalAttDouble(
  int         ncid,
  const char* name
) {
  std::size_t len = 0;
  if (nc_inq_attlen(ncid, NC_GLOBAL, name, &len) != NC_NOERR || len == 0) {
    return nullptr;
  }

  double* tmp = new double[len]{};
  if (nc_get_att_double(ncid, NC_GLOBAL, name, tmp) != NC_NOERR) {
    return nullptr;
  }

  return tmp;
}


char* applications::exahype2::swe::NetCDFReader::getGlobalAttText(
  int         ncid,
  const char* name
) {
  std::size_t len;
  if (nc_inq_attlen(ncid, NC_GLOBAL, name, &len) != NC_NOERR || len == 0) {
    return nullptr;
  }

  char* tmp = new char[len]{};
  if (nc_get_att_text(ncid, NC_GLOBAL, name, tmp) != NC_NOERR) {
    return nullptr;
  }

  return tmp;
}

int applications::exahype2::swe::NetCDFReader::
  readVariable1D(int ncid, const char* key, double* variable) {
  int varidkey = -1;

  if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR) {
    return -1;
  }

  if (nc_get_var_double(ncid, varidkey, variable) != NC_NOERR) {
    return -1;
  }

  return 0;
}


int applications::exahype2::swe::NetCDFReader::readVariable2D(
  int         ncid,
  const char* key,
  double**    variable,
  std::size_t rows,
  std::size_t cols
) {
  int varidkey = -1;

  if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR) {
    return -1;
  }

  double* tmp = new double[rows * cols]{};
  if (nc_get_var_double(ncid, varidkey, tmp) != NC_NOERR) {
    return -1;
  }

  std::size_t j = 0;
  for (std::size_t i = 0; i < rows * cols; i++) {
    if (i % rows == 0 && i > 0) {
      j++;
    }
    variable[i % rows][j] = tmp[i];
  }

  delete[] tmp;

  return 0;
}


int applications::exahype2::swe::NetCDFReader::
  readVariable2D(int ncid, const char* key, double* variable) {
  int varidkey = -1;

  if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR) {
    return -1;
  }

  if (nc_get_var_double(ncid, varidkey, variable) != NC_NOERR) {
    return -1;
  }

  return 0;
}


int applications::exahype2::swe::NetCDFReader::readVariable2DHyperslab(
  int         ncid,
  const char* key,
  double**    variable,
  std::size_t rows,
  std::size_t cols,
  std::size_t start
) {
  int varidkey = -1;

  if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR) {
    return -1;
  }

  std::size_t startp[]{start, 0, 0};
  std::size_t countp[]{1, cols, rows};
  double*     tmp = new double[cols * rows];

  if (nc_get_vara_double(ncid, varidkey, startp, countp, tmp) != NC_NOERR) {
    return -1;
  }

  std::size_t j = 0;
  for (std::size_t i = 0; i < rows * cols; i++) {
    if (i % rows == 0 && i > 0) {
      j++;
    }
    variable[i % rows][j] = tmp[i];
  }

  delete[] tmp;

  return 0;
}


int applications::exahype2::swe::NetCDFReader::readVariable3DHyperslab(
  int         ncid,
  const char* key,
  double*     variable,
  std::size_t rows,
  std::size_t cols,
  std::size_t start,
  std::size_t end
) {
  int varidkey = -1;
  if (nc_inq_varid(ncid, key, &varidkey) != NC_NOERR) {
    return -1;
  }

  std::size_t startp[]{start, 0, 0};
  std::size_t countp[]{end, cols, rows};

  if (nc_get_vara_double(ncid, varidkey, startp, countp, variable) != NC_NOERR) {
    return -1;
  }

  return 0;
}
