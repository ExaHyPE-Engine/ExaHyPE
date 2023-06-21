#ifndef EXASEIS_CONTEXT_CURVILINEAR_HEADER
#define EXASEIS_CONTEXT_CURVILINEAR_HEADER

// #include "kernels/GaussLegendreBasis.h"
#include "Context.h"
#include "SolverInformationADERDG.h"
#include "curvi/coordinate.h"
#include "../Curvilinear/CurvilinearDerivatives.h"
#include "curvi/geometry/block.h"
#include "curvi/interface/interface.h"
#include "curvi/kdTree/root.h"
#include "exahype2/aderdg/kernels/Utils/KernelUtils.h"

template <class Shortcuts, int basisSize>
class ContextCurvilinear: public Context<Shortcuts, basisSize> {

public:
  ContextCurvilinear(
    std::string&                            scenario_string,
    std::string&                            a_topography_string,
    DomainInformation*                      a_domain_info,
    SolverInformationADERDG<basisSize - 1>* a_solver_info
  ):
    Context<Shortcuts, basisSize>(scenario_string, a_domain_info) {

    domain_info       = a_domain_info;
    solver_info       = a_solver_info;
    topography_string = a_topography_string;
  }

  void initTransformation() {
    uint elements_l[3];
    elements_l[Coordinate::X] = domain_info->elements[0];
    elements_l[Coordinate::Y] = domain_info->elements[1];
    elements_l[Coordinate::Z] = domain_info->elements[2];

    double size[3];
    size[Coordinate::X] = domain_info->domainSize[0];
    size[Coordinate::Y] = domain_info->domainSize[1];
    size[Coordinate::Z] = domain_info->domainSize[2];

    double offset[3];
    offset[Coordinate::X] = domain_info->domainOffset[0];
    offset[Coordinate::Y] = domain_info->domainOffset[1];
    offset[Coordinate::Z] = domain_info->domainOffset[2];

    interface = new Curvi::Interface(topography_string, offset, size, elements_l, basisSize - 1);
  }

  Root* getRoot() { return interface->getRoot(); }

  void initTree() { interface->initTree(); }

  ~ContextCurvilinear() {
    delete interface;
    delete domain_info;
    delete solver_info;
    //    delete topography;
  }

  virtual void initUnknownsPatch(
    double*                                      luh,
    const tarch::la::Vector<Dimensions, double>& center,
    const tarch::la::Vector<Dimensions, double>& dx,
    double                                       t,
    double                                       dt
  ) {

    if (tarch::la::equals(t, 0.0)) {
      Shortcuts     s;
      int           numberOfData = s.SizeVariables + s.SizeParameters;
      kernels::idx4 id_xyzf(basisSize, basisSize, basisSize, numberOfData);
      kernels::idx3 id_xyz(basisSize, basisSize, basisSize);

      int ez = std::round((center[2] - domain_info->domainOffset[2] - dx[2] / 2.0) / dx[2]);
      int ey = std::round((center[1] - domain_info->domainOffset[1] - dx[1] / 2.0) / dx[1]);
      int ex = std::round((center[0] - domain_info->domainOffset[0] - dx[0] / 2.0) / dx[0]);

#ifndef QUICK_IDENTITY
      Block element = interface->getElement(solver_info->nodes, ez, ey, ex);
      int   index_offset[3];
      element.getIndexOffset(index_offset);

      for (int k = 0; k < basisSize; k++) {
        int e_k = index_offset[Coordinate::Z] + k;
        for (int j = 0; j < basisSize; j++) {
          int e_j = index_offset[Coordinate::Y] + j;
          for (int i = 0; i < basisSize; i++) {
            int e_i = index_offset[Coordinate::X] + i;
            // x,y,z
            luh[id_xyzf(k, j, i, s.curve_grid + 0)] = element(
              2, Coordinate::Z, e_k, Coordinate::Y, e_j, Coordinate::X, e_i
            );
            luh[id_xyzf(k, j, i, s.curve_grid + 1)] = element(
              1, Coordinate::Z, e_k, Coordinate::Y, e_j, Coordinate::X, e_i
            );
            luh[id_xyzf(k, j, i, s.curve_grid + 2)] = element(
              0, Coordinate::Z, e_k, Coordinate::Y, e_j, Coordinate::X, e_i
            );
          }
        }
      }

#else
      for (int k = 0; k < basisSize; k++) {
        for (int j = 0; j < basisSize; j++) {
          for (int i = 0; i < basisSize; i++) {
            luh[id_xyzf(k, j, i, s.curve_grid + 0)] = solver_info->nodes[i] * dx[0] + center[0] - dx[0] / 2;
            luh[id_xyzf(k, j, i, s.curve_grid + 1)] = solver_info->nodes[j] * dx[1] + center[1] - dx[1] / 2;
            luh[id_xyzf(k, j, i, s.curve_grid + 2)] = solver_info->nodes[k] * dx[2] + center[2] - dx[2] / 2;
          }
        }
      }
#endif

      double derivatives[basisSize * basisSize * basisSize * 10];
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
      ExaSeis::Derivatives<Shortcuts, basisSize>::metricDerivatives(solver_info->dudx, luh, &dx[0], derivatives);
#endif

      for (int k = 0; k < basisSize; k++) {
        for (int j = 0; j < basisSize; j++) {
          for (int i = 0; i < basisSize; i++) {
            std::copy_n(derivatives + id_der(k, j, i, 0), 10, luh + id_xyzf(k, j, i, s.jacobian));
          }
        }
      }

      tarch::la::Vector<Dimensions, double> curv_center;
      getElementCenter(luh, curv_center);
      for (int k = 0; k < basisSize; k++) {
        for (int j = 0; j < basisSize; j++) {
          for (int i = 0; i < basisSize; i++) {
            double coords[3] = {
              luh[id_xyzf(k, j, i, s.curve_grid + 0)],
              luh[id_xyzf(k, j, i, s.curve_grid + 1)],
              luh[id_xyzf(k, j, i, s.curve_grid + 2)]};
            this->scenario->initUnknownsPointwise(coords, curv_center, t, dt, luh + id_xyzf(k, j, i, 0));
          }
        }
      }

      for (int k = 0; k < basisSize; k++) {
        for (int j = 0; j < basisSize; j++) {
          for (int i = 0; i < basisSize; i++) {
            for (int m = 0; m < numberOfData; m++) {
              assertion5(std::isfinite(luh[id_xyzf(k, j, i, m)]), k, j, i, m, domain_info->meshLevel);
            }
          }
        }
      }
    }
  }

  void initPointSourceLocation(double pointSourceLocation[][3]) override {
    this->scenario->initPointSourceLocation(pointSourceLocation);

    for (int p = 0; p < 1; p++) {
      Coordinate dir_top = static_cast<Coordinate>(_TOP);
      double     coords[3];
      // invert coordinates as curvi order is zyx
      coords[0] = pointSourceLocation[p][2];
      coords[1] = pointSourceLocation[p][1];
      coords[2] = pointSourceLocation[p][0];

      pointSourceLocation[p][_TOP] = this->interface->invertProjection(dir_top, coords);
    }
  }

  void getElementSize(const double* const luh, tarch::la::Vector<Dimensions, double>& dx) {
    Shortcuts     s;
    int           numberOfData = s.SizeVariables + s.SizeParameters;
    kernels::idx4 id_xyzf(basisSize, basisSize, basisSize, numberOfData);

    dx[0] = 0;
    dx[1] = 0;
    dx[2] = 0;

    for (int i = 0; i < basisSize; i++) {
      for (int j = 0; j < basisSize; j++) {
        dx[0] = std::max(
          dx[0], luh[id_xyzf(i, j, basisSize - 1, s.curve_grid + 0)] - luh[id_xyzf(i, j, 0, s.curve_grid + 0)]
        );
        dx[1] = std::max(
          dx[1], luh[id_xyzf(i, basisSize - 1, j, s.curve_grid + 1)] - luh[id_xyzf(i, 0, j, s.curve_grid + 1)]
        );
        dx[2] = std::max(
          dx[2], luh[id_xyzf(basisSize - 1, i, j, s.curve_grid + 2)] - luh[id_xyzf(0, i, j, s.curve_grid + 2)]
        );
      }
    }
  }

  void getElementCenter(const double* const luh, tarch::la::Vector<Dimensions, double>& center) {
    int center_i1 = int(std::ceil(basisSize / 2.0));
    int center_i2 = int(std::floor(basisSize / 2.0));

    Shortcuts     s;
    int           numberOfData = s.SizeVariables + s.SizeParameters;
    kernels::idx4 id_xyzf(basisSize, basisSize, basisSize, numberOfData);

    center[0] = (luh[id_xyzf(center_i1, center_i1, center_i1, s.curve_grid + 0)]
                 + luh[id_xyzf(center_i2, center_i2, center_i2, s.curve_grid + 0)])
                / 2.0;
    center[1] = (luh[id_xyzf(center_i1, center_i1, center_i1, s.curve_grid + 1)]
                 + luh[id_xyzf(center_i2, center_i2, center_i2, s.curve_grid + 1)])
                / 2.0;
    center[2] = (luh[id_xyzf(center_i1, center_i1, center_i1, s.curve_grid + 2)]
                 + luh[id_xyzf(center_i2, center_i2, center_i2, s.curve_grid + 2)])
                / 2.0;
  }

  DomainInformation* getDomainInfo() { return domain_info; }

  //  Topography* topography;
protected:
  Curvi::Interface* interface;

  DomainInformation*                      domain_info;
  SolverInformationADERDG<basisSize - 1>* solver_info;
  std::string                             topography_string;
};

#endif
