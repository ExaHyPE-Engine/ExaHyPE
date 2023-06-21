#ifndef EXASEIS_CONTEXT_CURVILINEAR_PML_HEADER
#define EXASEIS_CONTEXT_CURVILINEAR_PML_HEADER

// #include "kernels/GaussLegendreBasis.h"
#include "ContextCurvilinear.h"
#include "exahype2/aderdg/kernels/Utils/KernelUtils.h"

template <class Shortcuts, int basisSize>
class ContextCurvilinearPML: public ContextCurvilinear<Shortcuts, basisSize> {

public:
  ContextCurvilinearPML(
    std::string&                            scenario_string,
    std::string&                            topography_string,
    DomainInformation*                      a_domain_info,
    SolverInformationADERDG<basisSize - 1>* a_solver_info,
    int                                     pml_width
  ):
    ContextCurvilinear<Shortcuts, basisSize>(scenario_string, topography_string, a_domain_info, a_solver_info) {
    this->topography->set_pml_width(pml_width);
  }
};

#endif
