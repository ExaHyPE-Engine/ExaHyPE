#include "Vector.h"


#if !defined(OpenMPGPUOffloading)

template
struct tarch::la::Vector<2,double>;

template
struct tarch::la::Vector<3,double>;

#endif


