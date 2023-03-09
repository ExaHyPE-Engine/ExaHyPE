
#ifndef EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_CONVERTER_H_
#define EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_CONVERTER_H_
#include <cstring>

#include "kernels/KernelUtils.h"

namespace generated {
namespace kernels {
namespace AderDG {
namespace nonlinear {
namespace converter {

  constexpr int getLduhGenArraySize()    { return 64;}
  constexpr int getLduhOptArraySize()    { return 64;}

  constexpr int getQhbndGenArraySize()   { return 64;}
  constexpr int getQhbndOptArraySize()   { return 64;}
  
  constexpr int getFhbndGenArraySize()   { return 64;}
  constexpr int getFhbndOptArraySize()   { return 64;}

  constexpr int getQFaceGenArraySize()   { return 16;}
  constexpr int getQFaceOptArraySize()   { return 16;}
  
  constexpr int getFFaceGenArraySize()   { return 16;}
  constexpr int getFFaceOptArraySize()   { return 16;}

  constexpr int getQhiGenArraySize()     { return 64;}
  constexpr int getQhiOptArraySize()     { return 64;}
  
  constexpr int getFhiGenArraySize()     { return 192;}
  constexpr int getFhiOptArraySize()     { return 192;}
  
  constexpr int getPSiGenArraySize()     { return 320;}
  constexpr int getPSiOptArraySize()     { return 320;}


  void Qhbnd_optimised2generic(const double* const opt, double* gen);
  void Qhbnd_generic2optimised(const double* const gen, double* opt);
  
  void Fhbnd_optimised2generic(const double* const opt, double* gen);
  void Fhbnd_generic2optimised(const double* const gen, double* opt);
  
  void QFace_optimised2generic(const double* const opt, double* gen);
  void QFace_generic2optimised(const double* const gen, double* opt);
  
  void FFace_optimised2generic(const double* const opt, double* gen);
  void FFace_generic2optimised(const double* const gen, double* opt);

  void Qhi_optimised2generic(const double* const opt, double* gen);
  void Qhi_generic2optimised(const double* const gen, double* opt);
  
  void Fhi_optimised2generic(const double* const opt, double* gen);
  void Fhi_generic2optimised(const double* const gen, double* opt);
    
  void PSi_optimised2generic(const double* const opt, double* gen);
  void PSi_generic2optimised(const double* const gen, double* opt);
  
  void Lduh_optimised2generic(const double* const opt, double* gen);
  void Lduh_generic2optimised(const double* const gen, double* opt);
}
}
}
}
}

#endif //EXAHYPE_GENERATED_KERNELS_ADERDG_NONLINEAR_CONVERTER_H_
