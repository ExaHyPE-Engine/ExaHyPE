#ifndef EXASEIS_REFINEMENT_HEADER
#define EXASEIS_REFINEMENT_HEADER

// #include "kernels/GaussLegendreBasis.h"
#include "exahype2/RefinementControl.h"

namespace Refinement {
  template <class Shortcuts>
  class RefinementCriterion {
  public:
    virtual bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) = 0;
  };
  template <class Shortcuts>
  class trackVelocity: public RefinementCriterion<Shortcuts> {
  public:
    trackVelocity(int a_basisSize, double a_refine_threshold, double a_coarsen_threshold):
      basisSize(a_basisSize),
      refine_threshold_2(a_refine_threshold * a_refine_threshold),
      coarsen_threshold_2(a_coarsen_threshold * a_coarsen_threshold){};

    bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) override {
      Shortcuts     s;
      kernels::idx4 idx(basisSize, basisSize, basisSize, s.SizeVariables + s.SizeParameters);
      double        maxVelocity_2 = 0;
      double        minVelocity_2 = std::numeric_limits<double>::max();
      for (int i = 0; i < basisSize; i++) {
        for (int j = 0; j < basisSize; j++) {
          for (int k = 0; k < basisSize; k++) {
            double velocity_2 = luh[idx(i, j, k, s.v + 0)] * luh[idx(i, j, k, s.v + 0)]
                                + luh[idx(i, j, k, s.v + 1)] * luh[idx(i, j, k, s.v + 1)]
                                + luh[idx(i, j, k, s.v + 2)] * luh[idx(i, j, k, s.v + 2)];

            maxVelocity_2 = std::max(maxVelocity_2, velocity_2);
            minVelocity_2 = std::min(minVelocity_2, velocity_2);
          }
        }
      }

      refine = ::exahype2::RefinementCommand::Keep;

      if (refine_threshold_2 < maxVelocity_2) {
        refine = ::exahype2::RefinementCommand::Refine;
        return false;
      }

      if (coarsen_threshold_2 > minVelocity_2) {
        refine = ::exahype2::RefinementCommand::Erase;
        return false;
      }

      return false;
    };

  private:
    int    basisSize;
    double refine_threshold_2;
    double coarsen_threshold_2;
  };

  template <class Shortcuts>
  class StaticAMR: public RefinementCriterion<Shortcuts> {
  public:
    bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) override {
      // static AMR
      if (!tarch::la::equals(t, 0.0)) {
        refine = ::exahype2::RefinementCommand::Keep;
        return true;
      }
      return false;
    }
  };

  template <class Shortcuts>
  class RefineBetweenPositions: public RefinementCriterion<Shortcuts> {
  public:
    RefineBetweenPositions(int level, double a_left[3], double a_right[3]):
      max_level(level) {
      rectangle_left[0] = a_left[0];
      rectangle_left[1] = a_left[1];
      rectangle_left[2] = a_left[2];

      rectangle_right[0] = a_right[0];
      rectangle_right[1] = a_right[1];
      rectangle_right[2] = a_right[2];
    }

    bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) override {
      double left_vertex[3];
      double right_vertex[3];
      bool   elementBetweenPoints = true;

      for (int i = 0; i < Dimensions; i++) {
        left_vertex[i]  = center[i] - dx[i] * 0.5;
        right_vertex[i] = center[i] + dx[i] * 0.5;
        elementBetweenPoints &= ((left_vertex[i] < rectangle_right[i]) && (right_vertex[i] > rectangle_left[i]));
      }

      if (elementBetweenPoints && (max_level > level)) { // solver->getMaximumAdaptiveMeshLevel()-1
        refine = ::exahype2::RefinementCommand::Refine;
        return true;
      }
      return false;
    }

  private:
    double rectangle_left[3];
    double rectangle_right[3];
    int    max_level;
  };

  template <class Shortcuts>
  class RefineDownToPositionCustomCoordinates: public RefinementCriterion<Shortcuts> {
  public:
    RefineDownToPositionCustomCoordinates(int a_basisSize, int level, double a_position[3], int a_top, double* a_FL):
      basisSize(a_basisSize),
      max_level(level),
      top(a_top) {
      position[0] = a_position[0];
      position[1] = a_position[1];
      position[2] = a_position[2];
      FL          = new double[a_basisSize];
      std::copy_n(a_FL, a_basisSize, FL);
    }

    bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) {
      double left_vertex = std::numeric_limits<double>::min();

      Shortcuts     s;
      kernels::idx4 idx(basisSize, basisSize, basisSize, s.SizeVariables + s.SizeParameters);
      for (int j = 0; j < basisSize; j++) {
        for (int k = 0; k < basisSize; k++) {
          double vertex = 0;
          for (int i = 0; i < basisSize; i++) {
            if (top == 2) {
              vertex += luh[idx(i, j, k, s.curve_grid + top)] * FL[i];
            } else if (top == 1) {
              vertex += luh[idx(j, i, k, s.curve_grid + top)] * FL[i];
            } else if (top == 0) {
              vertex += luh[idx(j, k, i, s.curve_grid + top)] * FL[i];
            }
          }
          left_vertex = std::max(left_vertex, vertex);
        }
      }

      bool elementAbovePointSource = (left_vertex < position[top]);
      if (elementAbovePointSource) { // solver->getMaximumAdaptiveMeshLevel()-1
        refine = ::exahype2::RefinementCommand::Refine;
        return true;
      }
      return false;
    }

  private:
    double  position[3];
    double* FL;
    int     basisSize;
    int     max_level;
    int     top;
  };

  template <class Shortcuts>
  class RefineDownToPosition: public RefinementCriterion<Shortcuts> {
  public:
    RefineDownToPosition(int a_basisSize, int level, double a_position[3], int a_top):
      basisSize(a_basisSize),
      max_level(level),
      top(a_top) {
      position[0] = a_position[0];
      position[1] = a_position[1];
      position[2] = a_position[2];
    }

    bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) {
      double left_vertex[Dimensions];
      for (int d = 0; d < Dimensions; d++) {
        left_vertex[d] = center[d] - dx[d] * 0.5;
      }

      bool elementAbovePointSource = (left_vertex[top] < position[top]);
      if (elementAbovePointSource) { // solver->getMaximumAdaptiveMeshLevel()-1
        refine = ::exahype2::RefinementCommand::Refine;
        return true;
      }
      return false;
    }

  private:
    double position[3];
    int    basisSize;
    int    max_level;
    int    top;
  };

  template <class Shortcuts>
  class RefinePositionCustomCoordinates: public RefinementCriterion<Shortcuts> {
  public:
    RefinePositionCustomCoordinates(double a_position[3], int a_basisSize, double* a_FL, double* a_FR) {
      position[0] = a_position[0];
      position[1] = a_position[1];
      position[2] = a_position[2];

      basisSize = a_basisSize;
      FR        = new double[a_basisSize];
      std::copy_n(a_FR, a_basisSize, FR);

      FL = new double[a_basisSize];
      std::copy_n(a_FL, a_basisSize, FL);
    };

    inline bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) {
      double        left_vertex[3];
      double        right_vertex[3];
      Shortcuts     s;
      bool          positionInElement = true;
      kernels::idx4 idx(basisSize, basisSize, basisSize, s.SizeVariables + s.SizeParameters);
      for (int i = 0; i < basisSize; i++) {
        left_vertex[0] += luh[idx(0, 0, i, s.curve_grid + 0)] * FL[i];
        right_vertex[0] += luh[idx(basisSize - 1, basisSize - 1, i, s.curve_grid + 0)] * FR[i];

        left_vertex[1] += luh[idx(0, i, 0, s.curve_grid + 1)] * FL[i];
        right_vertex[1] += luh[idx(basisSize - 1, i, basisSize - 1, s.curve_grid + 1)] * FR[i];

        left_vertex[2] += luh[idx(i, 0, 0, s.curve_grid + 2)] * FL[i];
        right_vertex[2] += luh[idx(i, basisSize - 1, basisSize - 1, s.curve_grid + 2)] * FR[i];
      }

      for (int i = 0; i < Dimensions; i++) {
        positionInElement &= ((left_vertex[i] <= position[i]) && (right_vertex[i] >= position[i]));
      }

      if (positionInElement) {
        refine = ::exahype2::RefinementCommand::Refine;
        return true;
      }
      return false;
    }

  private:
    double  position[3];
    double* FL;
    double* FR;
    int     basisSize;
  };

  template <class Shortcuts>
  class RefinePosition: public RefinementCriterion<Shortcuts> {
  public:
    RefinePosition(double a_position[3]) {
      position[0] = a_position[0];
      position[1] = a_position[1];
      position[2] = a_position[2];
    };

    inline bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) {
      double left_vertex[3];
      double right_vertex[3];

      bool positionInElement = true;

      for (int i = 0; i < Dimensions; i++) {
        left_vertex[i]  = center[i] - dx[i] * 0.5;
        right_vertex[i] = center[i] + dx[i] * 0.5;
      }

      for (int i = 0; i < Dimensions; i++) {
        positionInElement &= ((left_vertex[i] <= position[i]) && (right_vertex[i] >= position[i]));
      }

      if (positionInElement) {
        refine = ::exahype2::RefinementCommand::Refine;
        return true;
        std::cout << "refine ps" << std::endl;
      }
      return false;
    }

  private:
    double position[3];
  };

  template <class Shortcuts>
  class CoarseBoundaryLayer: public RefinementCriterion<Shortcuts> {
  private:
    std::vector<double> threshold_p[3];
    std::vector<double> threshold_m[2];

    double coarsest_size;
    int    max_depth; // Maximal tracked depth
    int    coarsest_level;
    int    top;
    int    left;
    int    front;

  public:
    CoarseBoundaryLayer(
      int          a_max_depth,
      int          a_coarsest_level,
      double       a_coarsest_size,
      int          a_top,
      const double domainSize[3],
      const double domainOffset[3]
    ):
      max_depth(a_max_depth),
      coarsest_level(a_coarsest_level),
      coarsest_size(a_coarsest_size),
      top(a_top) {

      left  = (top + 1) % 3;
      front = (top + 2) % 3;

      // We first compute properties of the mesh for the coarsest mesh size

      // The coorindates for the coars layers around the fine domain
      double coarse_layer[3];

      // We aim for 1/3 of the domain in each direction
      /*      coarse_layer[left ] = 1.0/3.0;
      coarse_layer[top  ] = 1.0/3.0;
      coarse_layer[front] = 1.0/3.0;*/

      coarse_layer[left]  = 12.5 / 25.0;
      coarse_layer[top]   = 12.5 / 25.0;
      coarse_layer[front] = 12.5 / 25.0;

      double coarse_elts[3];
      // Number of elements for the estimated layer
      coarse_elts[left]  = domainSize[left] / coarsest_size;
      coarse_elts[top]   = domainSize[top] / coarsest_size;
      coarse_elts[front] = domainSize[front] / coarsest_size;

      // Adjust positions to fit with coarsest mesh size,
      // we round down elements inside the fine domain
      coarse_layer[left]  = std::floor(coarse_elts[left] * coarse_layer[left]) / coarse_elts[left];
      coarse_layer[top]   = std::floor(coarse_elts[top] * coarse_layer[top]) / coarse_elts[top];
      coarse_layer[front] = std::floor(coarse_elts[front] * coarse_layer[front]) / coarse_elts[front];

      // for each possible depth (0 beeing the coarsest layer) we store the refinement threshold
      // left
      threshold_p[0].resize(max_depth);
      threshold_m[0].resize(max_depth);

      // front
      threshold_p[1].resize(max_depth);
      threshold_m[1].resize(max_depth);

      // top
      threshold_p[2].resize(max_depth);

      for (int depth = 0; depth < max_depth; depth++) {
        // the total number of elements in each dimenion for the current depth
        double num_elts[3];
        num_elts[left]  = coarse_elts[left] * pow(3.0, depth);
        num_elts[top]   = coarse_elts[top] * pow(3.0, depth);
        num_elts[front] = coarse_elts[front] * pow(3.0, depth);

        // the number of elements on one side of each layer for the current depth
        int num_layer_elements[3];
        num_layer_elements[left]  = coarse_layer[left] * num_elts[left];
        num_layer_elements[top]   = coarse_layer[top] * num_elts[top];
        num_layer_elements[front] = coarse_layer[front] * num_elts[front];

        // For each depth level we add a single element to the step wise layer
        for (int d = 0; d < depth; d++) {
          num_layer_elements[left] += std::pow(3.0, d);
          num_layer_elements[top] += std::pow(3.0, d);
          num_layer_elements[front] += std::pow(3.0, d);
        }

        // width of the coarse layer for the current depth
        double threshold_width[3];
        threshold_width[0] = domainSize[left] / num_elts[left] * num_layer_elements[left];
        threshold_width[1] = domainSize[front] / num_elts[front] * num_layer_elements[front];
        threshold_width[2] = domainSize[top] / num_elts[top] * num_layer_elements[top];

        // boundaries of the coarse domain
        // index for level = 2 is 0
        threshold_m[0][depth] = domainOffset[left] + threshold_width[0];
        threshold_m[1][depth] = domainOffset[front] + threshold_width[1];

        threshold_p[0][depth] = domainOffset[left] + domainSize[left] - threshold_width[0];
        threshold_p[1][depth] = domainOffset[front] + domainSize[front] - threshold_width[1];
        threshold_p[2][depth] = domainOffset[top] + domainSize[top] - threshold_width[2];
      }
    }

    bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) {
      // depth of the current element
      int depth = level - coarsest_level;

      if (depth >= max_depth) {
        return false;
      }

      // check if cell is in the boundary layer
      bool cellInBoundaryLayer = (center[left] < threshold_m[0][depth] || center[left] > threshold_p[0][depth])
                                 || (/*center[1] < boundary_y_left ||*/ center[top] > threshold_p[2][depth])
                                 || (center[front] < threshold_m[1][depth] || center[front] > threshold_p[1][depth]);

      if (cellInBoundaryLayer) {
        refine = ::exahype2::RefinementCommand::Keep;
        return true;
      }

      return false;
    }
  };

  template <class Shortcuts>
  class RefineCubeAroundPosition: public RefinementCriterion<Shortcuts> {
  public:
    RefineCubeAroundPosition(double a_position[3], double radius) {
      for (int i = 0; i < Dimensions; i++) {
        left_vertice[i]  = a_position[i] - radius;
        right_vertice[i] = a_position[i] + radius;
      }
    }

    bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) {
      bool inRadius = true;
      for (int i = 0; i < Dimensions; i++) {
        inRadius &= center[i] < right_vertice[i];
        inRadius &= center[i] > left_vertice[i];
      }

      if (inRadius) {
        refine = ::exahype2::RefinementCommand::Refine;
        return true;
      }

      return false;
    }

  private:
    double left_vertice[3];
    double right_vertice[3];
  };

  template <class Shortcuts>
  class RefineFilterCube: public RefinementCriterion<Shortcuts> {
  public:
    RefineFilterCube(double a_left_vertice[3], double a_right_vertice[3], int a_level_inside, int a_level_outside):
      level_inside(a_level_inside),
      level_outside(a_level_outside) {
      for (int i = 0; i < Dimensions; i++) {
        left_vertice[i]  = a_left_vertice[i];
        right_vertice[i] = a_right_vertice[i];
      }
    }

    bool eval(
      const double*                                luh,
      const tarch::la::Vector<Dimensions, double>& center,
      const tarch::la::Vector<Dimensions, double>& dx,
      double                                       t,
      const int                                    level,
      ::exahype2::RefinementCommand&               refine
    ) {
      bool left_vertice_in  = true;
      bool right_vertice_in = true;
      for (int i = 0; i < Dimensions; i++) {
        left_vertice_in &= center[i] - dx[i] < right_vertice[i];
        left_vertice_in &= center[i] - dx[i] > left_vertice[i];

        right_vertice_in &= center[i] + dx[i] < right_vertice[i];
        right_vertice_in &= center[i] + dx[i] > left_vertice[i];
      }

      if (left_vertice_in || right_vertice_in) {
        if (level == level_inside) {
          if (refine == ::exahype2::RefinementCommand::Refine) {
            refine = ::exahype2::RefinementCommand::Keep;
          }
        }
      } else {
        if (level == level_outside) {
          if (refine == ::exahype2::RefinementCommand::Refine) {
            refine = ::exahype2::RefinementCommand::Keep;
          }
        }
      }
      return true;
    }

  private:
    double left_vertice[3];
    double right_vertice[3];
    int    level_inside;
    int    level_outside;
  };

} // namespace Refinement

#endif
