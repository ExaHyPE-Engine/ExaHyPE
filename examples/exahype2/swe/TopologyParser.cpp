#include "TopologyParser.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>  

tarch::logging::Log   examples::exahype2::swe::TopologyParser::_log( "examples::exahype2::swe::TopologyParser" );

examples::exahype2::swe::TopologyParser::TopologyParser(std::string filename, std::string dispfilename){

  this->filepath=filename;
  this->parsetopofile();
  
  this->dispfilepath=dispfilename;
  this->parsedisplacementfile();

}

examples::exahype2::swe::TopologyParser::~TopologyParser(){
  this->zvalues.clear();
  this->zvalues.shrink_to_fit();
  this->displacement.clear();
  this->displacement.shrink_to_fit();
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
        if(!isspace(intermediate[0]) && !intermediate.empty()){
          tokens.push_back(intermediate); 
        }
    } 
    
    return tokens;
}

void examples::exahype2::swe::TopologyParser::parsetopofile(){

  std::ifstream topofile (this->filepath);
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

double examples::exahype2::swe::TopologyParser::sampletopology(double x, double y){
  
  int index = floor((1.0-y)*this->ncols)*this->nrows + floor(x*this->nrows);
	
  float z = this->zvalues.at(index);

  return z;

}


void examples::exahype2::swe::TopologyParser::parsedisplacementfile(){
  std::ifstream dispfile (this->dispfilepath);
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
        this->dispmt = value;
      }      
      else if (thisline.size() == 2 && thisline.back() == "xlower"){
        this->dispxlowerleft = value;
      }            
      else if (thisline.size() == 2 && thisline.back() == "ylower"){
        this->dispylowerleft = value;
      }   
      else if (thisline.size() == 2 && thisline.back() == "t0"){
        this->dispt0 = value;
      }         
      else if (thisline.size() == 2 && thisline.back() == "dx"){
        this->dispdx = value;
      } 
      else if (thisline.size() == 2 && thisline.back() == "dy"){
        this->dispdy = value;
      } 
      else if (thisline.size() == 2 && thisline.back() == "dt"){
        this->dispdt = value;
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
    std::cout << "Error opening file " << this->dispfilepath; 
  }
}

double mapCoordinate(double c_in, double dc_in, double clower_in, double n_in, double dc_out, double clower_out, double n_out){

  double c_out = c_in*n_in - (clower_out - clower_in)/dc_in;
  
  return c_out;

}


double examples::exahype2::swe::TopologyParser::sampledisplacement(double x, double y, double t){

  double disp_x = mapCoordinate(x, this->cellsize, this->xlowerleft, this->ncols, this->dispdx, this->dispxlowerleft, this->dispmx);
  double disp_y = mapCoordinate(1.0-y, this->cellsize, this->ylowerleft, this->nrows, this->dispdy, this->dispylowerleft, this->dispmy);

  if(disp_x < 0 || disp_y < 0 || disp_x >= dispmx || disp_y >= dispmy){
    return 0.0;
  } 

  int index = floor(disp_y)*this->dispmx + floor(disp_x);
		
	
  float z =0.0;
  
  z = this->displacement.at(index);

  return z;

}






