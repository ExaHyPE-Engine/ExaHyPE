#include "SolverRepository.h"



{% for N in NAMESPACE %}
namespace {{N}} {
{% endfor %}
  namespace repositories {
    {% for SOLVER in SOLVER_INSTANCES %}
    {{SOLVER[0]}}  {{SOLVER[1]}};
    {% endfor %}
    PetscData instanceOfPetsc;

  }
{% for N in NAMESPACE %}
}
{% endfor %}


void {{NAMESPACE | join("::")}}::repositories::computeLocalToGlobalMapsForAllSolvers() {
  {% for SOLVER in SOLVER_INSTANCES %}
  {% if SOLVER_INSTANCES[0][0]=="HDG" %}
    {{SOLVER[1]}}.getFaceLocalToGlobalMap().computeLocalToGlobalMap();
    {{SOLVER[1]}}.getCellLocalToGlobalMap().computeLocalToGlobalMap();
  {% else %}
    {{SOLVER[1]}}.getLocalToGlobalMap().computeLocalToGlobalMap();
  {% endif %}
  {% endfor %}
}


int {{NAMESPACE | join("::")}}::repositories::getTotalNumberOfDegreesOfFreedom() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  {% if SOLVER_INSTANCES[0][0]=="HDG" %}
  // result += {{SOLVER[1]}}.getFaceLocalToGlobalMap().getTotalNumberOfIndices();
  result += {{SOLVER[1]}}.getCellLocalToGlobalMap().getTotalNumberOfIndices();
  {% else %}
  result += {{SOLVER[1]}}.getLocalToGlobalMap().getTotalNumberOfIndices();
  {% endif %}
  {% endfor %}
  return result;
}

{% if SOLVER_INSTANCES[0][0]=="HDG" %}

int {{NAMESPACE | join("::")}}::repositories::getTotalFaceDofs() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getFaceLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getTotalCellDofs() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getCellLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getTotalVertexDofs() {
  int result = 0;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getVertexLocalToGlobalMap().getTotalNumberOfIndices();
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getGlobalCellIndex(std::pair<int,int> localIndex) {
  int result;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getCellLocalToGlobalMap().getGlobalIndex(localIndex);
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getGlobalFaceIndex(std::pair<int,int> localIndex) {
  int result;
  result += getTotalCellDofs();
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getFaceLocalToGlobalMap().getGlobalIndex(localIndex);
  {% endfor %}
  return result;
}

int {{NAMESPACE | join("::")}}::repositories::getGlobalVertexIndex(std::pair<int,int> localIndex) {
  int result;
  {% for SOLVER in SOLVER_INSTANCES %}
  result += {{SOLVER[1]}}.getVertexLocalToGlobalMap().getGlobalIndex(localIndex);
  {% endfor %}
  return result;
}

/*
overload to actually perform polynomial eval
*/
double {{NAMESPACE | join("::")}}::repositories::evalPoly( const std::vector<double>& coeffs, const double& quadPoint ){
  double output = 0;
  double x      = 1;
  //coeffs are ordered from highest degree to lowest degree,
  //so iterate from back to front
  for (auto iter=coeffs.crbegin(); iter!=coeffs.crend(); iter++){
    output += (*iter) * x;
    x *= quadPoint;
  }
  return output;
}

/*
overload to:
  - convert index into a tuple that describes position in the cell
  - evaluate the 1d basis at each of the points
  - take the product

  ie we promote a 1d basis to an n-dimensional basis by evaluating 
  our basis polynomial at (x,y,z,...) and taking the product
  poly(x)*poly(y)*poly(z)*...
*/
double {{NAMESPACE | join("::")}}::repositories::evalPoly( const int& index, const std::vector<double>& quadPoint ){
  //convert index to pair of indices
  const std::vector<int> indices = convertDofIndexToXYCoordinate(index);
  double output = 1;

  /*
  let our dof   indices be ordered [d0, d1, ...]
  and our point indices be ordered [p0, p1, ...]

  then we actually want to return 

  eval(d0, p0) * eval(d1, p1) * ...
  */

  auto indexIter = indices.cbegin();
  auto pointIter = quadPoint.cbegin();

  for (; indexIter!=indices.end(), pointIter!=quadPoint.end(); indexIter++, pointIter++)
    output *= evalPoly( polyCoeffs[ *indexIter ], *pointIter );

  return output;
}


const std::vector<int> {{NAMESPACE | join("::")}}::repositories::convertDofIndexToXYCoordinate(const int& dofIndex){
  std::vector<int> output;
  
  //get x coord by taking modulo with polynomialDegree + 1
  const int x = dofIndex % (polynomialDegree + 1);
  output.push_back(x);

  //take off the x coordinate, divide by (polynomialDegree + 1) to determine
  //how many blocks north we are
  const int y = dofIndex / (polynomialDegree + 1);
  output.push_back(y);

  return output;
}

double {{NAMESPACE | join("::")}}::repositories::evalDeriv( const int& dofIndex, const int& dimension, const std::vector<double>& quadPoint ){
  if (dimension >= Dimensions) throw std::runtime_error("taking derivative in wrong dimension!");

  //convert index to pair of indices
  const std::vector<int> indices = convertDofIndexToXYCoordinate(dofIndex);
  double output = 1;

  for (int iter = 0; iter!=indices.size(); iter++){
    //we want to eval deriv, but only in one dimension
    output *= (iter == dimension)
      ? evalDeriv( polyCoeffs[ indices[iter] ], quadPoint[iter] )
      : evalPoly(  polyCoeffs[ indices[iter] ], quadPoint[iter] );
  }

  return output;
}

double {{NAMESPACE | join("::")}}::repositories::evalDeriv( const std::vector<double>& coeffs, const double& quadPoint ){
  double output = 0;
  double x      = 1;
  double derivFactor = 1; //factor to account for falling powers of x

  for (auto iter=coeffs.crbegin()+1; iter!=coeffs.crend(); iter++){
    output += (*iter) * x * derivFactor;
    x *= quadPoint; derivFactor++;
  }
  return output;
}

tarch::la::Vector<Dimensions,double> {{NAMESPACE | join("::")}}::repositories::convertQuadPointToRealLocation(
  const int& quadIndex,
  const tarch::la::Vector<Dimensions,double>& cellCentre,
  const tarch::la::Vector<Dimensions,double>& cellH
){
  const std::vector<int> indicesXY = convertDofIndexToXYCoordinate(quadIndex);
  tarch::la::Vector<Dimensions,double> output;

  for (int i=0; i<Dimensions; i++){
    //init output to be bottom left corner of cell
    output[i] =  cellCentre[i] - cellH[i] / 2;
    
    //indicesXY indicates how many jumps of h / polynomial degree we need to do
    output[i] += indicesXY[i] * cellH[i] / polynomialDegree;
  }
  return output;  
}


{% endif %}
