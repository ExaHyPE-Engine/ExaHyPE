#ifndef _applications_exahype2_swe_parser_TopologyParser_H_
#define _applications_exahype2_swe_parser_TopologyParser_H_
#include "../Constants.h"
#include "NetCDFReader.hpp"
#include "NetCDFHelper.hpp"

namespace applications::exahype2::swe::parser {
  class TopologyParser;
}



class applications::exahype2::swe::parser::TopologyParser {
  private:
    double* bathymetry_ = nullptr;
    double* displacement_ = nullptr;

    size_t x_bathymetry_ = 0;               /* length of bathymetry in x-dimension */
    size_t y_bathymetry_ = 0;               /* length of bathymetry in y-dimension */
    size_t x_displacement_ = 0;             /* length of displacement in x-dimension */
    size_t y_displacement_ = 0;             /* length of displacement in y-dimension */
    double l_nX_ = -1;                      /* length of simulation in x-dimension */
    double l_nY_ = -1;                      /* length of simulation in y-dimension */
    double max_x_bathymetry_ = -1;          /* highest x-coordinate in bathymetry */
    double max_y_bathymetry_ = -1;          /* highest y-coordinate in bathymetry */
    double min_x_bathymetry_ = 1;           /* lowest x-coordinate in bathymetry */
    double min_y_bathymetry_ = 1;           /* lowest y-coordinate in bathymetry */
    double max_x_displacement_ = -1;        /* highest x-coordinate in displacement */
    double max_y_displacement_ = -1;        /* highest y-coordinate in displacement */
    double min_x_displacement_ = 1;         /* lowest x-coordinate in displacement */
    double min_y_displacement_ = 1;         /* lowest y-coordinate in displacement */
    double uq_displacement_origin_x = -1;   /* origin for displacment in x-dimension for uq */
    double uq_displacement_origin_y = -1;   /* origin for displacment in y-dimension for uq */

  public:
  
    TopologyParser(const char *bathymetry_file_path, const char *displacement_file_path, double i_uq_displacement_origin_x=INFINITY, double i_uq_displacement_origin_y=INFINITY);
    ~TopologyParser();

    void parse_bathymetry_file(const char *bathymetry_file_path);
    double sample_bathymetry(double x, double y);
    
    void parse_displacement_file(const char *displacement_file_path);
    double sample_displacement(double x, double y);
};


#endif
