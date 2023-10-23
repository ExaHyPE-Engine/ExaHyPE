'''
this file is for code which is no longer used
'''

from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2



'''

This class is somewhat deprecated. was put in place for demonstrating firedrake code, but we move
away from this now.

'''

class AssemblePETSCMatrixOnCellsAndFaces(ActionSet):
  ## add templates
  TemplateInitCell = """
  if (not marker.willBeRefined()){
    //init vector to collect the cell indices we use
    std::vector<int> cellDofIndices;

    //get global index
    std::pair<int,int> cellLocalIndex = std::make_pair(_spacetreeId, fineGridCell{{SOLVER_NAME}}.getNumber());
    int cellGlobalIndex = repositories::{{SOLVER_INSTANCE}}.getCellLocalToGlobalMap().getGlobalIndex(cellLocalIndex);

    //add this global index, plus each of its degrees of freedom, to the cellDofIndices
    for (int i=0; i<_cellDofs; i++)
      cellDofIndices.push_back( cellGlobalIndex + i ); //works because we reserved (eg) 27 indices per cell earlier
    
    /*
    enumerate the faces. for each face, we do the following:
    1. collect the global index of the face, add this to the list faceDofIndices
      - also add to the list each degree of freedom that lies on this face
    2. (have already done same for cell indices above)
    3. insert stencil for FF, CF, FC and CC using these indices
    */

    for (int face=0; face<2*Dimensions; face++){
      std::vector<int> faceDofIndices;

      //get global index
      std::pair<int,int> localIndex = std::make_pair(_spacetreeId, fineGridFaces{{SOLVER_NAME}}(face).getNumber());
      int globalIndex = repositories::{{SOLVER_INSTANCE}}.getFaceLocalToGlobalMap().getGlobalIndex(localIndex);

      //insert the faceDofs into our temp vector.
      for (int i=0; i<_faceDofs; i++)
        faceDofIndices.push_back( globalIndex + i );
      
      //insert the stencil for CF interactions
      //use face iterator to index our vector of stencils
      repositories::instanceOfPetsc.insertLocalMatrix( cellDofIndices, faceDofIndices, _stencilsCF[face] );

      //insert the stencil for FC interactions, only if we are not on the boundary
      if ( fineGridFaces{{SOLVER_NAME}}(face).getType() == facedata::{{SOLVER_NAME}}::Type::Interior ){
        repositories::instanceOfPetsc.insertLocalMatrix( faceDofIndices, cellDofIndices, _stencilsFC[face] );

        //also insert the FF stencil intended for the interior
        //there's only two, so we use 0 for boundary and 1 for interior
        repositories::instanceOfPetsc.insertLocalMatrix( faceDofIndices, faceDofIndices, _stencilsFF[1] );
      }

      else if ( fineGridFaces{{SOLVER_NAME}}(face).getType() == facedata::{{SOLVER_NAME}}::Type::Boundary ){
        //insert FF stencil intended for boundary
        repositories::instanceOfPetsc.insertLocalMatrix( faceDofIndices, faceDofIndices, _stencilsFF[0] );
      }

    }

    //before we exit, let's insert the stencil for cell-cell interactions
    repositories::instanceOfPetsc.insertLocalMatrix( cellDofIndices, cellDofIndices, _stencilCC );
  }
"""

  def __init__(self, solver):
    super( AssemblePETSCMatrixOnCellsAndFaces, self ).__init__()
    self.d = {}
    self.d["SOLVER_INSTANCE"]    = solver.instance_name()
    self.d["SOLVER_NAME"]        = solver.typename()
    self.d["CELL_CARDINALITY"]   = solver.number_of_cell_unknowns
    self.d["FACE_CARDINALITY"]   = solver.number_of_face_unknowns

    '''
    HANDLING THE STENCILS!
    '''
    
    #FIRST - HANDLE STENCIL FOR FACE-FACE INTERACTIONS
    #we have two stencils, and we insert them into a std::vector
    #the 0th one is for the boundary, and the 1st one is for the interior

    ##first, capture the dimensions of the stencil
    self.d["stencilRowsFF"] = len(solver.stencilsFF[0])
    self.d["stencilColsFF"] = len(solver.stencilsFF[0][0])
    numberOfCols = self.d["stencilColsFF"] #use this to insert a line break after every row
    self.d["STENCILSFF"] = []
    
    #pass in stencil as a list of lists
    #convert it to a flat list for passing as std::initializer_list
    for stencil in solver.stencilsFF:
      #flatten it
      stencil=[item for sublist in stencil for item in sublist]
      stencilAsString = "\n{" + str(stencil[0])
      for index, el in enumerate(stencil[1:]):
        stencilAsString += ", "
        if index > 0 and (index-1) % numberOfCols == 1:
          stencilAsString += "\n"
        stencilAsString += str(el)
      stencilAsString += "}"

      #add to list of stencils
      self.d["STENCILSFF"].append(stencilAsString)

    #NEXT - HANDLE STENCIL FOR CELL-FACE INTERACTIONS
    #this time we have a list of stencils which must be ordered

    ##first, capture the dimensions of the stencil
    self.d["stencilRowsCF"] = len(solver.stencilsCF[0])   #solver.stencilCF[0] is the 0th stencil itself
    self.d["stencilColsCF"] = len(solver.stencilsCF[0][0])#captures the number of columns 
    numberOfCols = self.d["stencilColsCF"]                #use this to insert a line break after every row
    self.d["STENCILSCF"] = []                             #init a list. we will insert the stencils into them.

    #pass in stencil as a list of lists
    #convert it to a flat list for passing as std::initializer_list
    for stencil in solver.stencilsCF:
      #flatten it
      stencil=[item for sublist in stencil for item in sublist]
      
      #init a string to capture the stencil, we will insert this into the dict at the end
      stencilAsString = "\n{" + str(stencil[0]) # add "{" to begin the intializer list
      for index, el in enumerate(stencil[1:]):
        stencilAsString += ", "
        if index > 0 and (index-1) % numberOfCols == 1:
          stencilAsString += "\n"
        stencilAsString += str(el)
      stencilAsString += "}"                    #end the initializer list

      #add to list of stencils 
      self.d["STENCILSCF"].append(stencilAsString)

    #NEXT - HANDLE STENCIL FOR FACE-CELL INTERACTIONS
    #this time we have a list of stencils which must be ordered

    ##first, capture the dimensions of the stencil
    self.d["stencilRowsFC"] = len(solver.stencilsFC[0])   #solver.stencilFC[0] is the 0th stencil itself
    self.d["stencilColsFC"] = len(solver.stencilsFC[0][0])#captures the number of columns 
    numberOfCols = self.d["stencilColsFC"]               #use this to insert a line break after every row
    self.d["STENCILSFC"] = []                            #init a list. we will insert the stencils into them

    #pass in stencil as a list of lists
    #convert it to a flat list for passing as std::initializer_list
    for stencil in solver.stencilsFC:
      stencil=[item for sublist in stencil for item in sublist]
      stencilAsString = "\n{" + str(stencil[0])
      for index, el in enumerate(stencil[1:]):
        stencilAsString += ", "
        if index > 0 and (index-1) % numberOfCols == 1:
          stencilAsString += "\n"
        stencilAsString += str(el)
      stencilAsString += "}"

      #add to list
      self.d["STENCILSFC"].append(stencilAsString)

    #FINALLY - HANDLE STENCIL FOR CELL-CELL INTERACTIONS

    ##first, capture the dimensions of the stencil
    self.d["stencilRowsCC"] = len(solver.stencilCC)
    self.d["stencilColsCC"] = len(solver.stencilCC[0])
    numberOfCols = len(solver.stencilCC[0]) #use this to insert a line break after every row

    #pass in stencil as a list of lists
    #convert it to a flat list for passing as std::initializer_list
    stencil=[item for sublist in solver.stencilCC for item in sublist]
    self.d["STENCILCC"] = "\n{" + str(stencil[0])
    for index, el in enumerate(stencil[1:]):
      self.d["STENCILCC"] += ", "
      if index > 0 and (index-1) % numberOfCols == 1:
        self.d["STENCILCC"] += "\n"
      self.d["STENCILCC"] += str(el)
    self.d["STENCILCC"] += "}"

  def get_constructor_body(self):
    return f"""
    _spacetreeId = treeNumber;
"""


  def get_body_of_operation(self, operation_name):
    """!
    for now, only touch face first time
    """
    ## add touchcellfirsttime

    ## add touch face first time
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = jinja2.Template(self.TemplateInitCell).render(**self.d)
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
#include "tarch/la/Matrix.h"
#include "repositories/SolverRepository.h"
"""
  def get_attributes(self):
    """!
    """

    #construct the output strings for stencilsCF and stencilsFC
    stencilsCF = '{' #start the std vector
    for stencil in self.d["STENCILSCF"]:
      stencilsCF += stencil
      stencilsCF += ",\n"
    stencilsCF += "}" #end the std vector
    #same again
    stencilsFC = '{' #start the std vector
    for stencil in self.d["STENCILSFC"]:
      stencilsFC += stencil
      stencilsFC += ",\n"
    stencilsFC += "}" #end the std vector

    #also for stencilsFF
    stencilsFF = '{' #start the std vector
    for stencil in self.d["STENCILSFF"]:
      stencilsFF += stencil
      stencilsFF += ",\n"
    stencilsFF += "}" #end the std vector


    return f"""
  int _spacetreeId;
  const int _faceDofs = {self.d["FACE_CARDINALITY"]};
  const int _cellDofs = {self.d["CELL_CARDINALITY"]};

  //we need a std::vector for these now, since we have mutiple
  const std::vector< tarch::la::Matrix<{self.d["stencilRowsFF"]},{self.d["stencilColsFF"]},double> > _stencilsFF = 
    {stencilsFF};

  //we need a std::vector for these now, since we have mutiple
  const std::vector< tarch::la::Matrix<{self.d["stencilRowsCF"]},{self.d["stencilColsCF"]},double> > _stencilsCF = 
    {stencilsCF};

  //we need a std::vector for these now, since we have mutiple
  const std::vector< tarch::la::Matrix<{self.d["stencilRowsFC"]},{self.d["stencilColsFC"]},double> > _stencilsFC = 
    {stencilsFC};

  const tarch::la::Matrix<{self.d["stencilRowsCC"]},{self.d["stencilColsCC"]},double> _stencilCC = {self.d["STENCILCC"]};
"""