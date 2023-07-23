#ifndef _applications_exahype2_swe_parser_TopologyParser_H_
#define _applications_exahype2_swe_parser_TopologyParser_H_
#ifndef ADJOINT
#include "../Constants.h"
#endif
#include "NetCDFReader.hpp"
#include "NetCDFHelper.hpp"

namespace applications::exahype2::swe::parser {
  class TopologyParser;
}



class applications::exahype2::swe::parser::TopologyParser {
  private:
    double* bathymetry_ = nullptr;
    double* displacement_ = nullptr;

    size_t x_bathymetry_ = 0;                     /* length of bathymetry in x-dimension */
    size_t y_bathymetry_ = 0;                     /* length of bathymetry in y-dimension */
    size_t x_displacement_ = 0;                   /* length of displacement in x-dimension */
    size_t y_displacement_ = 0;                   /* length of displacement in y-dimension */
    double l_nX_ = -1;                            /* length of simulation in x-dimension */
    double l_nY_ = -1;                            /* length of simulation in y-dimension */
    double max_x_bathymetry_ = -1;                /* highest x-coordinate in bathymetry */
    double max_y_bathymetry_ = -1;                /* highest y-coordinate in bathymetry */
    double min_x_bathymetry_ = 1;                 /* lowest x-coordinate in bathymetry */
    double min_y_bathymetry_ = 1;                 /* lowest y-coordinate in bathymetry */
    double max_x_displacement_ = -1;              /* highest x-coordinate in displacement */
    double max_y_displacement_ = -1;              /* highest y-coordinate in displacement */
    double min_x_displacement_ = 1;               /* lowest x-coordinate in displacement */
    double min_y_displacement_ = 1;               /* lowest y-coordinate in displacement */
    double uq_displacement_origin_x = INFINITY;   /* origin for displacment in x-dimension for uq */
    double uq_displacement_origin_y = INFINITY;   /* origin for displacment in y-dimension for uq */
    
    const char *nc_bathymetry_x_key_;
    const char *nc_bathymetry_y_key_;
    const char *nc_bathymetry_bathymetry_key_;

    const char *nc_displacement_x_key_;
    const char *nc_displacement_y_key_;
    const char *nc_displacement_displacement_key_;

  public:
  
    TopologyParser(
      const char *bathymetry_file_path, const char *displacement_file_path, 
      const char *nc_bathymetry_x_key="x", const char *nc_bathymetry_y_key="y", const char *nc_bathymetry_bathymetry_key="z",
      const char *nc_displacement_x_key="x", const char *nc_displacement_y_key="y", const char *nc_displacement_displacement_key="z",
      double i_uq_displacement_origin_x=INFINITY, double i_uq_displacement_origin_y=INFINITY
    );
    
    TopologyParser(
      const char *bathymetry_file_path, 
      double width_x, double width_y,
      const char *nc_bathymetry_x_key="x", const char *nc_bathymetry_y_key="y", const char *nc_bathymetry_bathymetry_key="z"
    );
    ~TopologyParser();

    void parse_bathymetry_file(const char *bathymetry_file_path);
    double sample_bathymetry(double x, double y);

    double get_max_x_bathymetry(){ return max_x_bathymetry_; }
    double get_min_x_bathymetry(){ return min_x_bathymetry_; }
    double get_max_y_bathymetry(){ return max_y_bathymetry_; }
    double get_min_y_bathymetry(){ return min_y_bathymetry_; }

    
    void parse_displacement_file(const char *displacement_file_path);
    double sample_displacement(double x, double y);
};


#endif
