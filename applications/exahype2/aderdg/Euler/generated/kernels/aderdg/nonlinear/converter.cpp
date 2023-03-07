
#include "./converter.h"


void generated::kernels::AderDG::nonlinear::converter::Qhbnd_optimised2generic(const double* const opt, double* gen) {
  for(int d=0; d<4; d++) {
    for(int ij=0; ij<4; ij++) {
      #pragma omp simd aligned(gen,opt:16)
      for(int n=0; n<4; n++) {
        gen[n+4*(ij+4*d)] = opt[n+4*(ij+4*d)];
      }
    }    
  }
}

void generated::kernels::AderDG::nonlinear::converter::Qhbnd_generic2optimised(const double* const gen, double* opt) {
  for(int d=0; d<4; d++) {
    for(int ij=0; ij<4; ij++) {
      #pragma omp simd aligned(gen,opt:16)
      for(int n=0; n<4; n++) {
        opt[n+4*(ij+4*d)] = gen[n+4*(ij+4*d)];
      }
    }    
  }
}

void generated::kernels::AderDG::nonlinear::converter::Fhbnd_optimised2generic(const double* const opt, double* gen) {
  for(int d=0; d<4; d++) {
    for(int ij=0; ij<4; ij++) {
      #pragma omp simd aligned(gen,opt:16)
      for(int n=0; n<4; n++) {
        gen[n+4*(ij+4*d)] = opt[n+4*(ij+4*d)];
      }
    }    
  }
}

void generated::kernels::AderDG::nonlinear::converter::Fhbnd_generic2optimised(const double* const gen, double* opt) {
  for(int d=0; d<4; d++) {
    for(int ij=0; ij<4; ij++) {
      #pragma omp simd aligned(gen,opt:16)
      for(int n=0; n<4; n++) {
        opt[n+4*(ij+4*d)] = gen[n+4*(ij+4*d)];
      }
    }    
  }
}

void generated::kernels::AderDG::nonlinear::converter::QFace_optimised2generic(const double* const opt, double* gen) {
  for(int ij=0; ij<4; ij++) {
    #pragma omp simd aligned(opt:16) //gen not always aligned if nor face0
    for(int n=0; n<4; n++) {
      gen[n+4*ij] = opt[n+4*ij];
    }
  }    
}

void generated::kernels::AderDG::nonlinear::converter::QFace_generic2optimised(const double* const gen, double* opt) {
  for(int ij=0; ij<4; ij++) {
    #pragma omp simd aligned(opt:16) //gen not always aligned if nor face0
    for(int n=0; n<4; n++) {
      opt[n+4*ij] = gen[n+4*ij];
    }
  }    
}

void generated::kernels::AderDG::nonlinear::converter::FFace_optimised2generic(const double* const opt, double* gen) {
  for(int ij=0; ij<4; ij++) {
    #pragma omp simd aligned(opt:16) //gen not always aligned if nor face0
    for(int n=0; n<4; n++) {
      gen[n+4*ij] = opt[n+4*ij];
    }
  }    
}

void generated::kernels::AderDG::nonlinear::converter::FFace_generic2optimised(const double* const gen, double* opt) {
  for(int ij=0; ij<4; ij++) {
    #pragma omp simd aligned(opt:16) //gen not always aligned if nor face0
    for(int n=0; n<4; n++) {
      opt[n+4*ij] = gen[n+4*ij];
    }
  }    
}

void generated::kernels::AderDG::nonlinear::converter::Qhi_optimised2generic(const double* const opt, double* gen) {
  for(int j=0; j<16; j++) {
    #pragma omp simd aligned(gen,opt:16)
    for(int i=0; i<4; i++) {
      gen[i+4*j] = opt[i+4*j];
    }
  }
}

void generated::kernels::AderDG::nonlinear::converter::Qhi_generic2optimised(const double* const gen, double* opt) {
  for(int j=0; j<16; j++) {
    #pragma omp simd aligned(gen,opt:16)
    for(int i=0; i<4; i++) {
      opt[i+4*j] = gen[i+4*j];
    }
  }
}

void generated::kernels::AderDG::nonlinear::converter::Fhi_optimised2generic(const double* const opt, double* gen) {
  for(int b=0; b<3; b++) {
    for(int j=0; j<16; j++) {
      #pragma omp simd aligned(gen,opt:16)
      for(int i=0; i<4; i++) {
        gen[i+4*j+b*64] = opt[i+4*j+b*64];
      }
    }
  }
}

void generated::kernels::AderDG::nonlinear::converter::Fhi_generic2optimised(const double* const gen, double* opt) {
  for(int b=0; b<3; b++) {
    for(int j=0; j<16; j++) {
      #pragma omp simd aligned(gen,opt:16)
      for(int i=0; i<4; i++) {
        opt[i+4*j+b*64] = gen[i+4*j+b*64];
      }
    }
  }
}

void generated::kernels::AderDG::nonlinear::converter::PSi_optimised2generic(const double* const opt, double* gen) {
  for(int b=0; b<5; b++) {
    for(int j=0; j<16; j++) {
      #pragma omp simd aligned(gen,opt:16)
      for(int i=0; i<4; i++) {
        gen[i+4*j+b*64] = opt[i+4*j+b*64];
      }
    }
  }
}

void generated::kernels::AderDG::nonlinear::converter::PSi_generic2optimised(const double* const gen, double* opt) {
  for(int b=0; b<5; b++) {
    for(int j=0; j<16; j++) {
      #pragma omp simd aligned(gen,opt:16)
      for(int i=0; i<4; i++) {
        opt[i+4*j+b*64] = gen[i+4*j+b*64];
      }
    }
  }
}

void generated::kernels::AderDG::nonlinear::converter::Lduh_optimised2generic(const double* const opt, double* gen) {
  for(int zyx=0; zyx < 16; zyx++){
    #pragma omp simd aligned(gen,opt:16)
    for(int n=0; n<4;n++){
      gen[zyx*4+n] = opt[zyx*4+n];
    }
  }
}

void generated::kernels::AderDG::nonlinear::converter::Lduh_generic2optimised(const double* const gen, double* opt) {
  for(int zyx=0; zyx < 16; zyx++){
    #pragma omp simd aligned(gen,opt:16)
    for(int n=0; n<4;n++){
      opt[zyx*4+n] = gen[zyx*4+n];
    }
  }
}