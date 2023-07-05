#define ADJOINT
#include "DimensionalSplitting.hpp"
#include "../parser/TopologyParser.h"
int main(int argc, char **argv)
{

}

namespace applications::exahype2::swe::adjoint
{
    class SWE_Adjoint
    {
        private:

            size_t l_nX_ = 0;
            size_t l_nY_ = 0;

            double width_x_ = 0;
            double width_y_ = 0;
            double cell_size_ = 0;
            double end_time_ = 0;
            double start_time_ = 0;

            size_t number_of_checkpoints_ = 0;
            double *checkpoints_ = nullptr;

            applications::exahype2::swe::parser::TopologyParser *topology_parser_;

            double *h_ = nullptr;
            double *hu_ = nullptr;
            double *hv_ = nullptr;
            double *b_ = nullptr;

            double max_x_bathymetry_ = -1;          /* highest x-coordinate in bathymetry */
            double max_y_bathymetry_ = -1;          /* highest y-coordinate in bathymetry */
            double min_x_bathymetry_ = 1;           /* lowest x-coordinate in bathymetry */
            double min_y_bathymetry_ = 1;           /* lowest y-coordinate in bathymetry */

            double area_of_interest_coord_0_ = -1;
            double area_of_interest_coord_1_ = -1;
            double area_of_interest_coord_2_ = -1;
            double area_of_interest_coord_3_ = -1;

            void inline set_ghost_layer()
            {
                #pragma omp parallel for schedule(static)
                for(int i = 0; i < l_nX_ + 2; i++)
                {
                    h_[i] = h_[l_nX_ + 2 + i];
                    hu_[i] = hu_[l_nX_ + 2 + i];
                    hv_[i] = hv_[l_nX_ + 2 + i];

                    h_[(l_nY_ + 1) * (l_nX_+ 2) + i] = h_[l_nY_ * (l_nX_+ 2) + i];
                    hu_[(l_nY_ + 1) * (l_nX_+ 2) + i] = hu_[l_nY_  * (l_nX_+ 2) + i];
                    hv_[(l_nY_ + 1) * (l_nX_+ 2) + i] = hv_[l_nY_ * (l_nX_+ 2) + i];
                }

                #pragma omp parallel for schedule(static)
                for(int j = 0; j < l_nY_ + 2; j++)
                {
                    h_[j * (l_nX_ + 2)] = h_[j * (l_nX_ + 2) + 1];
                    hu_[j * (l_nX_ + 2)] = hu_[j * (l_nX_ + 2) + 1];
                    hv_[j * (l_nX_ + 2)] = hv_[j * (l_nX_ + 2) + 1];

                    h_[j * (l_nX_ + 2) + (l_nX_ + 1)] = h_[j * (l_nX_ + 2) + l_nX_];
                    hu_[j * (l_nX_ + 2) + (l_nX_ + 1)] = hu_[j * (l_nX_ + 2) + l_nX_];
                    hv_[j * (l_nX_ + 2) + (l_nX_ + 1)] = hv_[j * (l_nX_ + 2) + l_nX_];
                }
            }

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

        public:
            SWE_Adjoint(
                const char *output_file_name,
                const char *bathymetry_file_path, 
                double width_x, double width_y, 
                double cell_size,
                double end_time, 
                double area_of_interest_coord_0, double area_of_interest_coord_1, double area_of_interest_coord_2, double area_of_interest_coord_3=INFINITY, 
                double start_time=0, 
                size_t checkpoints=0, 
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
                l_nX_ = floor(width_x_ / cell_size_);
                l_nY_ = floor(width_y_ / cell_size_);

                h_ = new double[(l_nX_ + 2) * (l_nY_ + 2)];
                hu_ = new double[(l_nX_ + 2) * (l_nY_ + 2)](); // initialize to 0
                hv_ = new double[(l_nX_ + 2) * (l_nY_ + 2)](); // initialize to 0
                b_ = new double[(l_nX_ + 2) * (l_nY_ + 2)];

                auto hump_criterion = area_of_interest_coord_3_ == INFINITY ? hump_criterion_circle : hump_criterion_square;

                #pragma omp parallel for collapse(2) schedule(static)
                for(int i = 1; i < l_nX_ + 1; i++)
                {
                    for(int j = 1; j < l_nY_ + 1; j++)
                    {
                        double x = i * 0.5 * cell_size_;
                        double y = j * 0.5 * cell_size_;
                        b_[j * (l_nX_ + 2) + i] = topology_parser_->sample_bathymetry(x, y);
                        h_[j * (l_nX_ + 2) + i] = std::max((this->*hump_criterion)(x,y) * 1.0 - b_[j * (l_nX_ + 2) + i], 0.0);
                    }
                }

                #pragma omp parallel for schedule(static)
                for(int i = 0; i < l_nX_ + 2; i++)
                {
                    b_[i] = b_[l_nX_ + 2 + i];
                    b_[(l_nY_ + 1) * (l_nX_ + 2) + i];
                }

                #pragma omp parallel for schedule(static)
                for(int j = 0; j < l_nY_ + 2; j++)
                {
                    b_[j * (l_nX_ + 2)] = b_[j * (l_nX_ + 2) + 1];
                    b_[j * (l_nX_ + 2) + (l_nX_ + 1)] = b_[j * (l_nX_ + 2) + l_nX_];
                }

                if(number_of_checkpoints_ > 0)
                {
                    checkpoints_[number_of_checkpoints_ + 1];
                    double time_width = end_time_ / number_of_checkpoints_;
                    for(int i = 0; i <= number_of_checkpoints_; i++)
                        checkpoints_[i] =  time_width * i;
                }
            }
    };
};