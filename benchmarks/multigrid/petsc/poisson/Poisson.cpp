#include "Poisson.h"


benchmarks::multigrid::petsc::poisson::Poisson::Poisson() {
  // temporary fix - can probably remove this after regenerating the code
  std::cout << "reserving\n\n";
  _valueAndRhs.reserve(16);
  std::cout << "size is " << _valueAndRhs.size() << "\n\n";

}


benchmarks::multigrid::petsc::poisson::Poisson::~Poisson() {
  // @todo add your stuff here
}


void benchmarks::multigrid::petsc::poisson::Poisson::init(
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  
  double&                                       value,
  double&                                       rhs
  
) {
  // @todo add your stuff here
  static int count;

  //i'm pretty sure these values are being hardcoded somewhere nearby
  std::cout << "calling init. count = " << count << "\n";
  count++;
  std::cout << "x[0], x[1] = " << x[0] << " " << x[1] << "\n";
  std::cout << "h[0], h[1] = " << h[0] << " " << h[1] << "\n";
  std::cout << "value, rhs = " << value << " " << rhs << "\n"; //std::numeric_limts<double>::min()

  _valueAndRhs.push_back( std::make_pair( value, rhs ) );
  std::cout << "size is " << _valueAndRhs.size() << "\n\n";

}

std::vector< std::pair<double, double> > benchmarks::multigrid::petsc::poisson::Poisson::getValuesAndRhs() const{
  return _valueAndRhs;
}

