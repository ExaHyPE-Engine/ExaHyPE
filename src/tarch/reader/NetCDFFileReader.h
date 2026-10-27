// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#ifdef PEANO_WITH_NETCDF

namespace tarch::reader {
  class NetCDFFileReader;
} // namespace tarch::reader

class tarch::reader::NetCDFFileReader final {
public:
  int open(const std::string& filename);
  int close(int ncid);
  std::size_t getDimension(int ncid, const char* key);
  int readVariable(int ncid, const char* key, double* values);
};

#endif
