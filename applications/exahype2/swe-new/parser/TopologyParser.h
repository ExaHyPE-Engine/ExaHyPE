//GeoClaw topology file paser
//
#ifndef _examples_exahype2_swe_TopologyParser_H_
#define _examples_exahype2_swe_TopologyParser_H_
#include "./../preprocessor_defintion.h"


#include "tarch/logging/Log.h"
#include <string>
#include <vector>

namespace examples {namespace exahype2 {namespace swe {
  class TopologyParser;

}}}



class examples::exahype2::swe::TopologyParser {
  private:
    static tarch::logging::Log _log;
    
    std::vector <std::string> splitline(std::string line, char delimiter);
    
    std::vector <double> bathymetry;
    #if defined ASCBATHY
    int ncols = -1; 
    int nrows = -1;
    double x_bathymetry_lower_left = 0.0;
    double y_bathymetry_lower_left = 0.0;
    double cellsize = 0.0;   
    #elif defined NCBATHY
    #endif
 
    std::vector <double> displacement;
    #ifdef TT3DISPL
    int dispmx = -1;
    int dispmy = -1;
    double x_displacement_lower_left = 0.0;
    double y_displacement_lower_left = 0.0;
    double dx_displacement = 0.0;
    double dy_displacement = 0.0;
    #elif defined NCDISPL
    #endif

  public:
  
    TopologyParser(std::string bathymetry_file_path, std::string displacement_file_path);
    ~TopologyParser();

    void parse_bathymetry_file(std::string bathymetry_file_path);
    double sample_bathymetry(double x, double y);
    std::vector <double> get_bathymetry() {return this->bathymetry;}

    #if defined ASCBATHY
    int getncols() {return this->ncols;} 
    int getnrows() {return this->nrows;}
    double get_x_bathymetry_lower_left() {return this->x_bathymetry_lower_left;}
    double get_y_bathymetry_lower_left() {return this->y_bathymetry_lower_left;}
    double getcellsize() {return this->cellsize;} 
    #elif defined NCBATHY
    #endif
    
    void parse_displacement_file(std::string displacement_file_path);
    double sample_displacement(double x, double y);
    std::vector <double> get_displacement() {return this->displacement;}

    #if defined TT3DISPL
    int getdispmx() {return this->dispmx;}
    int getdispmy() {return this->dispmy;}
    double get_x_displacement_lower_left() {return x_displacement_lower_left;}
    double get_y_displacement_lower_left() {return y_displacement_lower_left;}
    double get_dx_displacement() {return this->dx_displacement;}
    double get_dy_displacement() {return this->dy_displacement;}
    #elif defined NCDISPL
    #endif
    
            
};


#endif
