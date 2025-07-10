// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

/* Represents a 2D face    */
/*     0               nx  */
/*      --------------     */
/*  ny |              | ny */
/*     |              |    */
/*     |              |    */
/*     |              |    */
/*   0 |              | 0  */
/*      --------------     */
/*     0               nx  */

#include "toolbox/curvi/geometry/Edge.h"
#include "toolbox/curvi/netcdf/NetCDFWriter.h"

#include <functional>
#include <string>
#include <vector>

namespace toolbox {
  namespace curvi {
    class FaceBoundary;
  } // namespace curvi
} // namespace toolbox

namespace toolbox {
  namespace curvi {
    class Face {
    public:
      Face();
      Face(
        int                                   a_index_offset[3],
        FaceBoundary&                         boundary,
        std::function<double(double, double)> a_perturbation = [](double y, double x) { return 0.0; },
        bool                                  keepBoundary   = false
      );

      Face(
        const Coordinate                      a_coords[2],
        const int                             a_index_offset[3],
        const int                             a_index_size[3],
        const double                          offset_mu,
        std::function<double(double, double)> a_perturbation = [](double y, double x) { return 0.0; }
      );

      Face(const Face& other);     /*Copy constructor*/
      Face& operator=(Face other); /*Copy assignment*/

      Coordinate getCoord(int i) const;
      void       getCoord(Coordinate a_coord[2]) const;

      int  getIndexOffset(Coordinate coord) const;
      void getIndexOffset(int a_indexoffset[3]) const;

      int  getIndexSize(Coordinate coord) const;
      void getIndexSize(int a_size[3]) const;

      void getIndexRange(Coordinate coord, std::vector<int>& range) const;

      std::function<double(double, double)> getPerturbation() const;

      double getPosition() const;
      Edge   split(Face& other, Face& left, Face& right) const;

      bool intersect(
        double  f0,
        double  f1,
        double  t0,
        double  t1,
        double  x0,
        double  x1,
        double  y0,
        double  y1,
        double& x,
        double& y
      ) const;

      void reverseDimension(Coordinate coord);
      void remesh();

      void subface(int subface_index_offset[3], int subface_index_size[3], Face& subface) const;

      const std::vector<double>& getNodes() const;
      double&                    operator()(int coord, int j, int i);
      const double&              operator()(int coord, int j, int i) const;
      const double&              operator()(int coord, Coordinate coord_j, int j, Coordinate coord_i, int i) const;
      double&                    operator()(int coord, Coordinate coord_j, int j, Coordinate coord_i, int i);

      void                 toNetCDF(std::string filename) const;
      std::string          printNodes() const;
      friend std::ostream& operator<<(std::ostream& ss, const Face& f);

    private:
      Coordinate                            coords[2];
      int                                   index_offset[3];
      int                                   index_size[3];
      double                                position;
      std::function<double(double, double)> perturbation;
      std::vector<double>                   nodes;
    };
  } // namespace curvi
} // namespace toolbox