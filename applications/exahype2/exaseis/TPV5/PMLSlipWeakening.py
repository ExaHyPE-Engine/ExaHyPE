def initial():
  return """
  VariableShortcuts s;

  std::fill_n(Q, (NumberOfAuxiliaryVariables + NumberOfUnknowns) * (Order + 1) * (Order + 1) * (Order + 1), 0);

  context->initUnknownsPatch(Q, x, h, 0.0, 0.0,
  [&](
      const {{SOLUTION_STORAGE_PRECISION}}* const x,
      const tarch::la::Vector<Dimensions,double>& center,
      const double t,
      const double dt, 
      {{SOLUTION_STORAGE_PRECISION}}* Q
    ) -> void {
      Q[s.rho] = 2.67;
      Q[s.cp ] = 6.0;
      Q[s.cs ] = 3.464;
    }
  );

  // Initialisation of PML parameters

  double dmp_pml_width_x = pml_cell_width * h[0];
  double dmp_pml_width_y = pml_cell_width * h[1];
  double dmp_pml_width_z = pml_cell_width * h[2];

  double dmp_pml_right_x = DomainOffset[0] + DomainSize[0] - dmp_pml_width_x;
  double dmp_pml_right_y = DomainOffset[1] + DomainSize[1] - dmp_pml_width_y;
  double dmp_pml_right_z = DomainOffset[2] + DomainSize[2] - dmp_pml_width_z;

  double dmp_pml_left_x = DomainOffset[0] + dmp_pml_width_x;
  double dmp_pml_left_y = DomainOffset[1] + dmp_pml_width_y;
  double dmp_pml_left_z = DomainOffset[2] + dmp_pml_width_z;

  constexpr double amplitude_nominator = 6.0;

  double dmp_amplitude_x = (pml_power + 1) * amplitude_nominator / (2 * dmp_pml_width_x) * log(1.0 / pml_rel_error);
  double dmp_amplitude_y = (pml_power + 1) * amplitude_nominator / (2 * dmp_pml_width_y) * log(1.0 / pml_rel_error);
  double dmp_amplitude_z = (pml_power + 1) * amplitude_nominator / (2 * dmp_pml_width_z) * log(1.0 / pml_rel_error);

  double offset_x = x[0] - h[0] * 0.5;
  double offset_y = x[1] - h[1] * 0.5;
  double offset_z = x[2] - h[2] * 0.5;

  idx4 idx(Order + 1, Order + 1, Order + 1, NumberOfUnknowns + NumberOfAuxiliaryVariables);

  for (int k = 0; k < Order + 1; k++) { 
    double computational_z = (offset_z + h[2] * QuadraturePoints1d[k]); //QuadPoints are in opposite order to exa1
    for (int j = 0; j < Order + 1; j++) {
      double computational_y = (offset_y + h[1] * QuadraturePoints1d[j]); //QuadPoints are in opposite order to exa1
      for (int i = 0; i < Order + 1; i++) {
        double computational_x = (offset_x + h[0] * QuadraturePoints1d[i]); //QuadPoints are in opposite order to exa1

        if (_TOP != 0) { //  No PML on surface
          if (computational_x < dmp_pml_left_x) {
            Q[idx(
              k, j, i, s.dmp_pml + 0
            )] = dmp_amplitude_x * pow((dmp_pml_left_x - computational_x) / dmp_pml_width_x, pml_power);
          }
        }
        if (_TOP != 1) { //  No PML on surface
          if (computational_y <= dmp_pml_left_y) {
            Q[idx(
              k, j, i, s.dmp_pml + 1
            )] = dmp_amplitude_y * pow((dmp_pml_left_y - computational_y) / dmp_pml_width_y, pml_power);
          }
        }
        if (_TOP != 2) { //  No PML on surface
          if (computational_z <= dmp_pml_left_z) {
            Q[idx(
              k, j, i, s.dmp_pml + 2
            )] = dmp_amplitude_z * pow((dmp_pml_left_z - computational_z) / dmp_pml_width_z, pml_power);
          }
        }
        if (computational_x >= dmp_pml_right_x) {
          Q[idx(
            k, j, i, s.dmp_pml + 0
          )] = dmp_amplitude_x * pow((computational_x - dmp_pml_right_x) / dmp_pml_width_x, pml_power);
        }
        if (computational_y >= dmp_pml_right_y) {
          Q[idx(
            k, j, i, s.dmp_pml + 1
          )] = dmp_amplitude_y * pow((computational_y - dmp_pml_right_y) / dmp_pml_width_y, pml_power);
        }
        if (computational_z >= dmp_pml_right_z) {
          Q[idx(
            k, j, i, s.dmp_pml + 2
          )] = dmp_amplitude_z * pow((computational_z - dmp_pml_right_z) / dmp_pml_width_z, pml_power);
        }
      }
    }
  }
"""

def riemann_solver():
  return """
  if (isBoundaryFace) {
    double* FIn  = faceIndex < Dimensions ? FR : FL;
    double* FOut = faceIndex < Dimensions ? FL : FR;
    std::copy_n(FIn, NumberOfUnknowns*(Order+1)*(Order+1), FOut);
  }

  context->riemannSolver(
    FL, FR,
    QL, QR,
    t, dt,
    h,
    direction,
    isBoundaryFace,
    faceIndex,
    1 //surface
  );
"""

def userIncludes():
  return """
#define _CUSTOM_COORDINATES
#define ASAGI_NOMPI
#define _TOP 1

#include "../ExaSeis_core/Curvilinear/ContextCurvilinearPMLSlipWeakening.h"
#include "../ExaSeis_core/Numerics/riemannsolver_pml_slipweakening.h"
"""

def abstractDeclarations():
  return """
public:
  double QuadraturePoints1d[Order+1];
  
private:
  ContextCurvilinearPMLSlipWeakening<VariableShortcuts, Order + 1, NumberOfUnknowns, NumberOfAuxiliaryVariables, {{SOLUTION_STORAGE_PRECISION}}>* context;

  int                                                              pml_cell_width;
  double                                                           pml_alpha_const;
  double                                                           pml_alpha_scalar;
  double                                                           pml_rel_error;
  int                                                              pml_power;
"""

def abstractConstructor():
  return """
  std::copy_n(
    kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
    (Order+1),
    QuadraturePoints1d    
  );

  std::string scenario_string   = "TPV5";
  std::string topography_string = "tpv5_79.yaml";

  pml_cell_width   = 3;
  pml_power        = 1;
  pml_alpha_const  = 1.5;
  pml_alpha_scalar = 0.0;
  pml_rel_error    = 0.001;

  //setting coarsestMeshLevel and maxAdaptiveMeshLevel
  double maxDSize = std::numeric_limits<double>::min();
  double minDSize = std::numeric_limits<double>::max();
  tarch::la::Vector<Dimensions, double> sizes = DomainSize;
  
  for(int i=0; i<Dimensions; i++){
    if(sizes[i]>maxDSize){
      maxDSize = sizes[i];
    }
    if(sizes[i]<minDSize){
      minDSize = sizes[i];
    }    
  }
  
  int depth = 0;
  
  //finding coarsest possible Mesh level
  while(maxDSize>MaxAdmissibleCellH){
    depth += 1;
    maxDSize /= 3.0;
  }
  
  double realCoarsestMeshSize = maxDSize;
  double coarsestMeshLevel = depth;  
  
  depth = 0;
  while(minDSize>MinAdmissibleCellH){
    depth += 1;
    minDSize /= 3.0;
  }

  logTraceOutWith1Argument("Freesurface set at ", std::to_string(_TOP * 2));

  context = new ContextCurvilinearPMLSlipWeakening<VariableShortcuts, Order + 1, NumberOfUnknowns, NumberOfAuxiliaryVariables, {{SOLUTION_STORAGE_PRECISION}}>(
    topography_string,
    // filename_rupture_model,
    coarsestMeshLevel,
    realCoarsestMeshSize,
    depth,
    DomainOffset, DomainSize,
    kernels::ElasticSolver::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::nodes,
    kernels::ElasticSolver::DGMatrices<{{SOLUTION_STORAGE_PRECISION}}>::dudx
  );

  std::cout << "PML is using " << pml_cell_width << " elements" << std::endl;
  std::cout << "Freesurface set at " << _TOP * 2 << std::endl;
"""

def finish_time_step_implementation():
  return """
  if (_solverState==SolverState::GridInitialisation) {
    std::string filename_rupture_model  = "tpv5_fault.yaml";
    context->initRuptureModel(filename_rupture_model);
  }
"""