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
public:
  /** Tree id of the observer prototype. */
  static constexpr int TreePrototype = -1;

  {{CLASSNAME}}(int treeId = TreePrototype);

  GridTraversalObserver* clone(int treeId) override;

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

  void loadNodeData(const ::peano4::grid::GridTraversalEvent& event) override;
  void storeNodeData(const ::peano4::grid::GridTraversalEvent& event) override;

  void receiveNodeData(
    ::peano4::grid::GridTraversalObserver::NodeData nodeData,
    int                                             position,
    int                                             fromStack,
    ::peano4::stacks::SendReceiveContext            context,
    const ::peano4::grid::GridTraversalEvent&       event
  ) override;

private:
  const int _treeId;

  {{ATTRIBUTES}}
};
