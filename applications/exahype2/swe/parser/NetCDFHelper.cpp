#include "NetCDFHelper.hpp"
double applications::exahype2::swe::parser::NetCDFHelper::transformIndexSimulationToCDFRange(double i_z, double i_nZ, double i_min_z_cdf, double i_max_z_cdf){
  // TODO with an offset defined, min_z should probably be adjusted
  double min_z = 0.0;
  double max_z = i_nZ;
  double m = ((i_max_z_cdf - i_min_z_cdf) / (max_z - min_z));
  double b = i_min_z_cdf - m * min_z; // correction for y-intercept in index interpolation

  double ret = m * i_z + b;

  return ret;
}
  
int applications::exahype2::swe::parser::NetCDFHelper::transformIndexCDFRangeToArray(double i_x, double i_y, double i_min_x, double i_min_y, double i_max_x, double i_max_y, size_t i_xDim, size_t i_yDim){
  int x_index;
  int y_index;
  {
    size_t maxIndex = (i_xDim - 1);
    // Mesh width of data in x dimension
    double dx = ((i_min_x - i_max_x) / i_xDim);
    // f(0) = b
    double b = i_min_x / dx;
    double m = maxIndex / (i_max_x - i_min_x);
    x_index = floor(m * i_x + b);
  }
  {
    size_t maxIndex = (i_yDim - 1);
    // Mesh width of data in y dimension
    double dy = ((i_min_y - i_max_y) / i_yDim);
    // f(0) = b
    double b = i_min_y / dy;
    double m = maxIndex / (i_max_y - i_min_y);
    y_index = floor(m * i_y + b);
  }
  return y_index * i_xDim + x_index;
}