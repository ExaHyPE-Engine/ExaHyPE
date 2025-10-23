// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/kdTree/Node.h"

namespace toolbox {
  namespace curvi {
    class InnerNode: public Node {
    public:
      InnerNode(const YAML::Node& node);

      ~InnerNode();

      std::string toString(int depth = 0);

      void init(BlockBoundary& boundary, uint modes[3]);

      BlockBoundary traverse(int a_index[3]);

      BlockBoundary traverseProjection(double coordinate[3]);

      Block traverse();

      void setPosition(double a_position);

      double getPosition();

      void getCoordinates(Coordinate a_coords[2]);

      Coordinate getFaceNormal();

      double evalPerturbation(double y, double x);

      void checkInit();

      bool isLeaf() const { return false; }


    private:
      bool isInit;

      Coordinate                            coords[2];
      double                                position;
      int                                   index;
      easi::Component*                      model;
      std::function<double(double, double)> perturbation;


      Node* left;
      Node* right;
    };
  } // namespace curvi
} // namespace toolbox