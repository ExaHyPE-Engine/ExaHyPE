// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/Coordinate.h"
#include "toolbox/curvi/Index.h"

#include <iostream>
#include <netcdf.h>
#include <string>
#include <vector>

namespace toolbox {
  namespace curvi {
    class NetCDFWriter {
    public:
      NetCDFWriter(std::string filename, int a_nz, int a_ny, int a_nx);

      void open(int o_nz = 0, int o_ny = 0, int o_nx = 0, bool open = true, std::string prefix = "");

      void init(int o_nz = 0, int o_ny = 0, int o_nx = 0);

      int addVariable(std::string var_name);

      void write(const size_t* start, const size_t* count, const std::vector<double>& var, int var_id);
      void write(const size_t* start, const size_t* count, const std::vector<double>& var);

      void close();

    private:
      std::string m_filename;

      int              nx, ny, nz;
      int              ncid;
      int              zt_var_id;
      int              yt_var_id;
      int              xt_var_id;
      int              dims;
      std::vector<int> dim_ids;
    };
  } // namespace curvi
} // namespace toolbox