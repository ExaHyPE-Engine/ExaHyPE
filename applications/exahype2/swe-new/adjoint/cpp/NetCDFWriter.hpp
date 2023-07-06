#ifndef applications_exahype2_swe_adjoint_NetCDFWriter_h_
#define applications_exahype2_swe_adjoint_NetCDFWriter_h_
#include <netcdf.h>
#include <string.h>
#include <iostream>
namespace applications::exahype2::swe::adjoint{
    class NetCDFWriter{
        private:
            int file_id_;
            int time_id_;
            int varid_h_;
            int varid_hu_;
            int varid_hv_;
            int varid_b_;
            unsigned int l_nX_;
            unsigned int l_nY_;
            unsigned int flush;

            void nc_put_attr_text(int varid, const char* name, const char* value)
            {
                nc_put_att_text(file_id_, varid, name, strlen(value), value);
            }
        public:
            /**
             * Create a netCdf-file
             * Any existing file will be replaced.
             *
             * @param output_file_name Filename of the output file
             * @param bathymetry_file_path Path to the bathymetry file used
             * @param i_nX number of cells in x-dimension
             * @param i_nY number of cells in y-dimension
             * @param b bathymetry array
             * @param end_time end time of simulation
             * @param start_time start time for temporal region of interest
             * @param flush flush data to disk after |flush| write operations
             */
            NetCDFWriter(const char *output_file_name,
                        const char *bathymetry_file_path,
                        unsigned int i_nX, unsigned int i_nY,
                        double cell_size,
                        double *b,
                        double end_time,
                        double start_time = 0,
                        unsigned int flush = 0);
            ~NetCDFWriter();

            // writes the unknowns at a given time step to the netCDF-file.
            void writeTimeStep(double *h,
                                double *hu,
                                double *hv,
                                double t,
                                size_t checkpoint);
    };
};  
#endif