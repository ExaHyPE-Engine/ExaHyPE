# This file is part of the Peano's PETSc extension. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class AssemblePETSCMatrixWithQuadratures(ActionSet):
  ## add templates
  
  templateTouchCellFirstTime="""
  /*
  we need to create a matrix which we will pass to petsc.

  SIDE NOTE - WE WILL HAVE (FOR NOW) THE SAME VALUES IN EACH CELL (WE DO THE EXACT SAME CALCULATION)
  but we do it on each cell, rather than just reading off a value
  */
  tarch::la::Matrix<{{CELL_CARDINALITY}},{{CELL_CARDINALITY}},double> localMatrix;

  /*
  need a two-level loop - we follow the code in main.py (at time of writing, line 52) here:
  https://bitbucket.org/em459/hybridisableaderdg/src/prototype-development/src/prototype/main.py
  */
  if (not marker.willBeRefined())
  {

    for (int i=0; i<{{CELL_CARDINALITY}}; i++){
      for (int j=0; j<{{CELL_CARDINALITY}}; j++){
        //third level - loop over all the quad points and weights
        int p = 0; int w = 0;
        for (; p<repositories::quadWeights.size(); p++, w++){
          const std::vector<double> localQuadPoint  = repositories::quadPoints[p];
          const double              localQuadWeight = repositories::quadWeights[w];
          double gradDotGrad = repositories::evalDeriv( i, 0, localQuadPoint ) * repositories::evalDeriv( j, 0, localQuadPoint ) * marker.h()[0] / marker.h()[1]
                             + repositories::evalDeriv( i, 1, localQuadPoint ) * repositories::evalDeriv( j, 1, localQuadPoint ) * marker.h()[1] / marker.h()[0];
          localMatrix(i,j) += - localQuadWeight * gradDotGrad;
        }
      }
    }
    //we can send to petsc now.

    //get global index
    std::pair<int,int> localIndex( _spacetreeId, fineGridCell{{SOLVER_NAME}}.getNumber() );
    int globalIndex =  repositories::getGlobalCellIndex( localIndex );

    //construct the list of row indices we want to use:
    std::vector<int> rowIndices({{CELL_CARDINALITY}});
    //fill this vector up, starting with our global index
    std::iota(rowIndices.begin(), rowIndices.end(), globalIndex);

    //repeat for the colIndices
    std::vector<int> colIndices({{CELL_CARDINALITY}});
    //fill this vector up, starting with our global index
    std::iota(colIndices.begin(), colIndices.end(), globalIndex);

    //send to petsc
    repositories::instanceOfPetsc.insertLocalMatrix( rowIndices, colIndices, localMatrix );

    //do the same for rhs vector
    repositories::instanceOfPetsc.insertRHS( fineGridCell{{SOLVER_NAME}}.getRhs(), globalIndex );

    //insert zeroes into solution vectors
    repositories::instanceOfPetsc.insertX( fineGridCell{{SOLVER_NAME}}.getValue(), globalIndex );

  }
  """

  def __init__(self, solver):
    super( AssemblePETSCMatrixWithQuadratures, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["CELL_CARDINALITY"]   = solver.number_of_cell_unknowns

  def get_constructor_body(self):
    return f"""
    _spacetreeId = treeNumber;
"""


  def get_body_of_operation(self, operation_name):
    """!
    for now, only touch cell first time
    """
    ## add touchcellfirsttime
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTouchCellFirstTime).render(**self.d)
      pass 

    return result

  def get_action_set_name(self):
    """!
    
    Configure name of generated C++ action set
    
    This action set will end up in the directory observers with a name that
    reflects how the observer (initialisation) is mapped onto this action 
    set. The name pattern is ObserverName2ActionSetIdentifier where this
    routine co-determines the ActionSetIdentifier. We make is reflect the
    Python class name.
     
    """
    return __name__.replace(".py", "").replace(".", "_")
  
  def user_should_modify_template(self):
    """!
    
    The action set that Peano will generate that corresponds to this class
    should not be modified by users and can safely be overwritten every time
    we run the Python toolkit.
    
    """
    return False

  def get_includes(self):
    """!
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
#include <numeric> //for std::iota
"""
  def get_attributes(self):
    """!
    """

    return f"""
  int _spacetreeId;
"""


class AssemblePETSCMatrixForMixed(ActionSet):
  ## add templates
  
  templateTCFT = """
  if (not marker.willBeRefined())
  {
    //send precomputed cell-cell matrices to petsc!

    //global index
    std::pair<int,int> localIndex( _spacetreeId, fineGridCell{{SOLVER_NAME}}.getNumber() );
    int globalIndex =  repositories::getGlobalCellIndex( localIndex );

    //construct the list of row indices we want to use:
    int rowIndices[{{CELL_CARDINALITY}}];
    //fill this vector up, starting with our global index
    std::iota(rowIndices, rowIndices + {{CELL_CARDINALITY}}, globalIndex);

    //repeat for the colIndices
    int colIndices[{{CELL_CARDINALITY}}];
    //fill this vector up, starting with our global index
    std::iota(colIndices, colIndices + {{CELL_CARDINALITY}}, globalIndex);
    
    //we want to scale the matrix down
    //make a copy
    const int rows = repositories::{{SOLVER_INSTANCE}}._cellCellMassMatrix.rows();
    const int cols = repositories::{{SOLVER_INSTANCE}}._cellCellMassMatrix.cols();
    const int size = rows * cols;
    double tempMatrix[ rows*cols ];

    //scale mass matrix down by h**2
    std::transform( repositories::{{SOLVER_INSTANCE}}._cellCellMassMatrix.data(), repositories::{{SOLVER_INSTANCE}}._cellCellMassMatrix.data() + size,
                    repositories::{{SOLVER_INSTANCE}}._cellCellSystemMatrix.data(), // get data from both matrices, combine them together in the lambda
                    tempMatrix,                                                     // write result to temp array
                    [&](double m, double s){return m*marker.h()[0]*marker.h()[1] + s*marker.h()[0];} ); //scale mass matrix by h**2, system matrix by h, sum the result

    //send to petsc
    repositories::instanceOfPetsc.insertLocalMatrix( rows, rowIndices, cols, colIndices, tempMatrix );
  }
  """

  templateTFFT="""
  if (not marker.willBeRefined())
  {
    //send pre-computed face-face glue matrix to petsc

    //global index
    std::pair<int,int> localIndex( _spacetreeId, fineGridFace{{SOLVER_NAME}}.getNumber() );
    int globalIndex =  repositories::getGlobalFaceIndex( localIndex );

    //construct the list of row indices we want to use:
    int rowIndices[{{FACE_CARDINALITY}}];
    //fill this vector up, starting with our global index
    std::iota(rowIndices, rowIndices + {{FACE_CARDINALITY}}, globalIndex);

    //repeat for the colIndices
    int colIndices[{{FACE_CARDINALITY}}];
    //fill this vector up, starting with our global index
    std::iota(colIndices, colIndices + {{FACE_CARDINALITY}}, globalIndex);

    const int rows = repositories::{{SOLVER_INSTANCE}}._faceFaceMatrix.rows();
    const int cols = repositories::{{SOLVER_INSTANCE}}._faceFaceMatrix.cols();

    //finally, send to petsc
    repositories::instanceOfPetsc.insertLocalMatrix( rows, rowIndices, cols, colIndices, repositories::{{SOLVER_INSTANCE}}._faceFaceMatrix.data() );
  }
"""

  def __init__(self, solver):
    super( AssemblePETSCMatrixForMixed, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["CELL_CARDINALITY"]   = solver.number_of_matrix_entries_per_cell
    self.d["FACE_CARDINALITY"]   = solver.number_of_matrix_entries_per_face

  def get_constructor_body(self):
    return f"""
    _spacetreeId = treeNumber;
"""


  def get_body_of_operation(self, operation_name):
    """!
    for now, only touch cell first time
    """
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.templateTCFT).render(**self.d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.templateTFFT).render(**self.d)
      pass

    return result

  def get_action_set_name(self):
    """!
    
    Configure name of generated C++ action set
    
    This action set will end up in the directory observers with a name that
    reflects how the observer (initialisation) is mapped onto this action 
    set. The name pattern is ObserverName2ActionSetIdentifier where this
    routine co-determines the ActionSetIdentifier. We make is reflect the
    Python class name.
     
    """
    return __name__.replace(".py", "").replace(".", "_")
  
  def user_should_modify_template(self):
    """!
    
    The action set that Peano will generate that corresponds to this class
    should not be modified by users and can safely be overwritten every time
    we run the Python toolkit.
    
    """
    return False

  def get_includes(self):
    """!
   
Consult petsc.Project for details
    
"""    
    return """
#include "repositories/SolverRepository.h"
#include <numeric>   //for std::iota
#include <algorithm> //for std::transform
"""
  def get_attributes(self):
    """!
    """

    return f"""
  int _spacetreeId;
"""
