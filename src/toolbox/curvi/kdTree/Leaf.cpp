#include "toolbox/curvi/kdTree/Leaf.h"

toolbox::curvi::Leaf::Leaf():
  boundary(BlockBoundary(Coordinate::X, Coordinate::X, Coordinate::X)) {
  isInit = false;
};


void toolbox::curvi::Leaf::init(BlockBoundary& a_boundary, uint modes[3]) {
  boundary = a_boundary;
  isInit   = true;
}

std::string toolbox::curvi::Leaf::toString(int depth) {
  checkInit();

  std::stringstream ss;
  for (int i = 0; i < depth; i++) {
    ss << "\t";
  }
  ss << "Leaf: " << std::endl;
  ss << boundary.toString(depth + 1) << std::endl;
  return ss.str();
}

toolbox::curvi::Block toolbox::curvi::Leaf::traverse() {
  checkInit();

  Block block(boundary);
  return block;
}

toolbox::curvi::BlockBoundary toolbox::curvi::Leaf::traverse(int index[3]) {
  checkInit();
  return boundary;
}

toolbox::curvi::BlockBoundary toolbox::curvi::Leaf::traverseProjection(double coordinate[3]) {
  checkInit();
  return boundary;
}

void toolbox::curvi::Leaf::checkInit() {
  if (!isInit) {
    throw std::logic_error("Leaf not initialized yet");
  }
}