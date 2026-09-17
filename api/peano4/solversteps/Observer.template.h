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

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} final : public ::peano4::grid::GridTraversalObserver {
public:
  /** Tree id of the observer prototype. */
  static constexpr int TreePrototype = -1;

  explicit {{CLASSNAME}}(int treeId = TreePrototype);

  ::peano4::grid::GridTraversalObserver* clone(int treeId) override;

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

  void receiveNodeData(
    ::peano4::grid::GridTraversalObserver::NodeData nodeData,
    int                                             position,
    int                                             fromStack,
    ::peano4::grid::SendReceiveContext              context,
    const ::peano4::grid::GridTraversalEvent&       event
  ) override;

private:
  const int _treeId;

  /**
   * Stores entity pool slots for one cell on the active root-to-cell path.
   *
   * The preceding entry supplies coarse-grid slots to callbacks.
   */
  struct PathEntry final {
    std::array<int, TWO_POWER_D> vertexSlots{};
    std::array<int, TWO_TIMES_D> faceSlots{};
    std::array<int, 1>           cellSlot{};
  };

  std::vector<PathEntry> _activePath{};

  {{ATTRIBUTES}}
};
