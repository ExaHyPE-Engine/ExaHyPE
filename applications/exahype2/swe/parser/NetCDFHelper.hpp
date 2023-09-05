#ifndef _applications_exahype2_swe_parser_NetCDFHelper_H_
#define _applications_exahype2_swe_parser_NetCDFHelper_H_
#include <math.h>  
namespace applications::exahype2::swe::parser::NetCDFHelper
{
    /**
     * @brief Transforms coordinate from simulation plane to netCDF plane.
     * 
     * @param i_z coordinate of dimension z in simulation space
     * @param i_nZ size of simulation space in z direction
     * @param i_min_z_cdf min index in cdf space
     * @param i_max_z_cdf max index in cdf space
     * @return index in netCDF space [i_min_z_cdf, i_max_z_cdf]
     */
    double transformIndexSimulationToCDFRange(double i_z, double i_nZ, double i_min_z_cdf, double i_max_z_cdf);

    /**
     * @brief Transforms coordinate pair from netCDF plane to 1D row-wise array index.
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
}
#endif