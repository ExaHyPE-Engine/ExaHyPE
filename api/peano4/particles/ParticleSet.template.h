// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "globaldata/{{PARTICLE_TYPE}}.h"

#include "peano4/dataview/VertexMarker.h"
#include "peano4/grid/SendReceiveContext.h"
#include "peano4/grid/Stack.h"
#include "peano4/grid/StackExchange.h"

#include "peano4/particles/ParticleSet.h"

namespace {{NAMESPACE | join("::")}} {
  class {{CLASSNAME}};
} // namespace {{NAMESPACE | join("::")}}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} final :
  public peano4::particles::ParticleSet<::{{PARTICLE_FULL_QUALIFIED_TYPE}}> {
public:
  using Base = peano4::particles::ParticleSet<
    ::{{PARTICLE_FULL_QUALIFIED_TYPE}}
  >;
  using Container = std::vector<
    ::{{PARTICLE_FULL_QUALIFIED_TYPE}}*
  >;
  using DoFType = Base::DoFType;
  using value_type = Container::value_type;
  using iterator = Container::iterator;
  using const_iterator = Container::const_iterator;

  static constexpr int Cardinality = 1;

  /**
   * The set holds pointers to its particles, so it deletes them itself when the
   * rank overwrites its copy of the vertex with a subtree task's copy.
   */
  static constexpr bool RebuildsConsolidationSlotByMerge{true};

  explicit {{CLASSNAME}}() = default;
  {{CLASSNAME}}(const {{CLASSNAME}}& other);
  {{CLASSNAME}}({{CLASSNAME}}&& other) noexcept;
  {{CLASSNAME}}& operator=(const {{CLASSNAME}}& other);
  {{CLASSNAME}}& operator=({{CLASSNAME}}&& other) noexcept;
  ~{{CLASSNAME}}() noexcept;

  void merge(
    peano4::grid::SendReceiveContext      context,
    const {{CLASSNAME}}&                  neighbour,
    const peano4::dataview::VertexMarker& marker,
    int                                   treeId
  );
  void mergeWithParticle(
    DoFType*                              neighbourParticle,
    const peano4::dataview::VertexMarker& marker,
    int                                   treeId
  );
  bool send(
    const peano4::dataview::VertexMarker& marker,
    peano4::grid::SendReceiveContext      context
  );
  bool receiveAndMerge(
    const peano4::dataview::VertexMarker& marker,
    peano4::grid::SendReceiveContext      context
  );

  static bool load(const peano4::dataview::VertexMarker& marker);
  static bool store(const peano4::dataview::VertexMarker& marker);

  void particlesCanNotBeLiftedLocally();
  iterator particleCanNotBeLiftedLocally(iterator particle);
  void addParticle(DoFType* particle);
  iterator deleteParticle(iterator particle);
  void grabParticles({{CLASSNAME}}& sourceParticleSet);
  iterator grabParticle(iterator particle, {{CLASSNAME}}& sourceParticleSet);

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;

  int size() const;
  bool isValid() const;

  std::string toString() const;

  static void startSend(
    peano4::grid::Stack<{{CLASSNAME}}>&            stack,
    peano4::grid::StackExchangeJob<{{CLASSNAME}}>& job
  );
  static void startReceive(
    peano4::grid::Stack<{{CLASSNAME}}>&            stack,
    peano4::grid::StackExchangeJob<{{CLASSNAME}}>& job,
    int                                            numberOfElements
  );
  static bool tryToFinishSendOrReceive(
    peano4::grid::Stack<{{CLASSNAME}}>&            stack,
    peano4::grid::StackExchangeJob<{{CLASSNAME}}>& job
  );

private:
  void clone(const {{CLASSNAME}}& sourceParticleSet);
  void deleteParticlesForSlotRebuild(
    const peano4::dataview::VertexMarker& marker,
    int                                   treeId
  );
  void deleteParticles() noexcept;

  Container _container{};
};
