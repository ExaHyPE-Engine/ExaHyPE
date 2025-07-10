// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/geometry/Face.h"
#include "toolbox/curvi/kdTree/Root.h"

#include <string>
#include <yaml-cpp/yaml.h>

namespace toolbox {
  namespace curvi {
    class YAMLInput {
    public:
      YAMLInput(std::string input_file);

      void  parseDomain(double a_offset[3], double a_size[3]);
      void  parseGrid(uint a_grid[3], uint ez, uint ey, uint ex);
      void  parseModes(uint mode[3]);
      Root* parse(double offset[3], double size[3], uint grid[3]);

    private:
      YAML::Node config;
    };
  } // namespace curvi
} // namespace toolbox