// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/geometry/Block.h"

#include <map>

namespace toolbox {
  namespace curvi {
    namespace Interpolation {
      class Lagrange {
      public:
        static void interpolate3D(
          const double     position[3],
          const Block&     block,
          std::vector<int> range_z,
          std::vector<int> range_y,
          std::vector<int> range_x,
          double           result[NCOORDS]
        ) {
          for (int k : range_z) {
            double dz         = range_z.back() - range_z.front();
            double lagrange_z = uniformLagrangePolynomial(
              (position[0] - range_z.front()) / dz,
              k - range_z.front(),
              range_z.back() - range_z.front() + 1
            );
            for (int j : range_y) {
              double dy         = range_y.back() - range_y.front();
              double lagrange_y = uniformLagrangePolynomial(
                (position[1] - range_y.front()) / dy,
                j - range_y.front(),
                range_y.back() - range_y.front() + 1
              );
              for (int i : range_x) {
                double dx         = range_x.back() - range_x.front();
                double lagrange_x = uniformLagrangePolynomial(
                  (position[2] - range_x.front()) / dx,
                  i - range_x.front(),
                  range_x.back() - range_x.front() + 1
                );

                for (int d = 0; d < NCOORDS; d++) {
                  result[d] += lagrange_z * lagrange_y * lagrange_x
                               * block(d, Coordinate::Z, k, Coordinate::Y, j, Coordinate::X, i);
                }
              }
            }
          }
        };

        static void interpolate1D(
          double              position,
          std::vector<double> nodes,
          std::vector<double> values,
          double              result[NCOORDS]
        ) {
          // check if nodes and values fit:
          if (nodes.size() != values.size() / NCOORDS) {
            throw std::out_of_range("Values and nodes dont't fit in 1d lagrange interpolation");
          }

          std::fill_n(result, NCOORDS, 0);
          std::vector<int> index_range = Index::getIndexRange(0, nodes.size());
          for (int i : index_range) {
            double lagrange = lagrangePolynomial(position, i, nodes);
            idx2   idx(NCOORDS, nodes.size());
            for (int d = 0; d < NCOORDS; d++) {
              result[d] += lagrange * values[idx(d, i)];
            }
          }
        };

        static double lagrangePolynomial(double position, int i, std::vector<double> range) {
          double nominator   = 1;
          double denominator = 1;
          for (int j = 0; j < i; j++) {
            nominator *= (position - range[j]);
            denominator *= (range[i] - range[j]);
          }

          for (int j = i + 1; j < range.size(); j++) {
            nominator *= (position - range[j]);
            denominator *= (range[i] - range[j]);
          }

          return nominator / denominator;
        }


        static double uniformLagrangePolynomial(double position, int i, int range) {
          double              result       = 1;
          std::vector<double> uniform_mesh = getUniformMesh(range);

          for (int j = 0; j < i; j++) {
            result *= (position - uniform_mesh[j]);
          }

          for (int j = i + 1; j < range; j++) {
            result *= (position - uniform_mesh[j]);
          }

          result /= getLagrangeDenominator(i, range);

          return result;
        };

      private:
        static std::vector<double> getUniformMesh(int range) {
          // static std::map lagrangeDenominator;

          /*used to hash results*/
          /*       static std::map<int,std::vector<double>> mesh_map; */
          /* #pragma omp threadprivate(mesh_map)       */
          /*       if( mesh_map.find(range) != mesh_map.end()){ */
          /* 	return mesh_map[range]; */
          /*       } */
          /**********************/

          std::vector<double> uniform_mesh;

          for (int i = 0; i < range; i++) {
            uniform_mesh.push_back(i / double(range - 1));
          }
          // mesh_map[range] = uniform_mesh;
          return uniform_mesh;
        }

        static double getLagrangeDenominator(int i, int range) {

          /*used to hash results*/
          /*       typedef std::tuple<int,int> keytype; */
          /*       static std::map<keytype,double> denominator_map; */
          /* #pragma omp threadprivate(denominator_map) */
          /*       keytype key(i,range); */

          /*       if( denominator_map.find(key) != denominator_map.end()){ */
          /* 	return denominator_map[key]; */
          /*       } */
          /**********************/

          double              denominator = 1;
          std::vector<double> uniformMesh = getUniformMesh(range);
          for (int j = 0; j < i; j++) {
            denominator *= uniformMesh[i] - uniformMesh[j];
          }

          for (int j = i + 1; j < range; j++) {
            denominator *= uniformMesh[i] - uniformMesh[j];
          }

          // denominator_map[key] = denominator;
          return denominator;
        }
      };
    } // namespace Interpolation
  } // namespace curvi
} // namespace toolbox