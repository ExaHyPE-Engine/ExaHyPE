#ifndef _applications_exahype2_swe_parser_TopologyParser_H_
#define _applications_exahype2_swe_parser_TopologyParser_H_
#include "../Constants.h"
#include "NetCDFReader.hpp"

namespace applications::exahype2::swe::parser {
  class TopologyParser;
}



class applications::exahype2::swe::parser::TopologyParser {
  private:
    double* bathymetry = nullptr;
    double* displacement = nullptr;

    size_t xBathy = 0;               /* length of bathymetry in x-dimension */
    size_t yBathy = 0;               /* length of bathymetry in y-dimension */
    size_t xDis = 0;                 /* length of displacement in x-dimension */
    size_t yDis = 0;                 /* length of displacement in y-dimension */
    double l_nX = -1;                /* length of simulation in x-dimension */
    double l_nY = -1;                /* length of simulation in y-dimension */
    double max_x_bathymetry_ = -1;   /* highest x-coordinate in bathymetry */
    double max_y_bathymetry_ = -1;   /* highest y-coordinate in bathymetry */
    double min_x_bathymetry_ = 1;    /* lowest x-coordinate in bathymetry */
    double min_y_bathymetry_ = 1;    /* lowest y-coordinate in bathymetry */
    double max_x_displacement_ = -1; /* highest x-coordinate in displacement */
    double max_y_displacement_ = -1; /* highest y-coordinate in displacement */
    double min_x_displacement_ = 1;  /* lowest x-coordinate in displacement */
    double min_y_displacement_ = 1;  /* lowest y-coordinate in displacement */

    /**
     * @brief 
     * 
     * @param i_dz mesh width of simulation space in z direction
     * @param i_z coordinate of dimension z in simulation space
     * @param i_nZ size of simulation space in z direction
     * @param i_min_z_cdf min index in cdf space
     * @param i_max_z_cdf max index in cdf space
     * @return index in netCDF space [i_min_z_cdf, i_max_z_cdf]
     */
    double transformIndexSimulationToCDFRange(double i_z, double i_nZ, double i_min_z_cdf, double i_max_z_cdf);

      /**
     * @brief Transform index from cdf value to input data
     * 
     * @param i_x x-coordinate in the netCDF mesh
     * @param i_y y-coordinate in the netCDF mesh
     * @param i_min_x minimum x-coordinate in netCDF mesh
     * @param i_min_y minimum y-coordinate in netCDF mesh
     * @param i_max_x maximum x-coordinate in netCDF mesh
     * @param i_max_y maximum y-coordinate in netCDF mesh
     * @param i_xDim length of x-dimension in netCDF file
     * @param i_yDim length of y-dimension in netCDF file
     * @return index in [0, ]
     */
     int transformIndexCDFRangeToArray(double i_x, double i_y, double i_min_x, double i_min_y, double i_max_x, double i_max_y, size_t i_xDim, size_t i_yDim);

  public:
  
    TopologyParser(const char *bathymetry_file_path, const char *displacement_file_path);
    ~TopologyParser();

    void parse_bathymetry_file(const char *bathymetry_file_path);
    double sample_bathymetry(double x, double y);
    
    void parse_displacement_file(const char *displacement_file_path);
    double sample_displacement(double x, double y);
};


#endif
