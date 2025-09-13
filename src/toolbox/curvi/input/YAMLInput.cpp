// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "toolbox/curvi/input/YAMLInput.h"

#include "toolbox/curvi/kdTree/Root.h"

#include <algorithm>
#include <easi/YAMLParser.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <yaml-cpp/yaml.h>


toolbox::curvi::YAMLInput::YAMLInput(std::string input_file) {
  // check if file exists
  std::ifstream f(input_file.c_str());
  if (!f.good()) {
    std::stringstream ss;
    ss << "Could not open file ";
    ss << input_file;
    throw std::runtime_error("Could not open file");
  }
  std::cout << "Reading " << input_file << std::endl;
  config = YAML::LoadFile(input_file);
}

void toolbox::curvi::YAMLInput::parseDomain(double a_offset[3], double a_size[3]) {
  if (bool(config["domain"]) & bool(config["domain"]["offset"]) & bool(config["domain"]["size"])) {

    std::vector<double> offset = config["domain"]["offset"].as<std::vector<double>>();
    std::vector<double> size   = config["domain"]["size"].as<std::vector<double>>();

    std::copy_n(offset.data(), 3, a_offset);
    std::copy_n(size.data(), 3, a_size);
  } else {
    std::stringstream ss;
    ss << "Domain specifiation not valid: Missing keys are: " << std::endl;
    if (!config["domain"]) {
      ss << "domain:" << std::endl;
    }
    if (!config["domain"]["offset"]) {
      ss << "offset: [double, double, double]" << std::endl;
    }
    if (!config["domain"]["size"]) {
      ss << "size: [double, double, double]" << std::endl;
    }

    throw std::invalid_argument(ss.str());
  }
}


void toolbox::curvi::YAMLInput::parseGrid(uint a_grid[3], uint ez, uint ey, uint ex) {
  std::vector<uint> grid;

  if (!config["domain"]["grid"] && !config["domain"]["mode"]) {
    std::stringstream ss;
    ss << "Grid specifiation not valid: Missing keys are: " << std::endl;
    ss << "domain:" << std::endl;
    ss << "grid: [int, int ,int]" << std::endl;
    ss << "or" << std::endl;
    ss << "mode: [int, int ,int]" << std::endl;
  }
  if (bool(config["domain"]["grid"])) {
    grid = config["domain"]["grid"].as<std::vector<uint>>();
  } else {
    grid    = config["domain"]["mode"].as<std::vector<uint>>();
    grid[0] = (grid[0] - 1) * ez + 1;
    grid[1] = (grid[1] - 1) * ey + 1;
    grid[2] = (grid[2] - 1) * ex + 1;
  }
  std::copy_n(grid.data(), 3, a_grid);
}

void toolbox::curvi::YAMLInput::parseModes(uint a_mode[3]) {
  if (bool(config["domain"]) & bool(config["domain"]["mode"])) {
    std::vector<uint> mode = config["domain"]["mode"].as<std::vector<uint>>();
    std::copy_n(mode.data(), 3, a_mode);
  } else {
    std::stringstream ss;
    ss << "Domain specifiation not valid: Missing keys are: " << std::endl;
    if (!config["domain"]["mode"]) {
      ss << "mode: [uint, uint ,uint]" << std::endl;
    }
    throw std::invalid_argument(ss.str());
  }
}


toolbox::curvi::Root* toolbox::curvi::YAMLInput::parse(double offset[3], double size[3], uint grid[3]) {
  Root* root = new Root(config["topography"], offset, size, grid);
  return root;
}
