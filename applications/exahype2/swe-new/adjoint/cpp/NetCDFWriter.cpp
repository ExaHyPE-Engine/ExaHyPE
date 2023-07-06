#include "NetCDFWriter.hpp"
applications::exahype2::swe::adjoint::NetCDFWriter::NetCDFWriter(const char *output_file_name,
                    const char *bathymetry_file_path,
                    unsigned int i_nX, unsigned int i_nY,
                    double cell_size,
                    double *b,
                    double end_time,
                    double start_time,
                    unsigned int flush):
                    l_nX_(i_nX),
                    l_nY_(i_nY)
{
	//create a netCDF-file, an existing file will be replaced
	//check if the netCDF-file creation constructor succeeded.
	if (nc_create(output_file_name, NC_NETCDF4, &file_id_) != NC_NOERR)
	{
        std::cout << "Output NetCDF file could not be created!" << std::endl;
		return;
	}

	//dimensions
	int dimension_time, dimension_x, dimension_y;
	nc_def_dim(file_id_, "time", NC_UNLIMITED, &dimension_time);
	nc_def_dim(file_id_, "x", i_nX, &dimension_x);
	nc_def_dim(file_id_, "y", i_nY, &dimension_y);

	int varid_x, varid_y;

	nc_def_var(file_id_, "time", NC_DOUBLE, 1, &dimension_time, &time_id_);
	nc_put_attr_text(time_id_, "long_name", "Time");
	nc_put_attr_text(time_id_, "units", "seconds since simulation start"); 

	nc_def_var(file_id_, "x", NC_DOUBLE, 1, &dimension_x, &varid_x);
	nc_def_var(file_id_, "y", NC_DOUBLE, 1, &dimension_y, &varid_y);

	int dimensions[] = {dimension_time, dimension_y, dimension_x};
	nc_def_var(file_id_, "h", NC_DOUBLE, 3, dimensions, &varid_h_);
	nc_def_var(file_id_, "hu", NC_DOUBLE, 3, dimensions, &varid_hu_);
	nc_def_var(file_id_, "hv", NC_DOUBLE, 3, dimensions, &varid_hv_);
	nc_def_var(file_id_, "b", NC_DOUBLE, 2, &dimensions[1], &varid_b_);

	nc_put_attr_text(NC_GLOBAL, "Conventions", "CF-1.5");
	nc_put_attr_text(NC_GLOBAL, "title", "Computed Adjoint Solution");
	nc_put_attr_text(NC_GLOBAL, "history", "ExaHyPE SWE Application");
	nc_put_attr_text(NC_GLOBAL, "institution", "Technische Universitaet Muenchen, School of Computation, Information and Technology, Chair of Scientific Computing");
	nc_put_attr_text(NC_GLOBAL, "source", bathymetry_file_path);
	nc_put_attr_text(NC_GLOBAL, "references", "exahype.org");

	nc_put_att_double(file_id_, NC_GLOBAL, "end_time", NC_DOUBLE, 1, &end_time);
    nc_put_att_double(file_id_, NC_GLOBAL, "start_time", NC_DOUBLE, 1, &start_time);

    //setup grid size
	double s_dx = cell_size;
	double gridPosition =  0.5 * s_dx;
	for (size_t i = 0; i < l_nX_; i++)
	{
		nc_put_var1_double(file_id_, varid_x, &i, &gridPosition);
		gridPosition += s_dx;
	}

	double s_dy = cell_size;
	gridPosition =  0.5 * s_dy;
	for (size_t j = 0; j < l_nY_; j++)
	{
		nc_put_var1_double(file_id_, varid_y, &j, &gridPosition);
		gridPosition += s_dy;
	}

    size_t start[] = {0, 0};
    size_t count[] = {l_nY_, l_nX_};
    if(nc_put_vara_double(file_id_, varid_b_, start, count, b) != NC_NOERR)
    {
        std::cout << "Bathymetry could not be written to netCDF output file!" << std::endl;
        return;
    }

}

applications::exahype2::swe::adjoint::NetCDFWriter::~NetCDFWriter()
{
    nc_close(file_id_);
}

void applications::exahype2::swe::adjoint::NetCDFWriter::writeTimeStep(double *h, double *hu, double *hv, double t, size_t checkpoint)
{
    size_t start[] = {checkpoint, 0, 0};
    size_t count[] = {1, l_nY_, l_nX_};
    if(nc_put_vara_double(file_id_, varid_h_, start, count, h) != NC_NOERR)
    {
        std::cout << "h could not be written to output for timestep: " << t << "!" << std::endl;
        return;
    }
    if(nc_put_vara_double(file_id_, varid_hu_, start, count, hu) != NC_NOERR)
    {
        std::cout << "hu could not be written to output for timestep: " << t << "!" << std::endl;
        return;
    } 
    if(nc_put_vara_double(file_id_, varid_hv_, start, count, hv) != NC_NOERR)
    {
        std::cout << "hv could not be written to output for timestep: " << t << "!" << std::endl;
        return;
    } 
    nc_put_var1_double(file_id_, time_id_, &checkpoint, &t);

	if(flush > 0 && checkpoint % flush == 0)
		nc_sync(file_id_);
}