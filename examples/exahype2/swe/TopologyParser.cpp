#include "TopologyParser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>  

tarch::logging::Log   examples::exahype2::swe::TopologyParser::_log( "examples::exahype2::swe::TopologyParser" );

examples::exahype2::swe::TopologyParser::TopologyParser(std::string filename){

  this->filepath=filename;
  this->parsetopofile();

}

examples::exahype2::swe::TopologyParser::~TopologyParser(){
  std::cout << "Destroy topology parser."; 
}

std::vector <std::string> examples::exahype2::swe::TopologyParser::splitline(std::string line, char delimiter){

    // Vector of string to save tokens 
    std::vector <std::string> tokens; 
       
    std::stringstream strstream(line);  
    std::string intermediate; 
      
    //Split
    while(getline(strstream, intermediate, delimiter)) 
    { 
        tokens.push_back(intermediate); 
    } 
    
    return tokens;
}

void examples::exahype2::swe::TopologyParser::parsetopofile(){

  std::ifstream topofile (this->filepath);
  std::string line;
  std::vector <std::string> thisline;
  //std::stringstream ss; 
  float value;

  
  if (topofile.is_open()){
  
    while(getline(topofile,line)){
    
      thisline = splitline(line, ' ');      
           
     // ss << thisline.at(0);     
           
     // ss >> value;
      
      value = std::stof(thisline.at(0));
       
     // std::cout << thisline.at(0) << "\n";
     // std::cout << value << "\n";
    
      if (thisline.size() == 2 && thisline.back() == "ncols"){
        this->ncols = value;
      }
      else if (thisline.size() == 2 && thisline.back() == "nrows"){
        this->nrows = value;
      }
      else if (thisline.size() == 2 && thisline.back() == "xll"){
        this->xlowerleft = value;
      }            
      else if (thisline.size() == 2 && thisline.back() == "yll"){
        this->ylowerleft = value;
      }   
      else if (thisline.size() == 2 && thisline.back() == "cellsize"){
        this->cellsize = value;
      }         
      else if (thisline.size() == 2 && thisline.back() == "nodata_value"){
        this->nodatavalue = value;
      } 
      else if (thisline.size() == 1){
        this->zvalues.push_back(value);
      }           
      else{
        std::cout << "Warning: Undefined entry in topology file!"; 
      } 
    }
    
    topofile.close();
  }

  else{
    std::cout << "Error opening file " << this->filepath; 
  }

}

float examples::exahype2::swe::TopologyParser::sampletopology(float x, float y){
  
  int index = floor(y*361.0)*(this->ncols-1) + floor(x*361.0);
  
  //std::cout << x << " " << y << " " << index << "\n";
	
  float z = this->zvalues.at(index);
  
  //std::cout << z << "\n";
	
  return z;

}









