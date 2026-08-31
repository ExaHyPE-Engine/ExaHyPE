// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "globaldata/{{PARTICLE_TYPE}}.h"

#include <mpi.h>

#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/stacks/Stack.h"

#include "toolbox/particles/ParticleSet.h"

namespace {{NAMESPACE | join("::")}} {
  class {{CLASSNAME}};
} // namespace {{NAMESPACE | join("::")}}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}} final :
  public toolbox::particles::ParticleSet<::{{PARTICLE_FULL_QUALIFIED_TYPE}}> {
public:
  using Base = toolbox::particles::ParticleSet<
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

  void clear();
  void clone(const {{CLASSNAME}}& other);
  {{CLASSNAME}} createExchangeView();

  void merge(
    const peano4::stacks::SendReceiveContext context,
    const {{CLASSNAME}}& neighbour,
    const peano4::datamanagement::VertexMarker& marker,
    const int treeId
  );
  void mergeWithParticle(
    DoFType* const neighbourParticle,
    const peano4::datamanagement::VertexMarker& marker,
    const int treeId
  );
  void deleteParticlesForSlotRebuild(
    const peano4::datamanagement::VertexMarker& marker,
    const int treeId
  );
  bool send(
    const peano4::datamanagement::VertexMarker& marker,
    const peano4::stacks::SendReceiveContext context
  );
  bool receiveAndMerge(
    const peano4::datamanagement::VertexMarker& marker,
    const peano4::stacks::SendReceiveContext context
  );

  static bool load(const peano4::datamanagement::VertexMarker& marker);
  static bool store(const peano4::datamanagement::VertexMarker& marker);

  void particlesCanNotBeLiftedLocally();
  iterator particleCanNotBeLiftedLocally(iterator particle);
  void particlesCanNotBeDroppedLocallySoRollOverForNextMeshSweep(DoFType* const particle);
  void addParticle(DoFType* const particle);
  void deleteParticles() noexcept;
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
    peano4::stacks::Stack<{{CLASSNAME}}>& stack,
    const peano4::stacks::SendReceiveContext context,
    const MPI_Comm communicator
  );
  static void startReceive(
    peano4::stacks::Stack<{{CLASSNAME}}>& stack,
    const peano4::stacks::SendReceiveContext context,
    const MPI_Comm communicator,
    const int numberOfElements
  );
  static bool tryToFinishSendOrReceive(peano4::stacks::Stack<{{CLASSNAME}}>& stack);

  ~{{CLASSNAME}}() noexcept;

private:
  Container _container{};
};
