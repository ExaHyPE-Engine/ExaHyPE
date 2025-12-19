// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "BlockBoundary.h"

#include <string>
#include <vector>

namespace toolbox {
  namespace curvi {
    class Block {
    public:
      Block();

      Block(Coordinate a_coords[3], int a_index_offset[3], int a_index_size[3], std::vector<double> a_nodes);

      Block(BlockBoundary boundary);
      Block(BlockBoundary boundary, int a_index_offset[3], int a_index_size[3]);

      Block(const Block& block);     /*Copy constructor*/
      Block& operator=(Block block); /*Copy assignment*/

      Coordinate getCoord(int idx) const;
      void       getCoord(Coordinate a_coord[3]) const;
      void       getIndexSize(int a_index_size[3]) const;
      void       getIndexOffset(int a_index_offset[3]) const;

      Block apply(
        std::function<void(const Block&, int, int, int, Block&)> func,
        int                                                      mode_z,
        int                                                      mode_y,
        int                                                      mode_x
      ) const;


      Block resample(
        std::function<void(const Block&, int, int, int, Block&)> func,
        int                                                      in_z,
        int                                                      in_y,
        int                                                      in_x,
        int                                                      out_z,
        int                                                      out_y,
        int                                                      out_x
      ) const;

      /* Block interpolate(const std::vector<double>& nodes, */
      /*       	    int mode_z, int mode_y , int mode_x) const; */

      void interpolate(const double node[3], double result[3], const int strip = 10) const;

      Block getMetricDerivatives(double** dudx, int nodes_z, int nodes_y, int nodes_x, double dx[3]) const;

      void getIndexRange(Coordinate coord, int start, int end, std::vector<int>& a_range) const;
      void getIndexRange(Coordinate coord, std::vector<int>& a_range) const;

      /**
       * @todo I think this is not nice. Should link to tarch.
       *       The whole class also seems to use exceptions which is not good style
       *       either.
       */
      friend Block operator+(Block lhs, const Block& rhs) /*merge blocks*/;

      double&       operator()(int coord, int k, int j, int i);
      const double& operator()(int coord, int k, int j, int i) const;

      double& operator()(int coord, Coordinate coord_k, int k, Coordinate coord_j, int j, Coordinate coord_i, int i);

      const double& operator()(
        int        coord,
        Coordinate coord_k,
        int        k,
        Coordinate coord_j,
        int        j,
        Coordinate coord_i,
        int        i
      ) const;

      std::string toString() const;

      void appendToNetCDF(std::string filename, std::string prefix);
      void toNetCDF(std::string filename) const;

    private:
      std::vector<double> nodes;
      Coordinate          coords[3];
      int                 index_offset[3];
      int                 index_size[3];
    };
  } // namespace curvi
} // namespace toolbox
