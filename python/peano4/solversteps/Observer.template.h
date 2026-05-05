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

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public ::peano4::grid::GridTraversalObserver {
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

  virtual GridTraversalObserver* clone(int spacetreeId) override;

  virtual std::vector<::peano4::grid::GridControlEvent> getGridControlEvents() const override;

  static void prepareTraversal();
  static void unprepareTraversal();

  virtual void beginTraversal(const tarch::la::Vector<DIMENSIONS,double>& x, const tarch::la::Vector<DIMENSIONS,double>& h) override;
  virtual void endTraversal(const tarch::la::Vector<DIMENSIONS,double>& x, const tarch::la::Vector<DIMENSIONS,double>& h) override;

  virtual void enterNode(const ::peano4::grid::GridTraversalEvent& event) override;
  virtual void leaveNode(const ::peano4::grid::GridTraversalEvent& event) override;

{%- if VERTICES %}
  virtual void loadVertexData(const ::peano4::grid::GridTraversalEvent& event) override;
  virtual void storeVertexData(const ::peano4::grid::GridTraversalEvent& event) override;
  virtual void sendVertexData(int position, int toStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) override;
  virtual void receiveAndMergeVertexData(int position, int fromStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) override;
{%- endif %}

{%- if FACES %}
  virtual void loadFaceData(const ::peano4::grid::GridTraversalEvent& event) override;
  virtual void storeFaceData(const ::peano4::grid::GridTraversalEvent& event) override;
  virtual void sendFaceData(int position, int toStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) override;
  virtual void receiveAndMergeFaceData(int position, int fromStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) override;
{%- endif %}

{%- if CELLS %}
  virtual void loadCellData(const ::peano4::grid::GridTraversalEvent& event) override;
  virtual void storeCellData(const ::peano4::grid::GridTraversalEvent& event) override;
  virtual void sendCellData(int toStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) override;
  virtual void receiveAndMergeCellData(int fromStack, ::peano4::stacks::SendReceiveContext context, const ::peano4::grid::GridTraversalEvent& event) override;
{%- endif %}

  virtual void exchangeAllHorizontalDataExchangeStacks(bool symmetricDataCardinality) override;
  virtual void exchangeAllPeriodicBoundaryDataStacks() override;

  virtual void streamDataFromSplittingTreeToNewTree(int newWorker) override;
  virtual void streamDataFromJoiningTreeToMasterTree(int masterId) override;

  virtual void finishAllOutstandingSendsAndReceives() override;

  virtual void deleteAllStacks() override;
};
