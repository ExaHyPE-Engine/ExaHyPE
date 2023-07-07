#include "TopologyParser.h"
#include <iostream>
using namespace applications::exahype2::swe::parser::NetCDFHelper;

applications::exahype2::swe::parser::TopologyParser::TopologyParser(
  const char *bathymetry_file_path, const char *displacement_file_path, 
  double i_uq_displacement_origin_x, double i_uq_displacement_origin_y,
  const char *nc_bathymetry_x_key, const char *nc_bathymetry_y_key, const char *nc_bathymetry_bathymetry_key,
  const char *nc_displacement_x_key, const char *nc_displacement_y_key, const char *nc_displacement_displacement_key
):
uq_displacement_origin_x(i_uq_displacement_origin_x),
uq_displacement_origin_y(i_uq_displacement_origin_y),
nc_bathymetry_x_key_(nc_bathymetry_x_key),
nc_bathymetry_y_key_(nc_bathymetry_y_key),
nc_bathymetry_bathymetry_key_(nc_bathymetry_bathymetry_key),
nc_displacement_x_key_(nc_displacement_x_key),
nc_displacement_y_key_(nc_displacement_y_key),
nc_displacement_displacement_key_(nc_displacement_displacement_key)
{

  l_nX_ = applications::exahype2::swe::DomainSize(0);
  l_nY_ = applications::exahype2::swe::DomainSize(1);
  this->parse_bathymetry_file(bathymetry_file_path);
  this->parse_displacement_file(displacement_file_path);

}

applications::exahype2::swe::parser::TopologyParser::TopologyParser(
  const char* bathymetry_file_path,
  const char* displacement_file_path,
  const char* nc_bathymetry_x_key,
  const char* nc_bathymetry_y_key,
  const char* nc_bathymetry_bathymetry_key,
  const char* nc_displacement_x_key,
  const char* nc_displacement_y_key,
  const char* nc_displacement_displacement_key
):
nc_bathymetry_x_key_(nc_bathymetry_x_key),
nc_bathymetry_y_key_(nc_bathymetry_y_key),
nc_bathymetry_bathymetry_key_(nc_bathymetry_bathymetry_key),
nc_displacement_x_key_(nc_displacement_x_key),
nc_displacement_y_key_(nc_displacement_y_key),
nc_displacement_displacement_key_(nc_displacement_displacement_key)
{
  l_nX_ = applications::exahype2::swe::DomainSize(0);
  l_nY_ = applications::exahype2::swe::DomainSize(1);
  this->parse_bathymetry_file(bathymetry_file_path);
  this->parse_displacement_file(displacement_file_path);
}

applications::exahype2::swe::parser::TopologyParser::TopologyParser(
  const char *bathymetry_file_path, 
  double width_x, double width_y,
  const char *nc_bathymetry_x_key, const char *nc_bathymetry_y_key, const char *nc_bathymetry_bathymetry_key
):
l_nX_(width_x),
l_nY_(width_y),
nc_bathymetry_x_key_(nc_bathymetry_x_key),
nc_bathymetry_y_key_(nc_bathymetry_y_key),
nc_bathymetry_bathymetry_key_(nc_bathymetry_bathymetry_key)
{
  this->parse_bathymetry_file(bathymetry_file_path);
}


applications::exahype2::swe::parser::TopologyParser::~TopologyParser(){
  delete this->bathymetry_; 
  delete this->displacement_;
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
  if ((x_bathymetry_ = NetCDFReader::getDimension(ncid, nc_bathymetry_x_key_)) == 0)
  {
    std::cout << "Scenario could not be initialized: Dimension of the x-Bathymetry could not be read." << std::endl;
    return;
  }

  // Read y-dimension of bathymetry
  if ((y_bathymetry_ = NetCDFReader::getDimension(ncid, nc_bathymetry_y_key_)) == 0)
  {
    std::cout << "Scenario could not be initialized: Dimension of the y-Bathymetry could not be read." << std::endl;
    return;
  }

  bathymetry_ = new double[x_bathymetry_*y_bathymetry_];
  // Read bathymetry
  if (NetCDFReader::readVariable2D(ncid, nc_bathymetry_bathymetry_key_, bathymetry_, x_bathymetry_, y_bathymetry_) == -1)
  {
    std::cout << "Scenario could not be initialized: Bathymetry could not be read." << std::endl;
    return;
  }

  // Get max and min x
  {
    double tmp[x_bathymetry_]{0};
    if (NetCDFReader::readVariable1D(ncid, nc_bathymetry_x_key_, tmp) == -1)
    {
        std::cout << "Scenario could not be initialized: Bathymetry x indices could not be read." << std::endl;
        return;
    }
    min_x_bathymetry_ = tmp[0];
    max_x_bathymetry_ = tmp[x_bathymetry_ - 1];
  }

  // Get max and min y
  {
    double tmp[y_bathymetry_]{0};
    if (NetCDFReader::readVariable1D(ncid, nc_bathymetry_y_key_, tmp) == -1)
    {
        std::cout << "Scenario could not be initialized: Bathymetry y indices could not be read." << std::endl;
        return;
    }
    min_y_bathymetry_ = tmp[0];
    max_y_bathymetry_ = tmp[y_bathymetry_ - 1];
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
  if ((x_displacement_ = NetCDFReader::getDimension(ncid, nc_displacement_x_key_)) == 0)
  {
    std::cout << "Scenario could not be initialized: Dimension of the x-displacement could not be read." << std::endl;
    return;
  }

  // Read y-dimension of displacement
  if ((y_displacement_ = NetCDFReader::getDimension(ncid, nc_displacement_y_key_)) == 0)
  {
    std::cout << "Scenario could not be initialized: Dimension of the y-displacement could not be read." << std::endl;
    return;
  }

  // Get max and min x
  {
    double tmp[x_displacement_]{0};
    if (NetCDFReader::readVariable1D(ncid, nc_displacement_x_key_, tmp) == -1)
    {
        std::cout << "Scenario could not be initialized: Displacement x indices could not be read." << std::endl;
        return;
    }
    min_x_displacement_ = tmp[0];
    max_x_displacement_ = tmp[x_displacement_ - 1];
    // Has the origin of the displacement in x-dimension been put elsewhere than specified in the file?
    if(!isinf(uq_displacement_origin_x))
    {
      // Move boundaries
       std::cout << "Current boundaries in x-dimension: [" << min_x_displacement_ << ", " << max_x_displacement_ << "]." << std::endl;
      double difference = max_x_displacement_ - min_x_displacement_;
      min_x_displacement_ = uq_displacement_origin_x - (difference / 2.f);
      max_x_displacement_ = uq_displacement_origin_x + (difference / 2.f);
      std::cout << "Moved boundaries in x-dimension to: [" << min_x_displacement_ << ", " << max_x_displacement_ << "]." << std::endl;
    }
  }

  // Get max and min y
  {
    double tmp[y_displacement_]{0};
    if (NetCDFReader::readVariable1D(ncid, nc_displacement_y_key_, tmp) == -1)
    {
        std::cout << "Scenario could not be initialized: Displacement y indices could not be read." << std::endl;
        return;
    }
    min_y_displacement_ = tmp[0];
    max_y_displacement_ = tmp[y_displacement_ - 1];
    // Has the origin of the displacement in y-dimension been put elsewhere than specified in the file?
    if(!isinf(uq_displacement_origin_y))
    {
      // Move boundaries
      std::cout << "Current boundaries in y-dimension: [" << min_y_displacement_ << ", " << max_y_displacement_ << "]." << std::endl;
      double difference = max_y_displacement_ - min_y_displacement_;
      min_y_displacement_ = uq_displacement_origin_y - (difference / 2.f);
      max_y_displacement_ = uq_displacement_origin_y + (difference / 2.f);
      std::cout << "Moved boundaries in y-dimension to: [" << min_y_displacement_ << ", " << max_y_displacement_ << "]." << std::endl;
    }
  }

  displacement_ = new double[x_displacement_*y_displacement_];

  // Read displacement
  if (NetCDFReader::readVariable2D(ncid, nc_displacement_displacement_key_, displacement_, x_displacement_, y_displacement_) == -1)
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

double applications::exahype2::swe::parser::TopologyParser::sample_bathymetry(double x, double y){
  auto x_cdf = transformIndexSimulationToCDFRange(x, l_nX_, min_x_bathymetry_, max_x_bathymetry_);
  auto y_cdf = transformIndexSimulationToCDFRange(y, l_nY_, min_y_bathymetry_, max_y_bathymetry_);
  // No defined bathymetry
  if (x_cdf > max_x_bathymetry_ || x_cdf < min_x_bathymetry_ || y_cdf > max_y_bathymetry_ || y_cdf < min_y_bathymetry_)
      return 20.f;
  return bathymetry_[transformIndexCDFRangeToArray(x_cdf, y_cdf, min_x_bathymetry_, min_y_bathymetry_, max_x_bathymetry_, max_y_bathymetry_, x_bathymetry_, y_bathymetry_)];
}

double applications::exahype2::swe::parser::TopologyParser::sample_displacement(double x, double y){
  auto x_cdf = transformIndexSimulationToCDFRange(x, l_nX_, min_x_bathymetry_, max_x_bathymetry_);
  auto y_cdf = transformIndexSimulationToCDFRange(y, l_nY_, min_y_bathymetry_, max_y_bathymetry_);
  // Is displacement defined 
  if (x_cdf <= max_x_displacement_ && x_cdf >= min_x_displacement_ && y_cdf <= max_y_displacement_ && y_cdf >= min_y_displacement_)
    return displacement_[transformIndexCDFRangeToArray(x_cdf, y_cdf, min_x_displacement_, min_y_displacement_, max_x_displacement_, max_y_displacement_, x_displacement_, y_displacement_)];
  return 0.0;
}






