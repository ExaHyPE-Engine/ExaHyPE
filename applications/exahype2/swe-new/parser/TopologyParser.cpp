#include "TopologyParser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>  
tarch::logging::Log   examples::exahype2::swe::TopologyParser::_log( "examples::exahype2::swe::TopologyParser" );

examples::exahype2::swe::TopologyParser::TopologyParser(std::string bathymetry_file_path, std::string displacement_file_path){

  this->parse_bathymetry_file(bathymetry_file_path);

  this->parse_displacement_file(displacement_file_path);

}

examples::exahype2::swe::TopologyParser::~TopologyParser(){
  this->bathymetry.clear();
  this->bathymetry.shrink_to_fit();
  this->displacement.clear();
  this->displacement.shrink_to_fit();
  std::cout << "Destroy topology parser."; 
}

#if defined ASCBATHY || defined TT3DISPL
std::vector <std::string> examples::exahype2::swe::TopologyParser::splitline(std::string line, char delimiter){

    // Vector of string to save tokens 
    std::vector <std::string> tokens; 

    std::stringstream strstream(line);  
    std::string intermediate; 
      
    //Split
    while(getline(strstream, intermediate, delimiter)) 
    { 
        if(!isspace(intermediate[0]) && !intermediate.empty()){
          tokens.push_back(intermediate); 
        }
    } 
    
    return tokens;
}
#endif

#if defined ASCBATHY
void examples::exahype2::swe::TopologyParser::parse_bathymetry_file(std::string bathymetry_file_path){

  std::ifstream topofile (bathymetry_file_path);
  std::string line;
  std::vector <std::string> thisline;
  double value;

  
  if (topofile.is_open()){
  
    while(getline(topofile,line)){
    
      thisline = splitline(line, ' ');                
      value = std::stof(thisline.at(0));
    
      if (thisline.size() == 2 && thisline.back() == "ncols"){
        this->ncols = value;
      }
      else if (thisline.size() == 2 && thisline.back() == "nrows"){
        this->nrows = value;
      }
      else if (thisline.size() == 2 && thisline.back() == "xll"){
        this->x_bathymetry_lower_left = value;
      }            
      else if (thisline.size() == 2 && thisline.back() == "yll"){
        this->y_bathymetry_lower_left = value;
      }   
      else if (thisline.size() == 2 && thisline.back() == "cellsize"){
        this->cellsize = value;
      }         
      else if (thisline.size() == 2 && thisline.back() == "nodata_value"){
        continue;
      } 
      else if (thisline.size() == 1){
        this->bathymetry.push_back(value);
      }           
      else{
        std::cout << "Warning: Undefined entry in topology file!"; 
      } 
    }
    
    topofile.close();
  }

  else{
    std::cout << "Error opening file " << bathymetry_file_path; 
  }

}
#endif

double examples::exahype2::swe::TopologyParser::sample_bathymetry(double x, double y){
  
  #if defined ASCBATHY
  int index = floor((1.0-y)*this->ncols)*this->nrows + floor(x*this->nrows);
	
  float z = this->bathymetry.at(index);

  return z;
  #elif defined NCBATHY
  #endif

}

#if defined TT3DISPL
void examples::exahype2::swe::TopologyParser::parse_displacement_file(std::string displacement_file_path){
  std::ifstream dispfile (displacement_file_path);
  std::string line;
  std::vector <std::string> thisline;
  double value;

  
  if (dispfile.is_open()){
  
    while(getline(dispfile,line)){
    
      thisline = splitline(line, ' '); 
                 
      value = std::stof(thisline.at(0));
    
      if (thisline.size() == 2 && thisline.back() == "mx"){
        this->dispmx = value;
      }
      else if (thisline.size() == 2 && thisline.back() == "my"){
        this->dispmy = value;
      }
      else if (thisline.size() == 2 && thisline.back() == "mt"){
        continue;
      }      
      else if (thisline.size() == 2 && thisline.back() == "xlower"){
        this->x_displacement_lower_left = value;
      }            
      else if (thisline.size() == 2 && thisline.back() == "ylower"){
        this->y_displacement_lower_left = value;
      }   
      else if (thisline.size() == 2 && thisline.back() == "t0"){
        continue;
      }         
      else if (thisline.size() == 2 && thisline.back() == "dx"){
        this->dx_displacement = value;
      } 
      else if (thisline.size() == 2 && thisline.back() == "dy"){
        this->dy_displacement = value;
      } 
      else if (thisline.size() == 2 && thisline.back() == "dt"){
        continue;
      }             
      else if (thisline.size() > 2){
        for(int i=0; i < thisline.size(); i++){
            value = std::stof(thisline.at(i));
            this->displacement.push_back(value);
          }
      }           
      else{
        std::cout << "Warning: Undefined entry in displacement file!"; 
      } 
    }

    dispfile.close();
  }

  else{
    std::cout << "Error opening file " << displacement_file_path; 
  }
}

double mapCoordinate(double c_in, double dc_in, double clower_in, double n_in, double dc_out, double clower_out, double n_out){

  double c_out = c_in*n_in - (clower_out - clower_in)/dc_in;
  
  return c_out;

}
#endif


double examples::exahype2::swe::TopologyParser::sample_displacement(double x, double y){

#if defined TT3DISPL
  double disp_x = mapCoordinate(x, this->cellsize, this->x_bathymetry_lower_left, this->ncols, this->dx_displacement, this->x_displacement_lower_left, this->dispmx);
  double disp_y = mapCoordinate(y, this->cellsize, this->y_bathymetry_lower_left, this->nrows, this->dy_displacement, this->y_displacement_lower_left, this->dispmy);

  if(disp_x < 0 || disp_y < 0 || disp_x >= dispmx || disp_y >= dispmy){
    return 0.0;
  } 

  int index = floor(disp_y)*this->dispmx + floor(disp_x);
		
	
  float z =0.0;
  
  z = this->displacement.at(index);

  return z;
#elif defined NCDISPL
#endif
}






