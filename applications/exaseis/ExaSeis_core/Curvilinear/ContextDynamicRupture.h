#pragma once

#include "ContextCurvilinear.h"

#include "toolbox/curvi/kdTree/InnerNode.h"
#include "toolbox/curvi/kdTree/Root.h"

template <class Shortcuts, int basisSize, int numberOfVariables, int numberOfParameters, typename T>
class ContextDynamicRupture: public ContextCurvilinear<Shortcuts, basisSize, numberOfVariables+numberOfParameters, T> {

public:
  ContextDynamicRupture(
    std::string& topography_string,
    int coarsestMeshLevel,
    double coarsestMeshSize, 
    double maxAdaptiveDepth, 
    tarch::la::Vector<DIMENSIONS,double> _domainOffset, 
    tarch::la::Vector<DIMENSIONS,double> _domainSize,
    T* _nodes,
    T* _dudx
  ): ContextCurvilinear<Shortcuts, basisSize, numberOfVariables+numberOfParameters, T>(
      topography_string,
      coarsestMeshLevel, coarsestMeshSize, maxAdaptiveDepth, 
      _domainOffset, _domainSize,
      _nodes,_dudx
    )
  {

      domainSize[0] = _domainSize[0];
      domainSize[1] = _domainSize[1];
      domainSize[2] = _domainSize[2];

  }

  void initRuptureModel(
    std::string& filename_rupture_model
  ){

    easi::YAMLParser parser(3);

    model = parser.parse(filename_rupture_model);

    //double cohesion;
    easi::ArraysAdapter<T> adapter;
    adapter.addBindingPoint("mu_s",&mu_s);
    adapter.addBindingPoint("mu_d",&mu_d);
    adapter.addBindingPoint("d_c" ,&S_c);
    adapter.addBindingPoint("t_0" ,&t0_rupture);
    adapter.addBindingPoint("f_cy",&f_cy);
    adapter.addBindingPoint("f_wy",&f_wy);
    adapter.addBindingPoint("f_cz",&f_cz);
    adapter.addBindingPoint("f_wz",&f_wz);

    easi::Query query(1,3);
    query.group(0) = 0;
    query.x(0,0) = 0;
    query.x(0,1) = 0;
    query.x(0,2) = 0;

    model->evaluate(query,adapter);


    // We want to find the position of the fault so that we can check whether we
    // are on the fault in the Riemann solver.
    // Since the curve might be rough or not aligned with our domain axes, the position
    // of the curve in reference space is not constant.
    // However since curvi maps the reference space onto the physical space in such a way
    // that the fault is contiguous, this means that there is one position in the reference
    // space (i.e. Peano positions) that the fault falls on, and we would like to find this position.
    // Curvi does not however does not provide this position in a trivial way, so what we do
    // is we try to find one point which is on the fault, and invert the projection to find
    // what position in reference space maps to this point.
    // This will not provide an entirely accurate position, but should provide a rough
    // estimate, which we assume is accurate to a half-cells distance.

    toolbox::curvi::Root* root = this->interface->getRoot();
    toolbox::curvi::InnerNode* fault_node = static_cast<toolbox::curvi::InnerNode*>(root->getChild());
    fault_normal = fault_node->getFaceNormal();

    // We look for a point on the fault and, for lack of better options, pick
    // the point at the center of the fault
    double fault_center[3];
    fault_center[toolbox::curvi::Coordinate::X] = this->domainOffset[0] + domainSize[0];
    fault_center[toolbox::curvi::Coordinate::Y] = this->domainOffset[1] + domainSize[1];
    fault_center[toolbox::curvi::Coordinate::Z] = this->domainOffset[2] + domainSize[2];

    // Get the position on the plane, i.e. if you are on a y-z plane you
    // need y and z to query the perturbation at this point
    toolbox::curvi::Coordinate fault_coords[2];
    fault_node->getCoordinates(fault_coords);
    T xi = fault_center[(2-fault_coords[0])];
    T mu = fault_center[(2-fault_coords[1])];

    // this gives the base position of the fault in the physical space,
    // i.e. if the fault has been placed at x = 20.0 this returns 20.,
    // but does not take into account possible perturbations on said fault
    // so we need to add these
    fault_center[fault_normal] = fault_node->getPosition()
      + fault_node->evalPerturbation(xi, mu);

    // Finally, we query the interface as to the position in reference space which
    // would get mapped to this position
    fault_position = this->interface->invertProjection(fault_normal, fault_center);


  }

public:

  bool riemannSolver(
    T* FL, T* FR,
    const T* const QL, const T* const QR,
    const double t, const double dt,
    const tarch::la::Vector<DIMENSIONS, double>& facePos,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const int direction,
    bool isBoundaryFace,
    int faceIndex,
    int surface = 2
  );

  void initialStressTensor(
    T& sxx, T& syy, T& szz,
    T& sxy, T& sxz, T& syz,
    double* const x);
  void preStress(
    T& T0_n, T& T0_m, T& T0_l,
    double* const x, double t,
    T* const l, T* const m, T* const n);
  T boxcar(T& f, double x, T w);
  void tauStrength(T& tau_str, T sigma_n, T S, double* const x, double t);
  void slipWeakening(
    T& v1, T& v2,
    T& Vel, T& tau1, T& tau2,
    T phi_1, T phi_2,
    T eta, T tau_str, T sigma_n);
  void slipWeakeningFriction(
    T vn_p, T vn_m, 
    T Tn_p, T Tn_m,
    T zn_p, T zn_m,
    T& vn_hat_p, T& vn_hat_m,
    T& Tn_hat_p, T& Tn_hat_m,
    T vm_p, T vm_m,
    T Tm_p, T Tm_m,
    T zl_p, T zl_m,
    T& vm_hat_p, T& vm_hat_m,
    T& Tm_hat_p, T& Tm_hat_m,
    T vl_p,T vl_m,
    T Tl_p, T Tl_m,
    T zm_p, T zm_m,
    T& vl_hat_p, T& vl_hat_m,
    T& Tl_hat_p, T& Tl_hat_m,
    T* const l, T* const m, T* const n,
    double* const x, T S, double t);

public:
  double domainSize[DIMENSIONS];

  easi::Component* model;

  //rupture parameters
  T S_c;  //Slip-weakening critical distance (meters)
  T mu_d; //Dynamic coefficient of friction (dimensionless)
  T mu_s; //Static coefficient of friction (dimensionless)

  //duration of forced rupture (seconds)
  // the forces holding the fault together will decrease linearly for this duration starting at the forced rupture time.
  T t0_rupture; 

  /**
   * These define the position of the area with frictional cohesion.
   * Specifically, these define a rectangle of size 2*f_wy x 2*f_wz
   * in the y-z axis centered around the point f_cy x f_cz.
   * If within this rectangle, frictional cohesion applies, otherwise
   * it does not.
   * i.e. if -f_wy < y - f_cy < f_wy and -f_wz < z - f_cz < f_wz,
   * frictional cohesion applies.
   */
  T f_cy;
  T f_wy;
  T f_cz;
  T f_wz;

  // In reference space, i.e. this should correspond to the position
  // as viewed by ExaHyPE of the fault.
  T fault_position;

  // Normal on which the fault lies, i.e. if this axis is x, then
  // the fault is on a y-z plane.
  toolbox::curvi::Coordinate fault_normal;
};
