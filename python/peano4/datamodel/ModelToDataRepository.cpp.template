#include "DataRepository.h"


{DATA_CONTAINER_INSTANTIATION}


void {FULL_QUALIFIED_CLASSNAME}::initDatatypes() {{
  #ifdef Parallel
  {MPI_DATAYPE_INITIALISATION}
  #endif
}}


void {FULL_QUALIFIED_CLASSNAME}::shutdownDatatypes() {{

  {DATA_CONTAINER_CLEARS}

  #ifdef Parallel
  {MPI_DATAYPE_SHUTDOWN}
  #endif
}}
