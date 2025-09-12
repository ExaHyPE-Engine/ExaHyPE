// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "toolbox/curvi/Index.h"
#include "toolbox/curvi/geometry/Block.h"
#include "toolbox/curvi/geometry/BlockBoundary.h"
#include "toolbox/curvi/geometry/FaceBoundary.h"

namespace toolbox {
  namespace curvi {
    namespace Interpolation {
      class Transfinite {
      public:
        /**2D Transfinite interpolation
         * Performs 2d transfinite interpolation on a uniform grid.
         * left_bnd, right_bnd, bottom_bnd and top_bnd contain the interpolation values
         * on the boundaires of a uniform grid nx x ny.
         * The result are the interpolated values on the uniform grid.
         *
         *               top_bnd
         *  top_bnd(0)             top_bnd(nx-1)
         *      (0,my)o~~o~~o~~o~~o(nx,ny)
         *            (  :  :  :  (
         *            o--o--o--o--o
         *            )  :  :  :  )
         *   left_bnd o--o--o--o--o right_bnd
         *            (  :  :  :  (
         *            o--o--o--o--o
         *            )  :  :  :  )
         *       (0,0)o~~o~~o~~o~~o(nx,0)
         * bottom_bnd(0)             bottom_bnd(nx-1)
         *             bottom_bnd
         **/
        static void interpolate2D(FaceBoundary& boundary, std::vector<double>& face) {

          Coordinate eta = boundary.getCoord(1);
          Coordinate xi  = boundary.getCoord(0);

          int neta = boundary.getIndexSize(eta);
          int nxi  = boundary.getIndexSize(xi);

          double mesh_size_eta = 1.0 / (neta - 1);
          double mesh_size_xi  = 1.0 / (nxi - 1);

          double r;
          double q;

          idx3 id_dxy(NCOORDS, nxi, neta);
          idx2 id_dx(NCOORDS, neta);
          idx2 id_dy(NCOORDS, nxi);

          Edge& left_bnd   = boundary.getEdge(eta, false);
          Edge& right_bnd  = boundary.getEdge(eta, true);
          Edge& bottom_bnd = boundary.getEdge(xi, false);
          Edge& top_bnd    = boundary.getEdge(xi, true);

          for (int d = 0; d < NCOORDS; d++) {
            for (int j = 0; j < nxi; j++) {
              for (int i = 0; i < neta; i++) {
                q = (i)*mesh_size_eta;
                r = (j)*mesh_size_xi;
                // From https://en.wikipedia.org/wiki/Transfinite_interpolation
                face[id_dxy(
                  d,
                  j,
                  i
                )] = (1 - q) * left_bnd(d, j) + q * right_bnd(d, j) + (1 - r) * bottom_bnd(d, i) + r * top_bnd(d, i)
                     - (1 - q) * (1 - r) * left_bnd(d, 0) // intersection left bottom
                     - q * (1 - r) * right_bnd(d, 0)      // intersection right bottom
                     - (1 - q) * r * top_bnd(d, 0)        // intersection left top
                     - q * r * top_bnd(d, neta - 1);      // intersection right top
              }
            }
          }
        }


        static void interpolate3D(BlockBoundary& boundary, Block& block) {
          interpolate3D(
            boundary,
            boundary.getIndexOffset(boundary.getCoord(0)), // mu
            boundary.getIndexSize(boundary.getCoord(0)),
            boundary.getIndexOffset(boundary.getCoord(1)), // xi
            boundary.getIndexSize(boundary.getCoord(1)),
            boundary.getIndexOffset(boundary.getCoord(2)), // eta
            boundary.getIndexSize(boundary.getCoord(2)),
            block
          );
        }


        static void interpolate3D(
          BlockBoundary& boundary,
          int            o_mu,
          int            s_mu,
          int            o_xi,
          int            s_xi,
          int            o_eta,
          int            s_eta,
          Block&         block
        ) {

          Coordinate coords[3];
          boundary.getCoord(coords);
          Coordinate eta = coords[2];
          Coordinate xi  = coords[1];
          Coordinate mu  = coords[0];

          double bs_eta = boundary.getIndexSize(eta);
          double bs_xi  = boundary.getIndexSize(xi);
          double bs_mu  = boundary.getIndexSize(mu);

          double bo_eta = boundary.getIndexOffset(eta);
          double bo_xi  = boundary.getIndexOffset(xi);
          double bo_mu  = boundary.getIndexOffset(mu);

          double mesh_size_eta = 1.0 / double(bs_eta - 1.0);
          double mesh_size_xi  = 1.0 / double(bs_xi - 1.0);
          double mesh_size_mu  = 1.0 / double(bs_mu - 1.0);

          int bp_eta = bo_eta + bs_eta;
          int bp_xi  = bo_xi + bs_xi;
          int bp_mu  = bo_mu + bs_mu;


          int p_eta = o_eta + s_eta;
          int p_xi  = o_xi + s_xi;
          int p_mu  = o_mu + s_mu;

          double u, v, w;
          double uv, vw, uw, uvw1, uvw2;

          double d_eta, d_xi, d_mu;

          Face left_bnd   = boundary.getFace(eta, false); // mu  xi
          Face right_bnd  = boundary.getFace(eta, true);  // mu  xi
          Face bottom_bnd = boundary.getFace(mu, false);  // xi  eta
          Face top_bnd    = boundary.getFace(mu, true);   // xi  eta
          Face front_bnd  = boundary.getFace(xi, false);  // mu  eta
          Face back_bnd   = boundary.getFace(xi, true);   // mu  eta

          for (int d = 0; d < NCOORDS; d++) { // eta
            for (int i_eta = o_eta; i_eta < p_eta; i_eta++) {
              d_eta = mesh_size_eta * (i_eta - bo_eta);
              for (int i_xi = o_xi; i_xi < p_xi; i_xi++) { // xi
                d_xi = mesh_size_xi * (i_xi - bo_xi);
                for (int i_mu = o_mu; i_mu < p_mu; i_mu++) { // mu
                  d_mu = mesh_size_mu * (i_mu - bo_mu);

                  //(1-eta) * l(0,i_xi) + eta * r(0,i_xi)
                  u = (1 - d_eta) * left_bnd(d, mu, i_mu, xi, i_xi) + d_eta * right_bnd(d, mu, i_mu, xi, i_xi);

                  // d_mu = 0 => i_mu = 0
                  v = (1 - d_mu) * bottom_bnd(d, xi, i_xi, eta, i_eta) + d_mu * top_bnd(d, xi, i_xi, eta, i_eta);

                  //(1-xi) * f(0,i_eta) + xi * b(0,i_eta)
                  w = (1 - d_xi) * front_bnd(d, mu, i_mu, eta, i_eta) + d_xi * back_bnd(d, mu, i_mu, eta, i_eta);

                  // (1-eta) * l(0,i_xi) + eta * l(0,0) * (1-eta) * l(0,i_xi) + eta * l(0,0)
                  uw = (1 - d_eta) * (1 - d_xi) * left_bnd(d, mu, i_mu, xi, bo_xi)
                       + (1 - d_eta) * d_xi * left_bnd(d, mu, i_mu, xi, bp_xi - 1)
                       + d_eta * (1 - d_xi) * right_bnd(d, mu, i_mu, xi, bo_xi)
                       + d_eta * d_xi * right_bnd(d, mu, i_mu, xi, bp_xi - 1);

                  // (1-xi) * B(0,i_eta) + xi * B(n_xi-1,i_eta)
                  uv = (1 - d_mu) * (1 - d_eta) * bottom_bnd(d, xi, i_xi, eta, bo_eta)
                       + (1 - d_mu) * d_eta * bottom_bnd(d, xi, i_xi, eta, bp_eta - 1)
                       + d_mu * (1 - d_eta) * top_bnd(d, xi, i_xi, eta, bo_eta)
                       + d_mu * d_eta * top_bnd(d, xi, i_xi, eta, bp_eta - 1);

                  // (1-xi) * f(0,i_eta) + ix * b(0,i_eta)
                  vw = (1 - d_xi) * (1 - d_mu) * front_bnd(d, mu, bo_mu, eta, i_eta)
                       + (1 - d_xi) * d_mu * front_bnd(d, mu, bp_mu - 1, eta, i_eta)
                       + d_xi * (1 - d_mu) * back_bnd(d, mu, bo_mu, eta, i_eta)
                       + d_xi * d_mu * back_bnd(d, mu, bp_mu - 1, eta, i_eta);

                  //(1-xi)*(1-eta)  B(0,0) + (xi)*(1-eta)  B(n_xi-1,0)
                  uvw1 = (1 - d_mu) * (1 - d_xi) * (1 - d_eta) * bottom_bnd(d, xi, bo_xi, eta, bo_eta)
                         + (1 - d_mu) * d_xi * (1 - d_eta) * bottom_bnd(d, xi, bp_xi - 1, eta, bo_eta)
                         + d_mu * (1 - d_xi) * (1 - d_eta) * top_bnd(d, xi, bo_xi, eta, bo_eta)
                         + d_mu * d_xi * (1 - d_eta) * top_bnd(d, xi, bp_xi - 1, eta, bo_eta);

                  //(1-xi)*(1-eta)  B(0,n_eta-1) + (xi)*(1-eta)  B(n_xi-1,n_eta-1)
                  uvw2 = (1 - d_mu) * (1 - d_xi) * d_eta * bottom_bnd(d, xi, bo_xi, eta, bp_eta - 1)
                         + (1 - d_mu) * d_xi * d_eta * bottom_bnd(d, xi, bp_xi - 1, eta, bp_eta - 1)
                         + d_mu * (1 - d_xi) * d_eta * top_bnd(d, xi, bo_xi, eta, bp_eta - 1)
                         + d_mu * d_xi * d_eta * top_bnd(d, xi, bp_xi - 1, eta, bp_eta - 1);

                  block(d, coords[0], i_mu, coords[1], i_xi, coords[2], i_eta) = u + v + w - uv - uw - vw + uvw1 + uvw2;
                }
              }
            }
          }
        }
      };
    }; // namespace Interpolation
  } // namespace curvi
} // namespace toolbox