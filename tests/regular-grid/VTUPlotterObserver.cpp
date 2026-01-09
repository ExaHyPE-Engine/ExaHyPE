// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "VTUPlotterObserver.h"

#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/parallel/Node.h"
#include "peano4/parallel/SpacetreeSet.h"

std::unique_ptr<tarch::plotter::VTUFileWriter> tests::peano4::grid::VTUPlotterObserver::_fileWriter = nullptr;

tests::peano4::grid::VTUPlotterObserver::VTUPlotterObserver(int treeId):
  _spacetreeId(treeId) {

  _localSpacetreeId = ::peano4::parallel::Node::getInstance().getLocalTreeId(treeId);
}

void tests::peano4::grid::VTUPlotterObserver::prepareTraversal() {
  _fileWriter.reset();
  _fileWriter = std::make_unique<tarch::plotter::VTUFileWriter>(
    "",
    ::peano4::parallel::SpacetreeSet::getInstance().getLocalSpacetrees().size()
  );
}

void tests::peano4::grid::VTUPlotterObserver::unprepareTraversal() {
  assertion(_fileWriter != nullptr);
  assertion(_fileWriter->isOpen());

  _fileWriter->writeToFile("RegularGrid");
}

void tests::peano4::grid::VTUPlotterObserver::beginTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {
  _subdomainPlotter.reset();
  _subdomainPlotter = _fileWriter->createCellDataPlotter(_localSpacetreeId, "Subdomain", 1);

  _amrPlotter.reset();
  _amrPlotter = std::make_unique<tarch::plotter::AMRPlotter>(*_fileWriter, _localSpacetreeId, 0.8, 0.5);
}

void tests::peano4::grid::VTUPlotterObserver::endTraversal(
  const tarch::la::Vector<DIMENSIONS, double>& x,
  const tarch::la::Vector<DIMENSIONS, double>& h
) {}

void tests::peano4::grid::VTUPlotterObserver::enterNode(const ::peano4::grid::GridTraversalEvent& event) {
  if (event.getWillBeRefined() == 0 and event.getIsCellLocal()) {
    const auto& size            = event.getH();
    const auto& offset          = event.getX() - size * 0.5;
    const auto& refinementLevel = event.level;

#if DIMENSIONS == 2
    int cellIndex = _amrPlotter->plotCell2D(offset, size, refinementLevel);
#else
    int cellIndex = _amrPlotter->plotCell3D(offset, size, refinementLevel);
#endif

    _subdomainPlotter->plotCellData(cellIndex, _spacetreeId);
  }
}

void tests::peano4::grid::VTUPlotterObserver::leaveNode(const ::peano4::grid::GridTraversalEvent& event) {}

::peano4::grid::GridTraversalObserver* tests::peano4::grid::VTUPlotterObserver::clone(int spacetreeId) {
  return new VTUPlotterObserver(spacetreeId);
}

std::vector<::peano4::grid::GridControlEvent> tests::peano4::grid::VTUPlotterObserver::getGridControlEvents() const {
  return std::vector<::peano4::grid::GridControlEvent>();
}
