#pragma once

#include "../Numerics/idx.h"
#include "CurvilinearDerivatives.h"

#include "toolbox/curvi/Coordinate.h"
#include "toolbox/curvi/geometry/Block.h"
#include "toolbox/curvi/interface/Interface.h"
#include "toolbox/curvi/kdTree/Root.h"

template <class Shortcuts, int basisSize, int numberOfData, typename T>
class ContextCurvilinear{

public:
  ContextCurvilinear(
    std::string&                            topography_string,
    int coarsestMeshLevel,
    double coarsestMeshSize, 
    double maxAdaptiveDepth, 
    tarch::la::Vector<DIMENSIONS,double> _domainOffset, 
    tarch::la::Vector<DIMENSIONS,double> _domainSize,
    T* _nodes,
    T* _dudx
  ):
  meshLevel(coarsestMeshLevel){
    std::copy_n(_nodes, basisSize , nodes);
    for(int i=0; i<basisSize; i++){
      for(int j=0; j<basisSize; j++){
        dudx[i][j] = _dudx[j*basisSize+i];
      }
    }

    max_dx = coarsestMeshSize * std::pow(1 / 3.0, maxAdaptiveDepth);

    domainOffset[0] = _domainOffset[0];
    domainOffset[1] = _domainOffset[1];
    domainOffset[2] = _domainOffset[2];

    uint elements_l[3];
    elements_l[toolbox::curvi::Coordinate::X] = std::round(_domainSize[0] / coarsestMeshSize);
    elements_l[toolbox::curvi::Coordinate::Y] = std::round(_domainSize[1] / coarsestMeshSize);
    elements_l[toolbox::curvi::Coordinate::Z] = std::round(_domainSize[2] / coarsestMeshSize);

    double size[3];
    size[toolbox::curvi::Coordinate::X] = _domainSize[0];
    size[toolbox::curvi::Coordinate::Y] = _domainSize[1];
    size[toolbox::curvi::Coordinate::Z] = _domainSize[2];

    double offset[3];
    offset[toolbox::curvi::Coordinate::X] = domainOffset[0];
    offset[toolbox::curvi::Coordinate::Y] = domainOffset[1];
    offset[toolbox::curvi::Coordinate::Z] = domainOffset[2];

    interface = new toolbox::curvi::Interface(topography_string, offset, size, elements_l, basisSize - 1);

    interface->initTree();

  }

  toolbox::curvi::Root* getRoot() { return interface->getRoot(); }

  ~ContextCurvilinear() {
    delete interface;
  }

  void initUnknownsPatch(
    T*                                           luh,
    const tarch::la::Vector<DIMENSIONS, double>& center,
    const tarch::la::Vector<DIMENSIONS, double>& dx,
    double                                       t,
    double                                       dt,
    std::function<void(
      const T* const x,
      const tarch::la::Vector<DIMENSIONS,double>& center,
      const double t,
      const double dt, 
      T* Q
    )> initUnknownsPointwise
  ) {

    if (tarch::la::equals(t, 0.0)) {
      // int           numberOfData = Shortcuts::NumberOfUnknowns + Shortcuts::NumberOfAuxiliaryVariables;
      kernels::idx4 id_xyzf(basisSize, basisSize, basisSize, numberOfData);
      kernels::idx3 id_xyz(basisSize, basisSize, basisSize);

      int ex = std::round((center[0] - domainOffset[0] - dx[0] / 2.0) / dx[0]);
      int ey = std::round((center[1] - domainOffset[1] - dx[1] / 2.0) / dx[1]);
      int ez = std::round((center[2] - domainOffset[2] - dx[2] / 2.0) / dx[2]);

#ifndef QUICK_IDENTITY
      toolbox::curvi::Block element = interface->getElement(nodes, ex, ey, ez);
      int   index_offset[3];
      element.getIndexOffset(index_offset);

      for (int k = 0; k < basisSize; k++) {
        int e_k = index_offset[toolbox::curvi::Coordinate::Z] + k;
        for (int j = 0; j < basisSize; j++) {
          int e_j = index_offset[toolbox::curvi::Coordinate::Y] + j;
          for (int i = 0; i < basisSize; i++) {
            int e_i = index_offset[toolbox::curvi::Coordinate::X] + i;

            // x,y,z
            luh[id_xyzf(k, j, i, Shortcuts::curve_grid + 0)] = element(
              toolbox::curvi::Coordinate::X,
              toolbox::curvi::Coordinate::Z, e_k,
              toolbox::curvi::Coordinate::Y, e_j,
              toolbox::curvi::Coordinate::X, e_i
            );
            luh[id_xyzf(k, j, i, Shortcuts::curve_grid + 1)] = element(
              toolbox::curvi::Coordinate::Y, 
              toolbox::curvi::Coordinate::Z, e_k,
              toolbox::curvi::Coordinate::Y, e_j,
              toolbox::curvi::Coordinate::X, e_i
            );
            luh[id_xyzf(k, j, i, Shortcuts::curve_grid + 2)] = element(
              toolbox::curvi::Coordinate::Z,
              toolbox::curvi::Coordinate::Z, e_k,
              toolbox::curvi::Coordinate::Y, e_j,
              toolbox::curvi::Coordinate::X, e_i
            );

          }
        }
      }

#else
      for (int k = 0; k < basisSize; k++) {
        for (int j = 0; j < basisSize; j++) {
          for (int i = 0; i < basisSize; i++) {
            luh[id_xyzf(k, j, i, Shortcuts::curve_grid + 0)] = nodes[i] * dx[0] + center[0] - dx[0] / 2;
            luh[id_xyzf(k, j, i, Shortcuts::curve_grid + 1)] = nodes[j] * dx[1] + center[1] - dx[1] / 2;
            luh[id_xyzf(k, j, i, Shortcuts::curve_grid + 2)] = nodes[k] * dx[2] + center[2] - dx[2] / 2;
          }
        }
      }
#endif

      T derivatives[basisSize * basisSize * basisSize * 10];
      std::fill_n(derivatives, basisSize * basisSize * basisSize * 10, 0.0);
      kernels::idx4 id_der(basisSize, basisSize, basisSize, 10);

      // compute metric derivatives//
#ifdef QUICK_IDENTITY

      for (int k = 0; k < basisSize; k++) {
        for (int j = 0; j < basisSize; j++) {
          for (int i = 0; i < basisSize; i++) {
            derivatives[id_der(k, j, i, 0)] = 1.0;
            derivatives[id_der(k, j, i, 1)] = 1.0;
            derivatives[id_der(k, j, i, 5)] = 1.0;
            derivatives[id_der(k, j, i, 9)] = 1.0;
          }
        }
      }
#else
      ExaSeis::Derivatives<Shortcuts, T, basisSize, numberOfData>::metricDerivatives(dudx, luh, &dx[0], derivatives);
#endif

      for (int k = 0; k < basisSize; k++) {
        for (int j = 0; j < basisSize; j++) {
          for (int i = 0; i < basisSize; i++) {
            std::copy_n(derivatives + id_der(k, j, i, 0), 10, luh + id_xyzf(k, j, i, Shortcuts::jacobian));
          }
        }
      }

      tarch::la::Vector<DIMENSIONS, double> curv_center;
      getElementCenter(luh, curv_center);
      for (int k = 0; k < basisSize; k++) {
        for (int j = 0; j < basisSize; j++) {
          for (int i = 0; i < basisSize; i++) {
            T coords[3] = {
              luh[id_xyzf(k, j, i, Shortcuts::curve_grid + 0)],
              luh[id_xyzf(k, j, i, Shortcuts::curve_grid + 1)],
              luh[id_xyzf(k, j, i, Shortcuts::curve_grid + 2)]};

              initUnknownsPointwise(
                coords, curv_center, t, dt, luh + id_xyzf(k, j, i, 0)
              );

          }
        }
      }

      for (int k = 0; k < basisSize; k++) {
        for (int j = 0; j < basisSize; j++) {
          for (int i = 0; i < basisSize; i++) {
            for (int m = 0; m < numberOfData; m++) {
              assertion5(std::isfinite(luh[id_xyzf(k, j, i, m)]), k, j, i, m, meshLevel);
            }
          }
        }
      }
    }
  }

  void correctPointSourceLocation(double pointSourceLocation[][3]){

    for (int p = 0; p < 1; p++) {
      double     coords[3];
      // invert coordinates as curvi order is zyx
      coords[toolbox::curvi::Coordinate::X] = pointSourceLocation[p][0];
      coords[toolbox::curvi::Coordinate::Y] = pointSourceLocation[p][1];
      coords[toolbox::curvi::Coordinate::Z] = pointSourceLocation[p][2];

      pointSourceLocation[p][0] = this->interface->invertProjection(toolbox::curvi::Coordinate::X, coords);
      pointSourceLocation[p][1] = this->interface->invertProjection(toolbox::curvi::Coordinate::Y, coords);
      pointSourceLocation[p][2] = this->interface->invertProjection(toolbox::curvi::Coordinate::Z, coords);
    }
  }

  void getElementSize(const T* const luh, tarch::la::Vector<DIMENSIONS, double>& dx) {

    // int           numberOfData = Shortcuts::NumberOfUnknowns + Shortcuts::NumberOfAuxiliaryVariables;
    kernels::idx4 id_xyzf(basisSize, basisSize, basisSize, numberOfData);

    dx[0] = 0;
    dx[1] = 0;
    dx[2] = 0;

    for (int i = 0; i < basisSize; i++) {
      for (int j = 0; j < basisSize; j++) {
        dx[0] = std::max(
          dx[0], luh[id_xyzf(i, j, basisSize - 1, Shortcuts::curve_grid + 0)] - luh[id_xyzf(i, j, 0, Shortcuts::curve_grid + 0)]
        );
        dx[1] = std::max(
          dx[1], luh[id_xyzf(i, basisSize - 1, j, Shortcuts::curve_grid + 1)] - luh[id_xyzf(i, 0, j, Shortcuts::curve_grid + 1)]
        );
        dx[2] = std::max(
          dx[2], luh[id_xyzf(basisSize - 1, i, j, Shortcuts::curve_grid + 2)] - luh[id_xyzf(0, i, j, Shortcuts::curve_grid + 2)]
        );
      }
    }
  }

  void getElementCenter(const T* const luh, tarch::la::Vector<DIMENSIONS, double>& center) {
    int center_i1 = int(std::ceil(basisSize / 2.0));
    int center_i2 = int(std::floor(basisSize / 2.0));

    // int           numberOfData = Shortcuts::NumberOfUnknowns + Shortcuts::NumberOfAuxiliaryVariables;
    kernels::idx4 id_xyzf(basisSize, basisSize, basisSize, numberOfData);

    center[0] = (luh[id_xyzf(center_i1, center_i1, center_i1, Shortcuts::curve_grid + 0)]
                 + luh[id_xyzf(center_i2, center_i2, center_i2, Shortcuts::curve_grid + 0)])
                / 2.0;
    center[1] = (luh[id_xyzf(center_i1, center_i1, center_i1, Shortcuts::curve_grid + 1)]
                 + luh[id_xyzf(center_i2, center_i2, center_i2, Shortcuts::curve_grid + 1)])
                / 2.0;
    center[2] = (luh[id_xyzf(center_i1, center_i1, center_i1, Shortcuts::curve_grid + 2)]
                 + luh[id_xyzf(center_i2, center_i2, center_i2, Shortcuts::curve_grid + 2)])
                / 2.0;
  }

  toolbox::curvi::Interface* getInterface(){
    return interface;
  }


  double max_dx;

protected:
  toolbox::curvi::Interface* interface;

  double nodes[basisSize];
  double dudx[basisSize][basisSize];

  double       domainOffset[3];
  const int    meshLevel;

};
