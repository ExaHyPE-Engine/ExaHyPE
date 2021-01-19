#include "Generic.h"

#include "tarch/logging/Log.h"
#include "tarch/multicore/multicore.h"

#include "../PatchUtils.h"

#include "peano4/utils/Loop.h"

#include "exahype2/NonCriticalAssertions.h"


std::string exahype2::aderdg::plotNode(
  double Q[],
  int    unknowns
) {
  std::string result = "(" + std::to_string(Q[0]);
  for (int i=1; i<unknowns; i++) result += "," + std::to_string(Q[i]);
  result += ")";
  return result;
};


void exahype2::aderdg::validatePatch(
  const double* __restrict__ UIn,
  int                        unknowns,
  int                        auxiliaryVariables,
  int                        order,
  const                      std::string& location
) {
  const int numberOfNodesPerAxisInPatch = order + 1;
  dfor (k,numberOfNodesPerAxisInPatch) {
    int index = peano4::utils::dLinearised(k,numberOfNodesPerAxisInPatch) * (unknowns+auxiliaryVariables);
    for (int i=0; i<unknowns+auxiliaryVariables; i++) {
      nonCriticalAssertion6( std::isfinite(UIn[index+i]), unknowns, auxiliaryVariables, order, k, i, location );
    }
  }
}

void exahype2::aderdg::validateSpacetimePatch(
  const double* __restrict__ QIn,
  int                        unknowns,
  int                        auxiliaryVariables,
  int                        order,
  const                      std::string& location
) {
  const int numberOfNodesPerAxisInPatch = order + 1;
  dfor (k,numberOfNodesPerAxisInPatch) {
    int index = peano4::utils::dLinearised(k,numberOfNodesPerAxisInPatch) * (unknowns+auxiliaryVariables);
    for (int i=0; i<numberOfNodesPerAxisInPatch*(unknowns+auxiliaryVariables); i++) {
      nonCriticalAssertion6( std::isfinite(QIn[index+i]), unknowns, auxiliaryVariables, order, k, i, location );
    }
  }
}
    
//#if defined(OpenMPGPUOffloading)
//#pragma omp declare target
//#endif
//void exahype2::aderdg::copyPatch(
//  const double* __restrict__ QIn,
//  double* __restrict__       QOut,
//  int                        unknowns,
//  int                        auxiliaryVariables,
//  int                        order
//) {
//  const int numberOfNodesPerAxisInPatch = order + 1;
//  dfor (k,numberOfNodesPerAxisInPatch) {
//    int index = peano4::utils::dLinearised(k,numberOfNodesPerAxisInPatch) * (unknowns+auxiliaryVariables);
//	  for (int i=0; i<unknowns+auxiliaryVariables; i++) {
//       QOut[ index + i ] = QIn[index + 1]; 
//	  }
//  }
//}
//#if defined(OpenMPGPUOffloading)
//#pragma omp end declare target
//#endif
