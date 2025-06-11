// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>
#include <yaml-cpp/yaml.h>
#include "toolbox/curvi/geometry/face.h"
#include "toolbox/curvi/kdTree/root.h"

class YAMLInput{
 public:
  YAMLInput(std::string input_file);
  
  void parseDomain(double a_offset[3],double a_size[3]);
  void parseGrid(uint a_grid[3], uint ez, uint ey, uint ex);
  void parseModes(uint mode[3]);
  Root* parse(double offset[3],double size[3], uint grid[3]);
  
 private:
  YAML::Node config;
};
