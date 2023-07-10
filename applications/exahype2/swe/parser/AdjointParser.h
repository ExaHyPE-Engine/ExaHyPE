#ifndef _applications_exahype2_swe_parser_AdjointParser_H_
#define _applications_exahype2_swe_parser_AdjointParser_H_
#include "../Constants.h"
#include "NetCDFReader.hpp"
#include "NetCDFHelper.hpp"

namespace applications::exahype2::swe::parser {
  class AdjointParser;
}

class applications::exahype2::swe::parser::AdjointParser {
  private:
    double* adjoint_h_ = nullptr;
    double* adjoint_hu_ = nullptr;
    double* adjoint_hv_ = nullptr;
    double* adjoint_b_ = nullptr;
    double* adjoint_timestamps_ = nullptr;

    size_t number_timestamps_ = 0;

    size_t size_x_adjoint_ = 0;
    size_t size_y_adjoint_ = 0;

    double l_nX_ = -1;
    double l_nY_ = -1;

    double max_x_adjoint_ = -1;          /* highest x-coordinate in adjoint */
    double max_y_adjoint_ = -1;          /* highest y-coordinate in adjoint */
    double min_x_adjoint_ = 1;           /* lowest x-coordinate in adjoint */
    double min_y_adjoint_ = 1;           /* lowest y-coordinate in adjoint */

    double t_f_ = -1;                /* end time for adjoint solution  */
    double t_s_ = -1;                /* start time for adjoint solution */

    /**
     * @brief 
     * 
     * @param i_index 
     * @param i_x 
     * @param i_y 
     * @param i_eta 
     * @param i_hu 
     * @param i_hv 
     * @param i_b 
     * @param interpolated_values 
     */
    void interpolate_values(size_t i_index, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b, double *interpolated_values);

    /**
     * @brief Calculates the inner product of adjoint and current simulation at given timestamp index
     * 
     * @param i_index index of timestamp that poses as upper limit for current simulation time for interpolation
     * @param i_x coordinate of x-dimension of simulation for which the inner product is to be calculated
     * @param i_y coordinate of y-dimension of simulation for which the inner product is to be calculated
     * @param i_eta free surface of current cell (h + b)
     * @param i_hu momentum in x-dimension of current cell
     * @param i_hv momentum in y-dimension of current cell
     * @param i_b bathymetry of current cell
     * @return double inner product of current cell and adjoint solution for specific interpolated timestamp
     */
    double calculate_inner_product(size_t i_index, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b);

    /**
     * @brief Parses given netCDF file of adjoint simulation and reads out all necessary values
     * 
     * @param adjoint_file_path path to pre-computed adjoint solution netCDF file
     */
    void parse_adjoint_file(const char *adjoint_file_path);
  
  public:
    AdjointParser(const char *adjoint_file_path);
    ~AdjointParser();

    /**
     * @brief Calculates the maximum inner product of adjoint and current simulation
     * 
     * Use this function for guided adaptive mesh refinement as detailed in https://doi.org/10.1007/s00024-016-1412-y
     * 
     * @param i_t current simulation time
     * @param i_x coordinate of x-dimension of simulation for which the maximum inner product is to be calculated
     * @param i_y coordinate of y-dimension of simulation for which the maximum inner product is to be calculated
     * @param i_eta free surface of current cell (h + b)
     * @param i_hu momentum in x-dimension of current cell
     * @param i_hv momentum in y-dimension of current cell
     * @param i_b bathymetry in current cell
     * @return double maximum inner product of current cell and interpolated adjoint solution
     */
    double max_inner_product(double i_t, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b);
}
#endif
