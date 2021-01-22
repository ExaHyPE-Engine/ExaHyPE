# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet



class CreateRegularGrid(ActionSet):
  def __init__(self,maximum_cell_width):
    """
      maximum_cell_width
    """
    self.d = {}
    self.d[ "H_MAX" ]     = str(maximum_cell_width)


  def get_constructor_body(self):
    return ""

    
  def get_destructor_body(self):
    return ""


  __Template_GridControlEvents = """
  std::vector< peano4::grid::GridControlEvent > result;
  peano4::grid::GridControlEvent newEntry;
  newEntry.setRefinementControl( peano4::grid::GridControlEvent::RefinementControl::Refine );
  newEntry.setOffset(tarch::la::Vector<Dimensions,double>( -std::numeric_limits<double>::max()/2.0/3.0 ));
  newEntry.setWidth(tarch::la::Vector<Dimensions,double>(  std::numeric_limits<double>::max()/2.0 ));
  newEntry.setH(tarch::la::Vector<Dimensions,double>( {H_MAX} ));
  result.push_back(newEntry);
  return result;
"""


  def get_body_of_getGridControlEvents(self):
    return self.__Template_GridControlEvents.format(**self.d)


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_body_of_operation(self,operation_name):
    result = "\n"
    return result


  def get_attributes(self):
    return ""


  def get_includes(self):
    return ""

