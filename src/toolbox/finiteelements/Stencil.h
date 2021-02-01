// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TOOLBOX_FINITE_ELEMENTS_STENCIL_H_
#define _TOOLBOX_FINITE_ELEMENTS_STENCIL_H_

#include "peano4/utils/Globals.h"

#include "tarch/la/Vector.h"
#include "tarch/la/Matrix.h"

#include <complex>


namespace toolbox {
    namespace finiteelements {
      /**
       * Element-wise Matrix
       */
      typedef tarch::la::Matrix<TwoPowerD,TwoPowerD, double> ElementWiseAssemblyMatrix;

      typedef tarch::la::Matrix<TwoPowerD,TwoPowerD, std::complex<double> > ComplexElementWiseAssemblyMatrix;

      typedef tarch::la::Matrix<TwoPowerD,TwoPowerD, double> InterGridTransferMatrix;

      typedef tarch::la::Matrix<TwoPowerD/2,TwoPowerD/2, double> ElementWiseAssemblyMatrixOnSpaceTimeGrid;

      /**
       * Stencil
       */
      typedef tarch::la::Vector<ThreePowerD,double>  Stencil;
      typedef tarch::la::Vector<ThreePowerD,std::complex<double> >  ComplexStencil;

      /**
       * Vectors.
       */
      typedef tarch::la::Vector<ThreePowerD * TwoPowerD,double>  VectorOfStencils;
      typedef tarch::la::Vector<ThreePowerD * TwoPowerD,std::complex<double> >  VectorOfComplexStencils;

      /**
       * Vector of Element (Lexicographic Ordering of Unknowns)
       */
      typedef tarch::la::Vector<TwoPowerD,double> ElementWiseVector;

      int getStencilEntryLinearisedIndex( const tarch::la::Vector<Dimensions,int>  stencilEntry);


    }
}

#endif
