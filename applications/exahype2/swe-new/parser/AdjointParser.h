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
    double* eta_ = nullptr;
    double* timestamps_ = nullptr;

    size_t number_timestamps_ = 0;

    size_t x_eta_ = 0;
    size_t y_eta_ = 0;

    double l_nX_ = -1;
    double l_nY_ = -1;

    double max_x_eta_ = -1;          /* highest x-coordinate in adjoint */
    double max_y_eta_ = -1;          /* highest y-coordinate in adjoint */
    double min_x_eta_ = 1;           /* lowest x-coordinate in adjoint */
    double min_y_eta_ = 1;           /* lowest y-coordinate in adjoint */

    double t_f_ = -1;                /* end time for adjoint solution  */
    double t_s_ = -1;                /* start time for adjoint solution */

    void interpolate_values(size_t i_index, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b, double *interpolated_values);
    double calculate_inner_product(size_t i_index, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b);
    void parse_adjoint_file(const char *adjoint_file_path);
  
  public:
    AdjointParser(const char *adjoint_file_path);
    ~AdjointParser();

    double max_inner_product(double i_t, double i_x, double i_y, double i_eta, double i_hu, double i_hv, double i_b);
}
#endif
