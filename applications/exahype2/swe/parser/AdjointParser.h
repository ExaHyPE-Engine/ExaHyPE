#ifndef _applications_exahype2_swe_parser_AdjointParser_H_
#define _applications_exahype2_swe_parser_AdjointParser_H_
#include "NetCDFReader.hpp"
#include "NetCDFHelper.hpp"

namespace applications::exahype2::swe::parser {
  class AdjointParser;
}

class applications::exahype2::swe::parser::AdjointParser {
  private:
    double* adjoint_h_ = nullptr;           /* stores time-dependent h values of adjoint solution */
    double* adjoint_hu_ = nullptr;          /* stores time-dependent hu values of adjoint solution */
    double* adjoint_hv_ = nullptr;          /* stores time-dependent hv values of adjoint solution */
    double* adjoint_b_ = nullptr;           /* stores time-independent bathymetry values of adjoint solution */
    double* adjoint_timestamps_ = nullptr;  /* stores timestamps of adjoint solution */

    size_t number_timestamps_ = 0;          /* number of timestamps */

    size_t size_x_adjoint_ = 0;             /* number of cells in x dimension of adjoint solution */
    size_t size_y_adjoint_ = 0;             /* number of cells in y dimension of adjoint solution */

    double max_x_adjoint_ = -1;             /* highest x-coordinate in adjoint */
    double max_y_adjoint_ = -1;             /* highest y-coordinate in adjoint */
    double min_x_adjoint_ = 1;              /* lowest x-coordinate in adjoint */
    double min_y_adjoint_ = 1;              /* lowest y-coordinate in adjoint */

    double t_f_ = -1;                       /* end time for adjoint solution  */
    double t_s_ = -1;                       /* start time for adjoint solution */

    size_t inner_product_start_index_ = 1;  /* start index for inner product calculation. Same for all cells in a time step */
    double t_last_call_ = -1.0;             /* timestamp of last call of max_inner_product() */

    /**
     * @brief Interpolates spatial indices to fit adjoint solution and returns adjoint solution values at these indices 
     * 
     * @param i_index index of timestamp of adjoint solution for which the inner product is to be calculated
     * @param i_x coordinate of x-dimension of simulation which is being interpolated
     * @param i_y coordinate of y-dimension of simulation which is being interpolated
     * @param i_eta free surface of current cell (h + b)
     * @param i_hu momentum in x-dimension of current cell
     * @param i_hv momentum in y-dimension of current cell
     * @param i_b bathymetry of current cell
     * @param interpolated_values array to be filled with adjoint solution values in the order (h, hu, hv, b)
     */
    void interpolate_values(size_t i_index, double i_x, double i_y, double *interpolated_values);

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
    double calculate_inner_product(size_t i_index, double i_x, double i_y, const double* Q);

    /**
     * @brief Parses given netCDF file of adjoint simulation and reads out all necessary values
     * 
     * @param adjoint_file_path path to pre-computed adjoint solution netCDF file
     * @param start_time start time for temporal area of interest. Default (=INFINITY) is to read from file.
     */
    void parse_adjoint_file(const char *adjoint_file_path, double start_time=INFINITY);

    /**
     * @brief Call this once every time step to set start index for inner product calculation
     * 
     * @param i_t current time of forward simulation
     */
    void set_start_index(double i_t);
  
  public:
    AdjointParser(const char *adjoint_file_path, double start_time=INFINITY);
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
    double max_inner_product(double i_t, double i_x, double i_y, const double* Q);
};
#endif
