# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2

import peano4.datamodel


def create_cell_marker(name):
    """Creates the double-buffered cell metadata used by the tree analysis."""
    cell_marker = peano4.datamodel.MetaData(name)

    cell_marker.add_attribute(peano4.datamodel.Integer("newNumberOfParticles"))
    cell_marker.add_attribute(peano4.datamodel.Integer("numberOfParticles"))
    cell_marker.add_attribute(peano4.datamodel.Boolean("newParentOfRefinedCell"))
    cell_marker.add_attribute(peano4.datamodel.Boolean("parentOfRefinedCell"))

    cell_marker.merge_method_definition = """
  switch (context) {
    case ::peano4::stacks::SendReceiveContext::BoundaryExchange:
      assertion(false, "Cell statistics have no boundary exchange.");
      break;
    case ::peano4::stacks::SendReceiveContext::MultiscaleExchange:
      setNewNumberOfParticles(
        getNewNumberOfParticles() + neighbour.getNewNumberOfParticles()
      );
      setNumberOfParticles(
        getNumberOfParticles() + neighbour.getNumberOfParticles()
      );
      setNewParentOfRefinedCell(
        getNewParentOfRefinedCell() or neighbour.getNewParentOfRefinedCell()
      );
      setParentOfRefinedCell(
        getParentOfRefinedCell() or neighbour.getParentOfRefinedCell()
      );
      break;
    case ::peano4::stacks::SendReceiveContext::ForkDomain:
      assertion(false, "Cell statistics have no fork exchange.");
      break;
    case ::peano4::stacks::SendReceiveContext::JoinDomain:
      setNewNumberOfParticles(
        getNewNumberOfParticles() + neighbour.getNewNumberOfParticles()
      );
      setNumberOfParticles(
        getNumberOfParticles() + neighbour.getNumberOfParticles()
      );
      setNewParentOfRefinedCell(
        getNewParentOfRefinedCell() or neighbour.getNewParentOfRefinedCell()
      );
      setParentOfRefinedCell(
        getParentOfRefinedCell() or neighbour.getParentOfRefinedCell()
      );
      break;
    case ::peano4::stacks::SendReceiveContext::PeriodicBoundaryDataSwap:
      assertion(false, "Cell statistics have no periodic boundary swap.");
      break;
  }
"""
    cell_marker.send_condition = "false"
    cell_marker.receive_and_merge_condition = "false"

    return cell_marker


class ParticleTreeAnalysis(ActionSet):
    """Aggregates particle counts and refinement state from leaves to the root.

    A cell count includes particles on its adjacent vertices and all descendant
    cells. ``parentOfRefinedCell`` records whether any direct child is refined;
    ``ParticleAMR`` uses it to limit coarsening to one level per sweep. Add
    ``cell_marker`` to the project data model and every step using this action.
    Passing an existing marker lets multiple analyses share that metadata.
    """

    def __init__(self, particle_set, cell_marker=None):
        super().__init__(descend_invocation_order=1)
        if cell_marker is not None:
            self.cell_marker = cell_marker
            self._cell_data_name = cell_marker.name
        else:
            self._cell_data_name = particle_set.name + "CellStatistics"
            self.cell_marker = create_cell_marker(self._cell_data_name)

        self._dictionary = {
            "CELL_DATA_NAME": self._cell_data_name,
            "PARTICLE_SET_NAME": particle_set.name,
            "PARTICLE_NAME": particle_set.particle_model.name,
        }

    __Template_TouchCellFirstTime = jinja2.Template(
        """
  fineGridCell{{CELL_DATA_NAME}}.setNumberOfParticles(
    fineGridCell{{CELL_DATA_NAME}}.getNewNumberOfParticles()
  );
  fineGridCell{{CELL_DATA_NAME}}.setParentOfRefinedCell(
    fineGridCell{{CELL_DATA_NAME}}.getNewParentOfRefinedCell()
  );
  fineGridCell{{CELL_DATA_NAME}}.setNewNumberOfParticles(0);
  fineGridCell{{CELL_DATA_NAME}}.setNewParentOfRefinedCell(false);
"""
    )

    __Template_CreateCell = jinja2.Template(
        """
  fineGridCell{{CELL_DATA_NAME}}.setNumberOfParticles(
    std::numeric_limits<int>::max()
  );
  fineGridCell{{CELL_DATA_NAME}}.setNewNumberOfParticles(0);
  fineGridCell{{CELL_DATA_NAME}}.setParentOfRefinedCell(true);
  fineGridCell{{CELL_DATA_NAME}}.setNewParentOfRefinedCell(true);
"""
    )

    __Template_TouchCellLastTime = jinja2.Template(
        """
  int numberOfAdjacentParticles = 0;
  for (int vertex = 0; vertex < TWO_POWER_D; ++vertex) {
    numberOfAdjacentParticles +=
      fineGridVertices{{PARTICLE_SET_NAME}}(vertex).size();
  }

  fineGridCell{{CELL_DATA_NAME}}.setNewNumberOfParticles(
    fineGridCell{{CELL_DATA_NAME}}.getNewNumberOfParticles()
    + numberOfAdjacentParticles
  );

  coarseGridCell{{CELL_DATA_NAME}}.setNewNumberOfParticles(
    coarseGridCell{{CELL_DATA_NAME}}.getNewNumberOfParticles()
    + fineGridCell{{CELL_DATA_NAME}}.getNewNumberOfParticles()
  );

  if (marker.willBeRefined()) {
    coarseGridCell{{CELL_DATA_NAME}}.setNewParentOfRefinedCell(true);
  }
"""
    )

    def get_constructor_body(self):
        return ""

    def get_destructor_body(self):
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector<::peano4::grid::GridControlEvent>();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    def get_attributes(self):
        return ""

    __Template_Includes = jinja2.Template(
        """
#include "vertexdata/{{PARTICLE_SET_NAME}}.h"
#include "globaldata/{{PARTICLE_NAME}}.h"
"""
    )

    def get_includes(self):
        return self.__Template_Includes.render(**self._dictionary)

    def get_body_of_operation(self, operation_name):
        result = ""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = self.__Template_TouchCellFirstTime.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            result = self.__Template_TouchCellLastTime.render(**self._dictionary)
        elif operation_name == ActionSet.OPERATION_CREATE_CELL:
            result = self.__Template_CreateCell.render(**self._dictionary)
        return result
