
#ifndef EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_GEMMSCPP_H_
#define EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_GEMMSCPP_H_


namespace generated {
namespace kernels {
namespace AderDG {
namespace nonlinear {

void gemm_4_4_4_face_Q_x(const double* A, const double* B, double* C);
void gemm_4_4_4_face_F_x(const double* A, const double* B, double* C);
void gemm_4_4_4_volume_x(const double* A, const double* B, double* C);
void gemm_4_4_4_volume_y(const double* A, const double* B, double* C);
void gemm_4_4_4_volume_y_add(const double* A, const double* B, double* C);
void gemm_4_4_4_rhs_x(const double* A, const double* B, double* C);
void gemm_4_4_4_rhs_y(const double* A, const double* B, double* C);
void gemm_4_4_4_lduh_x(const double* A, const double* B, double* C);
void gemm_4_4_4_lduh_y(const double* A, const double* B, double* C);
void gemm_4_4_4_lqi(const double* A, const double* B, double* C);

}
}
}
}


#endif //EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_GEMMSCPP_H_