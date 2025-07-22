// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Face.h"
#include "FaceBoundary.h"
#include "toolbox/curvi/Coordinate.h"
#include "toolbox/curvi/Index.h"

#include <cmath>
#include <sstream>

/* Represents the face boundaries of a 3D block */

namespace toolbox {
  namespace curvi {
    class BlockBoundary {

    public:
      enum BlockFace { LEFT = 0, RIGHT = 1, BOTTOM = 2, TOP = 3, FRONT = 4, BACK = 5 };

      BlockBoundary(Coordinate a_coords[3]);
      BlockBoundary(Coordinate c0, Coordinate c1, Coordinate c2);
      BlockBoundary(Face topography, bool top, int n_depth, double depth);

      double& operator()(BlockFace face, Coordinate coord, int j, int i);

      const Face& getFace(BlockFace face);
      const Face& getFace(Coordinate coord, bool upper);

      void setFace(BlockFace face, const Face& other);
      void setFace(Coordinate coord, bool upper, Face other);

      void split(Face& face, BlockBoundary& left, BlockBoundary& right);

      void setCoord(Coordinate a_coords[3]);

      void       getCoord(Coordinate a_coords[3]);
      Coordinate getCoord(int i);

      void setIndexOffset(int a_index_offset[3]);
      void setIndexSize(int a_index_size[3]);

      void getIndexOffset(int a_index_offset[3]);
      int  getIndexOffset(Coordinate coord);

      void getIndexSize(int a_index_size[3]);
      int  getIndexSize(Coordinate coord);

      void getBoundingBox(double offset[3], double size[3]);

      void toNetCDF(std::string prefix);

      FaceBoundary setupSplittingFace(
        Coordinate cycx[2],
        int        fault_index,
        double     fault_position,
        int        indexOffset_fault[3]
      );

      std::string toString(int depth = 0);

      std::string printFaces();

      void checkConformity();

    private:
      Face left_face;
      Face right_face;
      Face bottom_face;
      Face top_face;
      Face front_face;
      Face back_face;

      int        index_offset[3];
      int        index_size[3];
      Coordinate coords[3];
    };
  } // namespace curvi
} // namespace toolbox