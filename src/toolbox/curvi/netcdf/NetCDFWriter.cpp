#include "toolbox/curvi/netcdf/NetCDFWriter.h"

toolbox::curvi::NetCDFWriter::NetCDFWriter(std::string filename, int a_nz, int a_ny, int a_nx):
  m_filename(filename),
  nz(a_nz),
  ny(a_ny),
  nx(a_nx) {

  if (nz == 1) {
    dims = 2;
  } else {
    dims = 3;
  }
}

void toolbox::curvi::NetCDFWriter::open(int o_nz, int o_ny, int o_nx, bool open, std::string prefix) {
  int retval;

  if ((retval = nc_open(m_filename.c_str(), NC_WRITE, &ncid))) {
    std::cout << "Couldn't create netcdf file" << std::endl;
  }

  if (dims == 3) {
    int z_dim_id;
    if ((retval = nc_inq_dimid(ncid, "z", &z_dim_id))) {
      std::cout << "Couldn't read dimension z" << std::endl;
    }
    dim_ids.push_back(z_dim_id);
  }

  int y_dim_id;
  if ((retval = nc_inq_dimid(ncid, "y", &y_dim_id))) {
    std::cout << "Couldn't read dimension y" << std::endl;
  }
  dim_ids.push_back(y_dim_id);


  int x_dim_id;
  if ((retval = nc_inq_dimid(ncid, "x", &x_dim_id))) {
    std::cout << "Couldn't read dimension x" << std::endl;
  }
  dim_ids.push_back(x_dim_id);

  /* go to define mode */
  if ((retval = nc_redef(ncid))) {
    std::cout << "Couldn't init define mode : " << retval << std::endl;
  }

  std::string s_xt = prefix + "_xt";
  std::string s_yt = prefix + "_yt";
  std::string s_zt = prefix + "_zt";
  if ((retval = nc_def_var(ncid, s_xt.c_str(), NC_DOUBLE, dims, dim_ids.data(), &xt_var_id))) {
    std::cout << "Couldn't create variable " << s_xt << ": " << retval << std::endl;
  }

  if ((retval = nc_def_var(ncid, s_yt.c_str(), NC_DOUBLE, dims, dim_ids.data(), &yt_var_id))) {
    std::cout << "Couldn't create variable " << s_yt << ": " << retval << std::endl;
  }

  if ((retval = nc_def_var(ncid, s_zt.c_str(), NC_DOUBLE, dims, dim_ids.data(), &zt_var_id))) {
    std::cout << "Couldn't create variable " << s_zt << ": " << retval << std::endl;
  }

  if ((retval = nc_enddef(ncid))) {
    std::cout << "Couldn't end define mode : " << retval << std::endl;
  }
}

void toolbox::curvi::NetCDFWriter::init(int o_nz, int o_ny, int o_nx) {
  int retval;

  int z_var_id;
  int y_var_id;
  int x_var_id;


  if ((retval = nc_create(m_filename.c_str(), NC_CLOBBER, &ncid))) {
    std::cout << "Couldn't create netcdf file" << std::endl;
  }

  if (dims == 3) {
    int z_dim_id;
    if ((retval = nc_def_dim(ncid, "z", nz, &z_dim_id))) {
      std::cout << "Couldn't create dimension z" << std::endl;
    }
    dim_ids.push_back(z_dim_id);
  }

  int y_dim_id;
  if ((retval = nc_def_dim(ncid, "y", ny, &y_dim_id))) {
    std::cout << "Couldn't create dimension y" << std::endl;
  }
  dim_ids.push_back(y_dim_id);


  int x_dim_id;
  if ((retval = nc_def_dim(ncid, "x", nx, &x_dim_id))) {
    std::cout << "Couldn't create dimension x" << std::endl;
  }
  dim_ids.push_back(x_dim_id);

  /*define dimensions variables*/
  if (dims == 3) {
    if ((retval = nc_def_var(ncid, "z", NC_INT, 1, dim_ids.data(), &z_var_id))) {
      std::cout << "Couldn't create variable z: " << retval << std::endl;
    }
  }

  if ((retval = nc_def_var(ncid, "y", NC_INT, 1, (&y_dim_id), &y_var_id))) {
    std::cout << "Couldn't create variable y: " << retval << std::endl;
  }

  if ((retval = nc_def_var(ncid, "x", NC_INT, 1, (&x_dim_id), &x_var_id))) {
    std::cout << "Couldn't create variable x: " << retval << std::endl;
  }

  /*set attributes for dimension variables*/
  if (dims == 3) {
    int z_range[2] = {0, nz - 1};
    if ((retval = nc_put_att_int(ncid, z_var_id, "actual_range", NC_INT, 2, z_range))) {
      std::cout << "error while trying to put attribute: " << std::endl;
    }
  }

  int y_range[2] = {0, ny - 1};
  if ((retval = nc_put_att_int(ncid, y_var_id, "actual_range", NC_INT, 2, y_range))) {
    std::cout << "error while trying to put attribute: " << std::endl;
  }


  int x_range[2] = {0, nx - 1};
  if ((retval = nc_put_att_int(ncid, x_var_id, "actual_range", NC_INT, 2, x_range))) {
    std::cout << "error while trying to put attribute: " << std::endl;
  }

  /* init transformation variables */
  std::string s_xt = "xt";
  std::string s_yt = "yt";
  std::string s_zt = "zt";
  if ((retval = nc_def_var(ncid, s_xt.c_str(), NC_DOUBLE, dims, dim_ids.data(), &xt_var_id))) {
    std::cout << "Couldn't create variable xt: " << retval << std::endl;
  }

  if ((retval = nc_def_var(ncid, s_yt.c_str(), NC_DOUBLE, dims, dim_ids.data(), &yt_var_id))) {
    std::cout << "Couldn't create variable yt: " << retval << std::endl;
  }

  if ((retval = nc_def_var(ncid, s_zt.c_str(), NC_DOUBLE, dims, dim_ids.data(), &zt_var_id))) {
    std::cout << "Couldn't create variable zt: " << retval << std::endl;
  }

  /* end define mode */
  if ((retval = nc_enddef(ncid))) {
    std::cout << "Couldn't end define mode : " << retval << std::endl;
  }

  /*write data for dimensions variables*/
  std::vector<int> x;
  for (int i = 0; i < nx; i++) {
    x.push_back(i + o_nx);
  }
  if ((retval = nc_put_var_int(ncid, x_var_id, x.data()))) {
    std::cout << "Couldn't write data for variable x: " << retval << std::endl;
  }

  /*create y*/
  std::vector<int> y;
  for (int i = 0; i < ny; i++) {
    y.push_back(i + o_ny);
  }
  if ((retval = nc_put_var_int(ncid, y_var_id, y.data()))) {
    std::cout << "Couldn't write data for variable y: " << retval << std::endl;
  }

  /*create z*/
  if (dims == 3) {
    std::vector<int> z;

    for (int i = 0; i < nz; i++) {
      z.push_back(i + o_nz);
    }
    if ((retval = nc_put_var_int(ncid, z_var_id, z.data()))) {
      std::cout << "Couldn't write data for variable z: " << retval << std::endl;
    }
  }
}

int toolbox::curvi::NetCDFWriter::addVariable(std::string var_name) {
  int retval;
  /*go to define mode*/
  if ((retval = nc_redef(ncid))) {
    std::cout << "Couldn't end define mode : " << retval << std::endl;
  }
  int var_id;
  if ((retval = nc_def_var(ncid, var_name.c_str(), NC_DOUBLE, dims, dim_ids.data(), &var_id))) {
    std::cout << "Couldn't create variable xt: " << retval << std::endl;
  }
  /*end define mode*/
  if ((retval = nc_redef(ncid))) {
    std::cout << "Couldn't end define mode : " << retval << std::endl;
  }
  return var_id;
}

void toolbox::curvi::NetCDFWriter::
  write(const size_t* start, const size_t* count, const std::vector<double>& var, int var_id) {
  idx4 idx(3, nz, ny, nx);
  int  retval;
  if ((retval = nc_put_vara_double(ncid, var_id, start, count, var.data()))) {
    std::cout << "Couldn't write data for additional variable: " << retval << std::endl;
  }
}

void toolbox::curvi::NetCDFWriter::write(const size_t* start, const size_t* count, const std::vector<double>& var) {
  int retval;

  idx4 idx(3, nz, ny, nx);
  if ((retval = nc_put_vara_double(ncid, xt_var_id, start, count, var.data() + idx(Coordinate::X, 0, 0, 0)))) {
    std::cout << "Couldn't write data for variable xt: " << retval << std::endl;
  }

  if ((retval = nc_put_vara_double(ncid, yt_var_id, start, count, var.data() + idx(Coordinate::Y, 0, 0, 0)))) {
    std::cout << "Couldn't write data for variable yt: " << retval << std::endl;
  }

  if ((retval = nc_put_vara_double(ncid, zt_var_id, start, count, var.data() + idx(Coordinate::Z, 0, 0, 0)))) {
    std::cout << "Couldn't write data for variable zt: " << retval << std::endl;
  }
}

void toolbox::curvi::NetCDFWriter::close() {
  int retval;
  if ((retval = nc_close(ncid))) {
    std::cout << "Couldn't close file" << std::endl;
  }
}