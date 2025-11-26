// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "globaldata/{{PARTICLE_TYPE}}.h"

#include "tarch/la/Vector.h"
#include "tarch/mpi/mpi.h"

#include "peano4/datamanagement/VertexMarker.h"
#include "peano4/grid/LoadStoreComputeFlag.h"
#include "peano4/stacks/Stack.h"
#include "peano4/utils/Globals.h"

#include "toolbox/particles/ParticleSet.h"

#include <{{STD_CONTAINER}}>

{% for item in NAMESPACE -%}
namespace {{ item }} {
{%- endfor %}
  class {{CLASSNAME}};
{% for item in NAMESPACE -%}
}
{%- endfor %}

class {{NAMESPACE | join("::")}}::{{CLASSNAME}}: public toolbox::particles::ParticleSet<{% for item in NAMESPACE -%}{% if not loop.last %}{{ item }}::{% endif %}{%- endfor %}globaldata::{{PARTICLE_TYPE}}> {
public:
  using Base      = toolbox::particles::ParticleSet<{% for item in NAMESPACE -%}{% if not loop.last %}{{ item }}::{% endif %}{%- endfor %}globaldata::{{PARTICLE_TYPE}}>;
  using Container = std::{{STD_CONTAINER}}<{% for item in NAMESPACE -%}{% if not loop.last %}{{ item }}::{% endif %}{%- endfor %}globaldata::{{PARTICLE_TYPE}}*>;
  using DoFType   = Base::DoFType;

  static constexpr int Cardinality = 1; // TODO: Remove

  typedef Container::value_type      value_type;
  typedef Container::iterator        iterator;
  typedef Container::const_iterator  const_iterator;

private:
  static tarch::logging::Log _log;

  Container _container;

public:
  explicit {{CLASSNAME}}() = default;
  enum class ObjectConstruction { NoData };
  explicit {{CLASSNAME}}(ObjectConstruction) noexcept:
    {{CLASSNAME}}() {}
  ~{{CLASSNAME}}() noexcept = default;

  {{CLASSNAME}}(const {{CLASSNAME}}& other);
  {{CLASSNAME}}& operator=(const {{CLASSNAME}}& other);
  {{CLASSNAME}}({{CLASSNAME}}&& other) noexcept;
  {{CLASSNAME}}& operator=({{CLASSNAME}}&& other) noexcept;

  // TODO: Make it a deep clear, so that the memory is freed.
  void clear();
  void clone(const {{CLASSNAME}}& other);

  void merge(::peano4::stacks::SendReceiveContext context, const {{CLASSNAME}}& neighbour, const ::peano4::datamanagement::VertexMarker& marker, int spacetreeId);
  void mergeWithParticle(DoFType* neighbourParticle, const ::peano4::datamanagement::VertexMarker& marker, int spacetreeId);
  bool send(const ::peano4::datamanagement::VertexMarker& marker);
  bool receiveAndMerge(const ::peano4::datamanagement::VertexMarker& marker);
  static ::peano4::grid::LoadStoreComputeFlag loadStoreComputeFlag(const ::peano4::datamanagement::VertexMarker& marker);

  void particlesCanNotBeLiftedLocally();
  Container::iterator particleCanNotBeLiftedLocally(const Container::iterator& particle);

  void particlesCanNotBeDroppedLocallySoRollOverForNextMeshSweep(DoFType* particle);

  void addParticle(DoFType* particle);

  // TODO: Same as clear()
  void deleteParticles();
  Container::iterator deleteParticle(const Container::iterator& particle);

  void grabParticles({{CLASSNAME}}& sourceParticleSet);
  Container::iterator grabParticle(const Container::iterator& particle, {{CLASSNAME}}& sourceParticleSet);

  Container::iterator begin();
  Container::iterator end();
  Container::const_iterator begin() const;
  Container::const_iterator end() const;

  int size() const;
  bool isValid() const;

  std::string toString() const;

#ifdef WITH_MPI
public:
  static MPI_Datatype getBoundaryExchangeDatatype();
  static MPI_Datatype getMultiscaleDataExchangeDatatype();
  static MPI_Datatype getForkDatatype();
  static MPI_Datatype getJoinDatatype();
  static MPI_Datatype getGlobalCommunicationDatatype();

  static void startSend(::peano4::stacks::Stack<{{CLASSNAME}}>& stack, const ::peano4::stacks::SendReceiveContext context, MPI_Comm comm);
  static void startReceive(::peano4::stacks::Stack<{{CLASSNAME}}>& stack, const ::peano4::stacks::SendReceiveContext context, MPI_Comm comm, int numberOfElements);
  static bool tryToFinishSendOrReceive(::peano4::stacks::Stack<{{CLASSNAME}}>& stack);
#endif
};
