// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace exahype2 {
  namespace dg {
    namespace rusanov {
      namespace omp {
        template <
          typename Solver,
          int      order,
          int      unknowns,
          int      auxiliaryVariables
        >
        void cellIntegral_patchwise_in_situ_GaussLegendre(
          int                                            targetDevice,
          ::exahype2::CellData&                          cellData,
          bool                                           evaluateFlux,
          bool                                           evaluateNonconservativeProduct,
          bool                                           evaluateSource,
          bool                                           evaluatePointSources
        ) {
          // @todo No docu yet
          // @todo Should refer to a generic OMP implementation and not the static variant
          ::exahype2::dg::cellIntegral_patchwise_in_situ_GaussLegendre<Solver,order,unknowns,auxiliaryVariables>(
            cellData,
            evaluateFlux,
            evaluateNonconservativeProduct,
            evaluateSource,
            evaluatePointSources
          );
        }
      }
    }
  }
}

