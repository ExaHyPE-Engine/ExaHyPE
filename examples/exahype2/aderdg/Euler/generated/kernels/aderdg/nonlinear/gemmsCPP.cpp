
#include "./gemmsCPP.h"
#include "./Kernels.h" //for the libxsmm flop counter

#define NDEBUG // remove the libxsmm_num_total_flops counter
// amrRoutines gemms
#include "./asm_amrRoutines.c"

// fusedSpaceTimePredictorVolumeIntegral gemms
#include "./asm_fstpvi.c"
