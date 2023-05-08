#include "TopologyParser.h"
#include <iostream>
#include <math.h>  

applications::exahype2::swe::parser::TopologyParser::TopologyParser(const char *bathymetry_file_path, const char *displacement_file_path){

  l_nX = DomainSize.begin()[0];
  l_nY = DomainSize.begin()[1];
  this->parse_bathymetry_file(bathymetry_file_path);
  this->parse_displacement_file(displacement_file_path);

}

applications::exahype2::swe::parser::TopologyParser::~TopologyParser(){
  delete this->bathymetry; 
  delete this->displacement;
}

void applications::exahype2::swe::parser::TopologyParser::parse_bathymetry_file(const char *bathymetry_file_path){
  int ncid;
  // Open bathymetry netCDF file
  if ((ncid = NetCDFReader::openCdf(bathymetry_file_path)) == -1)
  {
    std::cout << "Scenario could not be initialized: Bathymetry file could not be opened at: "<< bathymetry_file_path << std::endl;
    return;
  }

  // Read x-dimension of bathymetry
  if ((xBathy = NetCDFReader::getDimension(ncid, "x")) == 0)
  {
    std::cout << "Scenario could not be initialized: Dimension of the x-Bathymetry could not be read." << std::endl;
    return;
  }

  // Read y-dimension of bathymetry
  if ((yBathy = NetCDFReader::getDimension(ncid, "y")) == 0)
  {
    std::cout << "Scenario could not be initialized: Dimension of the y-Bathymetry could not be read." << std::endl;
    return;
  }

  bathymetry = new double[xBathy*yBathy];
  // Read bathymetry
  if (NetCDFReader::readVariable2D(ncid, "z", bathymetry, xBathy, yBathy) == -1)
  {
    std::cout << "Scenario could not be initialized: Bathymetry could not be read." << std::endl;
    return;
  }

  // Get max and min x
  {
    double tmp[xBathy]{0};
    if (NetCDFReader::readVariable1D(ncid, "x", tmp) == -1)
    {
        std::cout << "Scenario could not be initialized: Bathymetry x indices could not be read." << std::endl;
        return;
    }
    min_x_bathymetry_ = tmp[0];
    max_x_bathymetry_ = tmp[xBathy - 1];
  }

  // Get max and min y
  {
    double tmp[yBathy]{0};
    if (NetCDFReader::readVariable1D(ncid, "y", tmp) == -1)
    {
        std::cout << "Scenario could not be initialized: Bathymetry y indices could not be read." << std::endl;
        return;
    }
    min_y_bathymetry_ = tmp[0];
    max_y_bathymetry_ = tmp[yBathy - 1];
  }

  // Close bathymetry netCDF file
  if (NetCDFReader::closeCdf(ncid) == -1)
  {
    std::cout << "Scenario could not be initialized: Bathymetry file could not be closed." << std::endl;
    return;
  }
}

void applications::exahype2::swe::parser::TopologyParser::parse_displacement_file(const char *displacement_file_path){
  // Read displacement netCDF file
  int ncid;
  if ((ncid = NetCDFReader::openCdf(displacement_file_path)) == -1)
  {
    std::cout << "Scenario could not be initialized: Displacement file could not be opened at: "<< displacement_file_path << std::endl;
    return;
  }

  // Read x-dimension of displacement
  if ((xDis = NetCDFReader::getDimension(ncid, "x")) == 0)
  {
    std::cout << "Scenario could not be initialized: Dimension of the x-displacement could not be read." << std::endl;
    return;
  }

  // Read y-dimension of displacement
  if ((yDis = NetCDFReader::getDimension(ncid, "y")) == 0)
  {
    std::cout << "Scenario could not be initialized: Dimension of the y-displacement could not be read." << std::endl;
    return;
  }

  // Get max and min x
  {
    double tmp[xDis]{0};
    if (NetCDFReader::readVariable1D(ncid, "x", tmp) == -1)
    {
        std::cout << "Scenario could not be initialized: Displacement x indices could not be read." << std::endl;
        return;
    }
    min_x_displacement_ = tmp[0];
    max_x_displacement_ = tmp[xDis - 1];
  }

  // Get max and min y
  {
    double tmp[yDis]{0};
    if (NetCDFReader::readVariable1D(ncid, "y", tmp) == -1)
    {
        std::cout << "Scenario could not be initialized: Displacement y indices could not be read." << std::endl;
        return;
    }
    min_y_displacement_ = tmp[0];
    max_y_displacement_ = tmp[yDis - 1];
  }

  displacement = new double[xDis*yDis];

  // Read displacement
  if (NetCDFReader::readVariable2D(ncid, "z", displacement, xDis, yDis) == -1)
  {
    std::cout << "Scenario could not be initialized: Displacement could not be read." << std::endl;
    return;
  }

  // Close displacement netCDF file
  if (NetCDFReader::closeCdf(ncid) == -1)
  {
    std::cout << "Scenario could not be initialized: Displacement file could not be closed." << std::endl;
    return;
  }
}

double applications::exahype2::swe::parser::TopologyParser::transformIndexSimulationToCDFRange(double i_z, double i_nZ, double i_min_z_cdf, double i_max_z_cdf){
  double min_z = 0.0;
  double max_z = i_nZ;
  double m = ((i_max_z_cdf - i_min_z_cdf) / (max_z - min_z));
  double b = i_min_z_cdf - m * min_z; // correction for y-intercept in index interpolation

  double ret = m * i_z + b;

  return ret;
}
  
int applications::exahype2::swe::parser::TopologyParser::transformIndexCDFRangeToArray(double i_x, double i_y, double i_min_x, double i_min_y, double i_max_x, double i_max_y, size_t i_xDim, size_t i_yDim){
  int x_index;
  int y_index;
  {
    size_t maxIndex = (i_xDim - 1);
    // Mesh width of data in x dimension
    double dx = ((i_min_x - i_max_x) / i_xDim);
    // f(0) = b
    double b = i_min_x / dx;
    double m = maxIndex / (i_max_x - i_min_x);
    int x_index = floor(m * i_x + b);
  }
  {
    size_t maxIndex = (i_yDim - 1);
    // Mesh width of data in y dimension
    double dy = ((i_min_y - i_max_y) / i_yDim);
    // f(0) = b
    double b = i_min_y / dy;
    double m = maxIndex / (i_max_y - i_min_y);
    int y_index = floor(m * i_y + b);
  }
  return y_index * i_xDim + x_index;
}

double applications::exahype2::swe::parser::TopologyParser::sample_bathymetry(double x, double y){
  auto x_cdf = transformIndexSimulationToCDFRange(x, l_nX, min_x_bathymetry_, max_x_bathymetry_);
  auto y_cdf = transformIndexSimulationToCDFRange(y, l_nY, min_y_bathymetry_, max_y_bathymetry_);
  // No defined bathymetry
  if (x_cdf > max_x_bathymetry_ || x_cdf < min_x_bathymetry_ || y_cdf > max_y_bathymetry_ || y_cdf < min_y_bathymetry_)
      return 20.f;
  return bathymetry[transformIndexCDFRangeToArray(x, y, min_x_bathymetry_, min_y_bathymetry_, max_x_bathymetry_, max_y_bathymetry_, xBathy, yBathy)];
}

double applications::exahype2::swe::parser::TopologyParser::sample_displacement(double x, double y){
  auto x_cdf = transformIndexSimulationToCDFRange(x, l_nX, min_x_bathymetry_, max_x_bathymetry_);
  auto y_cdf = transformIndexSimulationToCDFRange(y, l_nY, min_y_bathymetry_, max_y_bathymetry_);
  if (x_cdf <= max_x_displacement_ && x_cdf >= min_x_displacement_ && y_cdf <= max_y_displacement_ && y_cdf >= min_y_displacement_)
    return displacement[transformIndexCDFRangeToArray(x, y, min_x_displacement_, min_y_displacement_, max_x_displacement_, max_y_displacement_, xDis, yDis)];
  return 0.0;
}






