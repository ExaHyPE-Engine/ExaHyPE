#ifndef applications_exahype2_swe_adjoint_SWEAdjoint_h_
#define applications_exahype2_swe_adjoint_SWEAdjoint_h_
#include "./DimensionalSplitting.hpp"
#include "./NetCDFWriter.hpp"
#include "../../parser/TopologyParser.h"
#include <iostream>

namespace applications::exahype2::swe::adjoint
{
    class SWEAdjoint
    {
        private:

            size_t l_nX_ = 0; /* number of cells in x-dimension */
            size_t l_nY_ = 0; /* number of cells in y-dimension */

            double width_x_ = 0; /* width of coordinate space in x-dimension */
            double width_y_ = 0; /* width of coordinate space in y-dimension*/
            double cell_size_ = 0; /* discretization interval for both dimensions */
            double end_time_ = 0; /* end time of simulation and of temporal region of interest */
            double start_time_ = 0; /* start time of temporal region of interest */

            size_t number_of_checkpoints_ = 0; /* number of checkpoints to output at. 0 means infinity in this case */
            double *checkpoints_ = nullptr; /* array of checkpoints if number of checkpoints is not 0 */

            applications::exahype2::swe::parser::TopologyParser *topology_parser_; /* parser for bathymetry */
            applications::exahype2::swe::adjoint::NetCDFWriter *netcdf_writer_; /* output writer */

            double *h_ = nullptr; /* array containing water height */
            double *hu_ = nullptr; /* array for containing momentum in x-dimension */
            double *hv_ = nullptr; /* array for containing momentum in y-dimenion */
            double *b_ = nullptr; /* array for containing bathymetry */

            double max_x_bathymetry_ = -1;          /* highest x-coordinate in bathymetry */
            double max_y_bathymetry_ = -1;          /* highest y-coordinate in bathymetry */
            double min_x_bathymetry_ = 1;           /* lowest x-coordinate in bathymetry */
            double min_y_bathymetry_ = 1;           /* lowest y-coordinate in bathymetry */

            double area_of_interest_coord_0_ = -1; /* Either x value for corner of square or x coordinate of center of circle */
            double area_of_interest_coord_1_ = -1; /* Either x value for corner of square or y coordinate of center of circle */
            double area_of_interest_coord_2_ = -1; /* Either y value for corner of square or radius of circle */
            double area_of_interest_coord_3_ = -1; /* Either y value for corner of square or INFINITY */

            /**
             * @brief Set the ghost layer of all simulation arrays
             * 
             * This function sets the ghost layers of h, hu and hv.
             */
            void inline set_ghost_layer();

            /**
             * @brief Define square area of interest
             * 
             * @param x x coordinate 
             * @param y y coordinate
             * @return true iff (x,y) in area of interest
             * @return false otherwise
             */
            bool inline hump_criterion_square(double x, double y);

            /**
            * @brief Define circular area of interest
            * 
            * @param x x coordinate
            * @param y y coordinate
            * @return true iff (x,y) in area of interest
            * @return false otherwise
            */
            bool inline hump_criterion_circle(double x, double y);

            void inline run_simulation();
        
        public:

            /**
             * @brief Construct a new swe adjoint object
             * 
             * @param output_file_name 
             * @param bathymetry_file_path 
             * @param width_x width of simulation plane in x-dimension  
             * @param width_y width of simulation plane in y-dimension
             * @param cell_size size of cell used in simulation
             * @param end_time end time of simulation and also end time for temporal region of interest
             * @param area_of_interest_coord_0 spacial region of interest: If square, this is one x value of a corner of the square. If circle, this is the x coordinate of the center
             * @param area_of_interest_coord_1 spacial region of interest: If square, this is another x value of a corner of the square. If circle, this is the y coordinate of the center
             * @param area_of_interest_coord_2 spacial region of interest: If square, this is one y value of a corner of the square. If circle, this is the radius
             * @param area_of_interest_coord_3 spacial region of interest: If square, this is another y value of a corner of the square. If circle, this is INFINITY
             * @param start_time start time of temporal region of interest
             * @param checkpoints number of checkpoints at which output is to be written. If 0, then every timestep output is written
             * @param nc_key_x key of x values in netCDF file
             * @param nc_key_y key of y values in netCDF file
             * @param nc_key_bathymetry key of bathymetry values in netCDF file
             */
            SWEAdjoint(
                const char *output_file_name,
                const char *bathymetry_file_path, 
                double width_x, double width_y, 
                double cell_size,
                double end_time, 
                double area_of_interest_coord_0, double area_of_interest_coord_1, double area_of_interest_coord_2, double area_of_interest_coord_3=INFINITY, 
                double start_time=0, 
                size_t checkpoints=0, 
                size_t flush=0,
                const char* nc_key_x="x", const char* nc_key_y="y", const char* nc_key_bathymetry="z"
                );
            
            ~SWEAdjoint();
    };
}
#endif