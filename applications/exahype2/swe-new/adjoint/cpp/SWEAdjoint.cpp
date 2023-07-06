#define ADJOINT
#include "./DimensionalSplitting.hpp"
#include "./NetCDFWriter.hpp"
#include "../../parser/TopologyParser.h"
#include <iostream>

namespace applications::exahype2::swe::adjoint
{
    class SWE_Adjoint
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
            void inline set_ghost_layer()
            {
                // set ghost layer in 0th row and (l_nY_ + 1)th row
                #pragma omp parallel for schedule(static)
                for(int i = 0; i < l_nX_ + 2; i++)
                {
                    h_[i * (l_nY_ + 2)] = h_[i * (l_nY_ + 2) + 1];
                    hu_[i * (l_nY_ + 2)] = hu_[i * (l_nY_ + 2) + 1];
                    hv_[i * (l_nY_ + 2)] = hv_[i * (l_nY_ + 2) + 1];

                    h_[i * (l_nY_+ 2) + (l_nY_ + 1)] = h_[i * (l_nY_+ 2) + l_nY_];
                    hu_[i * (l_nY_+ 2) + (l_nY_ + 1)] = hu_[i * (l_nY_+ 2) + l_nY_];
                    hv_[i * (l_nY_+ 2) + (l_nY_ + 1)] = hv_[i * (l_nY_+ 2) + l_nY_];
                }

                // set ghost layer in 0th column and (l_nX_ + 1)th column
                #pragma omp parallel for schedule(static)
                for(int j = 0; j < l_nY_ + 2; j++)
                {
                    h_[j] = h_[l_nY_ + 2 + j];
                    hu_[j] = hu_[l_nY_ + 2 + j];
                    hv_[j] = hv_[l_nY_ + 2 + j];

                    h_[(l_nX_ + 1) * (l_nY_ + 2) + j] = h_[l_nX_ * (l_nY_ + 2) + j];
                    hu_[(l_nX_ + 1) * (l_nY_ + 2) + j] = hu_[l_nX_ * (l_nY_ + 2) + j];
                    hv_[(l_nX_ + 1) * (l_nY_ + 2) + j] = hv_[l_nX_ * (l_nY_ + 2) + j];
                }
            }
            
            /**
             * @brief Define square area of interest
             * 
             * @param x x coordinate 
             * @param y y coordinate
             * @return true iff (x,y) in area of interest
             * @return false otherwise
             */
            bool inline hump_criterion_square(double x, double y)
            {
                double x_cdf = applications::exahype2::swe::parser::NetCDFHelper::transformIndexSimulationToCDFRange(x, l_nX_, min_x_bathymetry_, max_x_bathymetry_);
                double y_cdf = applications::exahype2::swe::parser::NetCDFHelper::transformIndexSimulationToCDFRange(y, l_nY_, min_y_bathymetry_, max_y_bathymetry_);
                double x_min = std::min(area_of_interest_coord_0_, area_of_interest_coord_1_);
                double x_max = std::max(area_of_interest_coord_0_, area_of_interest_coord_1_);
                double y_min = std::min(area_of_interest_coord_2_, area_of_interest_coord_3_);
                double y_max = std::max(area_of_interest_coord_2_, area_of_interest_coord_3_);
                return x_min < x_cdf && x_max > x_cdf && y_min < y_cdf && y_max > y_cdf;
            }

            /**
             * @brief Define circular area of interest
             * 
             * @param x x coordinate
             * @param y y coordinate
             * @return true iff (x,y) in area of interest
             * @return false otherwise
             */
            bool inline hump_criterion_circle(double x, double y)
            {
                double x_cdf = applications::exahype2::swe::parser::NetCDFHelper::transformIndexSimulationToCDFRange(x, l_nX_, min_x_bathymetry_, max_x_bathymetry_);
                double y_cdf = applications::exahype2::swe::parser::NetCDFHelper::transformIndexSimulationToCDFRange(y, l_nY_, min_y_bathymetry_, max_y_bathymetry_);
                double x_center = area_of_interest_coord_0_;
                double y_center = area_of_interest_coord_1_;
                double radius = area_of_interest_coord_2_;
                double distance_x = x_cdf - x_center;
                double distance_y = y_cdf - y_center; 
                return sqrt((distance_x * distance_x) + (distance_y * distance_y)) < radius;
            }

            void inline run_simulation()
            {
                auto dimensional_splitting = Dimensional_Splitting(l_nX_, l_nY_, cell_size_, cell_size_, h_, hu_, hv_, b_);
                double t = 0.0;
                size_t checkpoint = 0;
                // TODO write to file
                checkpoint++;
                while(t < end_time_)
                {
                    std::cout << "Simulating time: " << t << std::endl;
                    t += dimensional_splitting.compute_numerical_fluxes();
                    set_ghost_layer();
                    if(checkpoints_ == nullptr || checkpoints_[checkpoint] <= t)
                    {
                        netcdf_writer_->writeTimeStep(h_, hu_, hv_, t, checkpoint);
                        checkpoint++;
                    }
                }
            }

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
            SWE_Adjoint(
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
                ):
                width_x_(width_x),
                width_y_(width_y),
                cell_size_(cell_size),
                end_time_(end_time),
                area_of_interest_coord_0_(area_of_interest_coord_0),
                area_of_interest_coord_1_(area_of_interest_coord_1),
                area_of_interest_coord_2_(area_of_interest_coord_2),
                area_of_interest_coord_3_(area_of_interest_coord_3),
                start_time_(start_time),
                number_of_checkpoints_(checkpoints),
                topology_parser_(new applications::exahype2::swe::parser::TopologyParser(bathymetry_file_path, width_x, width_y, nc_key_x, nc_key_y, nc_key_bathymetry))
            {
                // calculate number of cells for each dimension
                l_nX_ = floor(width_x_ / cell_size_);
                l_nY_ = floor(width_y_ / cell_size_);

                // create arrays to hold values of simulation
                h_ = new double[(l_nX_ + 2) * (l_nY_ + 2)];
                hu_ = new double[(l_nX_ + 2) * (l_nY_ + 2)](); // initialize to 0
                hv_ = new double[(l_nX_ + 2) * (l_nY_ + 2)](); // initialize to 0
                b_ = new double[(l_nX_ + 2) * (l_nY_ + 2)];

                // pick the correct hump criterion
                auto hump_criterion = area_of_interest_coord_3_ == INFINITY ? &applications::exahype2::swe::adjoint::SWE_Adjoint::hump_criterion_circle : &applications::exahype2::swe::adjoint::SWE_Adjoint::hump_criterion_square;

                // initialize bathymetry column wise
                // initialize water height using bathymetry (and hump criterion in area of interest)
                #pragma omp parallel for collapse(2) schedule(static)
                for(int j = 1; j < l_nY_ + 1; j++)
                {
                    for(int i = 1; i < l_nX_ + 1; i++)
                    {
                        double x = (i - 1) + 0.5 * cell_size_;
                        double y = (j - 1) + 0.5 * cell_size_;
                        b_[i * (l_nY_ + 2) + j] = topology_parser_->sample_bathymetry(x, y);
                        h_[i * (l_nY_ + 2) + j] = std::max((this->*hump_criterion)(x,y) * 1.0 - b_[i * (l_nY_ + 2) + j], 0.0);
                    }
                }

                // delete topology parser, no longer needed
                delete topology_parser_;

                // set ghost layers for bathymetry
                // for 0th column and (l_nX_ + 1)th column
                #pragma omp parallel for schedule(static)
                for(int j = 0; j < l_nY_ + 2; j++)
                {
                    b_[j] = b_[l_nY_ + 2 + j];
                    b_[(l_nX_ + 1) * (l_nY_ + 2) + j] = b_[l_nX_ * (l_nY_ + 2) + j];
                }

                // set ghost layer for 0th row and (l_nY_ + 1)th row
                #pragma omp parallel for schedule(static)
                for(int i = 0; i < l_nX_ + 2; i++)
                {
                    b_[i * (l_nY_ + 2)] = b_[i * (l_nY_ + 2) + 1];
                    b_[i * (l_nY_ + 2) + (l_nY_ + 1)] = b_[i * (l_nY_ + 2) + l_nY_];
                }

                // set ghost layer for all other variables
                set_ghost_layer();

                // calculate checkpoints, if not every timestep results in output
                if(number_of_checkpoints_ > 0)
                {
                    checkpoints_[number_of_checkpoints_ + 1];
                    double time_width = end_time_ / number_of_checkpoints_;
                    for(int i = 0; i <= number_of_checkpoints_; i++)
                        checkpoints_[i] =  time_width * i;
                }
                
                netcdf_writer_ = new applications::exahype2::swe::adjoint::NetCDFWriter(output_file_name, bathymetry_file_path, l_nX_, l_nY_, cell_size_, b_, end_time, start_time_, flush);

                std::cout << "Starting simulation with the following parameters:" << std::endl
                            << "width: [" << width_x_ << ", " << width_y_ << "]" << std::endl
                            << "dimension: [" << l_nX_ << ", " << l_nY_ << "]" << std::endl
                            << "cell_size: " << cell_size_ << std::endl
                            << "end_time: " << end_time_ << std::endl
                            << "hump_criterion: " << (hump_criterion == &applications::exahype2::swe::adjoint::SWE_Adjoint::hump_criterion_circle ? "circle" : "square") << std::endl
                            << "output_file_name: " << output_file_name << std::endl
                            << "checkpoints: " << number_of_checkpoints_ << std::endl;
            }

            ~SWE_Adjoint()
            {
                delete h_;
                delete hu_;
                delete hv_;
                delete b_;
                delete netcdf_writer_;
            }
    };
};

int main(int argc, char **argv)
{
    /*
    output_file_name="test.nc", 
    bathymetry_file_path="../netCDFInputs/gebco_2023_n65.0_s10.0_w-180.0_e-120.0.nc", 
    width=[10,10], cell_size=0.1, 
    area_of_interest=[[-124.19083, 41.74111e0], [1e0, float('inf')]], 
    end_time = 0.1,
    nc_key_x='lon',
    nc_key_y='lat',
    nc_key_bathymetry='elevation')
    */
    auto swe_adjoint = applications::exahype2::swe::adjoint::SWE_Adjoint(
        "test.nc",
        "../../netCDFInputs/gebco_2023_n65.0_s10.0_w-180.0_e-120.0.nc",
        10.0, 10.0,
        0.1,
        0.1,
        -124.19083, 41.75111, 1.0, INFINITY,
        0.0, 0, 0,"lon", "lat", "elevation"
    );
    return 0;
}