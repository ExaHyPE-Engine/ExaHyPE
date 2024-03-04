# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.datamodel.DoF import DoFAssociation

import jinja2
import dastgen2


class MPIAndStorageAspect(dastgen2.aspects.MPI):
    """!

    Represents Peano's MPI and storage aspect injected into a DaStGen model.


    This is an aspect to a DaStGen object, i.e. something that's
    added to a data model to augment it with some behaviour. The
    realisation of this aspect is manifold yet all serves the
    purpose to make data fit for MPI:

    - The aspect ensures that we include the right headers.
    - The aspect ensures that the generated has the right signature
      which in turn depends on the fact to which grid entity the type
      is associated to
    - The aspect lets you embed a data merge operation into the
      generated data.

    The aspect also determines how and
    if we store data or not in Peano. Therefore, it covers more than
    solely MPI. Use the store and load attribute to control these
    predicates. Please study @ref page_peano_localisation for further
    documentation.


    ## Usage

    The instance is to be added to a DaStGen2 model through add_aspect().

    If you want to inject a particular merge code, just set the internal
    string self.merge_implementation.


    ## Attributes

    dof_association: DoFAssociation
      Clarifies which grid entity the underlying datatype is associated
      to.

    """

    def __init__(self, dof_association_):
        super(MPIAndStorageAspect, self).__init__()
        self.dof_association = dof_association_
        self.merge_implementation = ""
        self.receive_predicate = "true"
        self.send_predicate = "true"
        self.load_store_compute_flag = "::peano4::grid::LoadStoreComputeFlag::LoadFromInputStream_ProvideToCalculations_StoreToOutputStream"
        self.includes = ""
        pass

    def __str__(self):
        result = "("
        if self.dof_association == DoFAssociation.Vertex:
            result += "vertex"
        elif self.dof_association == DoFAssociation.Face:
            result += "face"
        elif self.dof_association == DoFAssociation.Cell:
            result += "cell"
        else:
            result += "not-associated"

        if self.merge_implementation != "":
            result += ",merge-impl="
            result += self.merge_implementation
        else:
            result += ",empty-merge-impl"

        result += ",load/store/compute flag=" + self.load_store_compute_flag
        result += ",compute=" + self.compute
        result += ",send=" + self.send_predicate
        result += ",receive=" + self.receive_predicate

        result += ")"
        return result

    def get_include(self):
        result = (
            super(MPIAndStorageAspect, self).get_include()
            + """
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"
#include "peano4/grid/LoadStoreComputeFlag.h"
#include "peano4/utils/Globals.h"
#include "peano4/grid/TraversalObserver.h"
"""
        )
        if self.dof_association != DoFAssociation.Generic:
            result += """
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/VertexMarker.h"
"""
        for include in self.includes:
            result += include
        return result

    def get_method_declarations(self, full_qualified_name):
        d = {
            "full_qualified_name": full_qualified_name,
            "name": full_qualified_name.split("::")[-1],
        }
        result = (
            super(MPIAndStorageAspect, self).get_method_declarations(
                full_qualified_name
            )
            + """

    enum ObjectConstruction {
      NoData
    };

    {{name}}( ObjectConstruction ):
        {{name}}() {}
    
#ifdef Parallel
    static void sendAndPollDanglingMessages(const {{full_qualified_name}}& message, int destination, int tag, MPI_Comm communicator=tarch::mpi::Rank::getInstance().getCommunicator());
    static void receiveAndPollDanglingMessages({{full_qualified_name}}& message, int source, int tag, MPI_Comm communicator=tarch::mpi::Rank::getInstance().getCommunicator() );
#endif
    """
        )

        if self.dof_association == DoFAssociation.Vertex:
            result += """
    void merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{full_qualified_name}}& neighbour, const peano4::datamanagement::VertexMarker& marker, int spacetreeId);
    
    bool receiveAndMerge(const peano4::datamanagement::VertexMarker& marker) const;
    bool send(const peano4::datamanagement::VertexMarker& marker) const;
    static ::peano4::grid::LoadStoreComputeFlag loadStoreComputeFlag(const peano4::datamanagement::VertexMarker& marker);
"""
        elif self.dof_association == DoFAssociation.Face:
            result += """
    void merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{full_qualified_name}}& neighbour, const peano4::datamanagement::FaceMarker& marker, int spacetreeId);

    bool receiveAndMerge(const peano4::datamanagement::FaceMarker& marker) const;
    bool send(const peano4::datamanagement::FaceMarker& marker) const;
    static ::peano4::grid::LoadStoreComputeFlag loadStoreComputeFlag(const peano4::datamanagement::FaceMarker& marker);
"""
        elif self.dof_association == DoFAssociation.Cell:
            result += """
    void merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{full_qualified_name}}& neighbour, const peano4::datamanagement::CellMarker& marker, int spacetreeId);

    bool receiveAndMerge(const peano4::datamanagement::CellMarker& marker) const;
    bool send(const peano4::datamanagement::CellMarker& marker) const;
    static ::peano4::grid::LoadStoreComputeFlag loadStoreComputeFlag(const peano4::datamanagement::CellMarker& marker);
"""
            pass
        elif (
            self.dof_association == DoFAssociation.Generic
            or self.dof_association == DoFAssociation.Global
        ):
            pass
        else:
            assert False

        return jinja2.Template(result).render(**d)

    def get_implementation(self, full_qualified_name):
        d = {
            "full_qualified_name": full_qualified_name,
            "merge_implementation": self.merge_implementation,
            "receive_predicate": self.receive_predicate,
            "send_predicate": self.send_predicate,
            "load_store_compute_flag": self.load_store_compute_flag,
        }

        result = (
            super(MPIAndStorageAspect, self).get_implementation(full_qualified_name)
            + """
#ifdef Parallel
void {{full_qualified_name}}::sendAndPollDanglingMessages(const {{full_qualified_name}}& message, int destination, int tag, MPI_Comm communicator ) {
  {{full_qualified_name}}::send(
    message, destination, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "{{full_qualified_name}}", "sendAndPollDanglingMessages()",destination, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "{{full_qualified_name}}", "sendAndPollDanglingMessages()", destination, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void {{full_qualified_name}}::receiveAndPollDanglingMessages({{full_qualified_name}}& message, int source, int tag, MPI_Comm communicator ) {
  {{full_qualified_name}}::receive(
    message, source, tag,
    [&]() {
      tarch::mpi::Rank::getInstance().setDeadlockWarningTimeStamp();
      tarch::mpi::Rank::getInstance().setDeadlockTimeOutTimeStamp();
    },
    [&]() {
      tarch::mpi::Rank::getInstance().writeTimeOutWarning( "{{full_qualified_name}}", "receiveAndPollDanglingMessages()", source, tag );
      tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( "{{full_qualified_name}}", "receiveAndPollDanglingMessages()", source, tag );
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    """
        )

        if self.dof_association == DoFAssociation.Vertex:
            result += """
void {{full_qualified_name}}::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{full_qualified_name}}& neighbour, const peano4::datamanagement::VertexMarker& marker, int spacetreeId) {
  {{merge_implementation}}
}


bool {{full_qualified_name}}::receiveAndMerge(
  const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[0]}}& {{arg[1]}} {% endfor %}
) const {
  return {{receive_predicate}};
}


bool {{full_qualified_name}}::send(
  const peano4::datamanagement::VertexMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[0]}}& {{arg[1]}} {% endfor %}
) const {
  return {{send_predicate}};
}


::peano4::grid::LoadStoreComputeFlag {{full_qualified_name}}::loadStoreComputeFlag( 
  const peano4::datamanagement::VertexMarker& marker 
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[0]}}& {{arg[1]}} {% endfor %}
) {
  return {{load_store_compute_flag}};
}
"""
        elif self.dof_association == DoFAssociation.Face:
            result += """
void {{full_qualified_name}}::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{full_qualified_name}}& neighbour, const peano4::datamanagement::FaceMarker& marker, int spacetreeId) {
  {{merge_implementation}}
}


bool {{full_qualified_name}}::receiveAndMerge(
  const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[0]}}& {{arg[1]}} {% endfor %}
) const {
  return {{receive_predicate}};
}


bool {{full_qualified_name}}::send(
  const peano4::datamanagement::FaceMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[0]}}& {{arg[1]}} {% endfor %}
) const {
  return {{send_predicate}};
}


::peano4::grid::LoadStoreComputeFlag {{full_qualified_name}}::loadStoreComputeFlag( 
  const peano4::datamanagement::FaceMarker& marker 
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[0]}}& {{arg[1]}} {% endfor %}
) {
  return {{load_store_compute_flag}};
}
"""
        elif self.dof_association == DoFAssociation.Cell:
            result += """
void {{full_qualified_name}}::merge(peano4::grid::TraversalObserver::SendReceiveContext context, const {{full_qualified_name}}& neighbour, const peano4::datamanagement::CellMarker& marker, int spacetreeId) {
  {{merge_implementation}}
}


bool {{full_qualified_name}}::receiveAndMerge(
  const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[0]}}& {{arg[1]}} {% endfor %}
) const {
  return {{receive_predicate}};
}


bool {{full_qualified_name}}::send(
  const peano4::datamanagement::CellMarker& marker
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[0]}}& {{arg[1]}} {% endfor %}
) const {
  return {{send_predicate}};
}


::peano4::grid::LoadStoreComputeFlag {{full_qualified_name}}::loadStoreComputeFlag( 
  const peano4::datamanagement::CellMarker& marker 
  {% for arg in ADDITIONAL_LOAD_STORE_ARGUMENTS %}, const {{arg[0]}}& {{arg[1]}} {% endfor %}
) {
  return {{load_store_compute_flag}};
}
"""
        elif (
            self.dof_association == DoFAssociation.Generic
            or self.dof_association == DoFAssociation.Global
        ):
            pass
        else:
            assert False

        return jinja2.Template(result).render(**d)
