#pragma once

#include "ContextCurvilinear.h"

#include "asagi_reader.h"

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
    asagiReader = new AsagiReader("");
    easi::YAMLParser parser(3,asagiReader);

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
  }

  ~ContextDynamicRupture() {
    delete asagiReader;
  }

public:

  void riemannSolver(
    T* FL, T* FR,
    const T* const QL, const T* const QR,
    const double t, const double dt,
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

  AsagiReader* asagiReader;
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

};
