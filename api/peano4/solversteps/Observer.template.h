// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "repositories/DataRepository.h"

#include "peano4/Globals.h"
#include "peano4/grid/GridControlEvent.h"
#include "peano4/grid/GridTraversalEvent.h"
#include "peano4/grid/GridTraversalObserver.h"

#include "tarch/la/Vector.h"

{{INCLUDES}}

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};
{% for item in NAMESPACE -%}
}
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} final: public ::peano4::grid::GridTraversalObserver {
private:
  const int _spacetreeId;

  {{ATTRIBUTES}}

public:
  /**
   * If you pass this marker into the constructor, it implies that this
   * observer is used as prototype for the real observers which are created
   * per local spacetree.
   */
  static constexpr int SpacetreePrototype = -1;

  /**
   * @param spacetreeId -1 is the dummy, i.e. this observer is not
   *                    instantiated for a spacetree yet but serves as
   *                    blueprint for the real trees later on.
   */
  {{CLASSNAME}}(int spacetreeId = SpacetreePrototype);

  GridTraversalObserver* clone(int spacetreeId) override;

  std::vector<::peano4::grid::GridControlEvent> getGridControlEvents() const override;

  static void prepareTraversal();
  static void unprepareTraversal();

  void beginTraversal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) override;
  void endTraversal(
    const tarch::la::Vector<DIMENSIONS, double>& x,
    const tarch::la::Vector<DIMENSIONS, double>& h
  ) override;

  void enterNode(const ::peano4::grid::GridTraversalEvent& event) override;
  void leaveNode(const ::peano4::grid::GridTraversalEvent& event) override;

  void setLocalTrees() override;

  void loadNodeData(const ::peano4::grid::GridTraversalEvent& event) override;
  void storeNodeData(const ::peano4::grid::GridTraversalEvent& event) override;

  void sendNodeData(
    ::peano4::grid::GridTraversalObserver::NodeData nodeData,
    int                                             position,
    int                                             toStack,
    ::peano4::stacks::SendReceiveContext            context,
    const ::peano4::grid::GridTraversalEvent&       event,
    const ::peano4::grid::PeriodicStamp*            periodicStamp = nullptr
  ) override;
  void receiveNodeData(
    ::peano4::grid::GridTraversalObserver::NodeData nodeData,
    int                                             position,
    int                                             fromStack,
    ::peano4::stacks::SendReceiveContext            context,
    const ::peano4::grid::GridTraversalEvent&       event,
    const ::peano4::grid::PeriodicStamp*            periodicStamp = nullptr
  ) override;

  void exchangeAllHorizontalDataExchangeStacks() override;
  void purgePeriodicBoundaryDataStacks() override;
  void exchangeAllPeriodicBoundaryDataStacks() override;
  void exchangeCrossRankPeriodicData() override;

  void streamDataFromSplittingTreeToNewTree(int newWorker) override;
  void streamDataFromJoiningTreeToMasterTree(int masterId) override;

  void finishAllOutstandingSendsAndReceives() override;
};
