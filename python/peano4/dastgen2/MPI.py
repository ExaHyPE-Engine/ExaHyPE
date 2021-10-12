# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from dastgen2 import *

from peano4.datamodel.DoF import DoFAssociation


class MPI(object):
  """ 
  
  Represents Peano's MPI aspect injected into a DaStGen model. 
  
  
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
  
  =====
  Usage
  =====
  
  The instance is to be added to a DaStGen2 model through add_aspect().  
  
  If you want to inject a particular merge code, just set the internal 
  string self.merge_implementation.
  
  ==========
  Attributes
  ==========

  dof_association: DoFAssociation
    Clarifies which grid entity the underlying datatype is associated
    to.
  
  """
  def __init__(self, dof_association_):
    self.dof_association = dof_association_
    self.merge_implementation = ""
    pass


  def __str__(self):
    result = "("
    if self.dof_association==DoFAssociation.Vertex:
      result += "vertex"
    if self.dof_association==DoFAssociation.Face:
      result += "face"
    if self.dof_association==DoFAssociation.Cell:
      result += "cell"
    if self.merge_implementation!="":
      result += ",merge-impl="
      result += self.merge_implementation
    else:
      result += ",empty-merge-impl"
    result += ")"
    return result


  def set_model(self,data_model):
    self._data_model = data_model

  def get_include(self):
    result = """
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"
#include "peano4/utils/Globals.h"
"""
    if self.dof_association!=DoFAssociation.Generic:
      result += """
#include "peano4/datamanagement/CellMarker.h"
#include "peano4/datamanagement/FaceMarker.h"
#include "peano4/datamanagement/VertexMarker.h"
"""
    return result
  
  def get_attributes(self):
    return ""
  
  def get_method_declarations(self,full_qualified_name):
    result = """
#ifdef Parallel
    static void sendAndPollDanglingMessages(const """ + full_qualified_name + """& message, int destination, int tag, MPI_Comm communicator=tarch::mpi::Rank::getInstance().getCommunicator());
    static void receiveAndPollDanglingMessages(""" + full_qualified_name + """& message, int source, int tag, MPI_Comm communicator=tarch::mpi::Rank::getInstance().getCommunicator() );
#endif
    """
    
    if self.dof_association==DoFAssociation.Vertex:
      result += """
    void merge(const """ + full_qualified_name + """& neighbour, const peano4::datamanagement::VertexMarker& marker);
    
    static bool receiveAndMerge(const peano4::datamanagement::VertexMarker& marker);
    static bool send(const peano4::datamanagement::VertexMarker& marker);
    static bool storePersistently(const peano4::datamanagement::VertexMarker& marker);
    static bool loadPersistently(const peano4::datamanagement::VertexMarker& marker);
""" 
    elif self.dof_association==DoFAssociation.Face:
      result += """
    void merge(const """ + full_qualified_name + """& neighbour, const peano4::datamanagement::FaceMarker& marker);

    static bool receiveAndMerge(const peano4::datamanagement::FaceMarker& marker);
    static bool send(const peano4::datamanagement::FaceMarker& marker);
    static bool storePersistently(const peano4::datamanagement::FaceMarker& marker);
    static bool loadPersistently(const peano4::datamanagement::FaceMarker& marker);
""" 
    elif self.dof_association==DoFAssociation.Cell:
      result += """
    void merge(const """ + full_qualified_name + """& neighbour, const peano4::datamanagement::CellMarker& marker);

    static bool receiveAndMerge(const peano4::datamanagement::CellMarker& marker);
    static bool send(const peano4::datamanagement::CellMarker& marker);
    static bool storePersistently(const peano4::datamanagement::CellMarker& marker);
    static bool loadPersistently(const peano4::datamanagement::CellMarker& marker);
""" 
      pass
    elif self.dof_association==DoFAssociation.Generic or self.dof_association==DoFAssociation.Global:
      pass
    else:
      assert False
    
    return result


  def get_implementation(self,full_qualified_name):
    result = """
#ifdef Parallel
void """ + full_qualified_name + """::sendAndPollDanglingMessages(const """ + full_qualified_name + """& message, int destination, int tag, MPI_Comm communicator ) {
  """ + full_qualified_name + """::send(
    message, destination, tag,
    [&]() {
      auto  timeOutWarning   = tarch::mpi::Rank::getInstance().getDeadlockWarningTimeStamp();
      auto timeOutShutdown  = tarch::mpi::Rank::getInstance().getDeadlockTimeOutTimeStamp();
      bool triggeredTimeoutWarning = false;
      if (
        tarch::mpi::Rank::getInstance().isTimeOutWarningEnabled() &&
        (std::chrono::system_clock::now()>timeOutWarning) &&
        (!triggeredTimeoutWarning)
      ) {
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( """ + "\"" + full_qualified_name + "\"" + """, "sendAndPollDanglingMessages()",destination, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( """ + "\"" + full_qualified_name + "\"" + """, "sendAndPollDanglingMessages()", destination, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}


void """ + full_qualified_name + """::receiveAndPollDanglingMessages(""" + full_qualified_name + """& message, int source, int tag, MPI_Comm communicator ) {
  """ + full_qualified_name + """::receive(
    message, source, tag,
    [&]() {
      auto timeOutWarning   = tarch::mpi::Rank::getInstance().getDeadlockWarningTimeStamp();
      auto timeOutShutdown  = tarch::mpi::Rank::getInstance().getDeadlockTimeOutTimeStamp();
      bool triggeredTimeoutWarning = false;
      if (
        tarch::mpi::Rank::getInstance().isTimeOutWarningEnabled() &&
        (std::chrono::system_clock::now()>timeOutWarning) &&
        (!triggeredTimeoutWarning)
      ) {
        tarch::mpi::Rank::getInstance().writeTimeOutWarning( """ + "\"" + full_qualified_name + "\"" + """, "receiveAndPollDanglingMessages()", source, tag );
        triggeredTimeoutWarning = true;
      }
      if (
        tarch::mpi::Rank::getInstance().isTimeOutDeadlockEnabled() &&
        (std::chrono::system_clock::now()>timeOutShutdown)
      ) {
        tarch::mpi::Rank::getInstance().triggerDeadlockTimeOut( """ + "\"" + full_qualified_name + "\"" + """, "receiveAndPollDanglingMessages()", source, tag );
      }
      tarch::services::ServiceRepository::getInstance().receiveDanglingMessages();
    },
    communicator
  );
}
#endif
    """


    if self.dof_association==DoFAssociation.Vertex:
      result += """
void """ + full_qualified_name + "::merge(const """ + full_qualified_name + """& neighbour, const peano4::datamanagement::VertexMarker& marker) {
""" + self.merge_implementation + """
}


bool """ + full_qualified_name + """::receiveAndMerge(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool """ + full_qualified_name + """::send(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool """ + full_qualified_name + """::storePersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


bool """ + full_qualified_name + """::loadPersistently(const peano4::datamanagement::VertexMarker& marker) {
  return true;
}


""" 
    elif self.dof_association==DoFAssociation.Face:
      result += """
void """ + full_qualified_name + "::merge(const """ + full_qualified_name + """& neighbour, const peano4::datamanagement::FaceMarker& marker) {
""" + self.merge_implementation + """
}


bool """ + full_qualified_name + """::receiveAndMerge(const peano4::datamanagement::FaceMarker& marker) {
  return true;
}


bool """ + full_qualified_name + """::send(const peano4::datamanagement::FaceMarker& marker) {
  return true;
}


bool """ + full_qualified_name + """::storePersistently(const peano4::datamanagement::FaceMarker& marker) {
  return true;
}


bool """ + full_qualified_name + """::loadPersistently(const peano4::datamanagement::FaceMarker& marker) {
  return true;
}
""" 
    elif self.dof_association==DoFAssociation.Cell:
      result += """
void """ + full_qualified_name + "::merge(const """ + full_qualified_name + """& neighbour, const peano4::datamanagement::CellMarker& marker) {
""" + self.merge_implementation + """
}


bool """ + full_qualified_name + """::receiveAndMerge(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool """ + full_qualified_name + """::send(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool """ + full_qualified_name + """::storePersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}


bool """ + full_qualified_name + """::loadPersistently(const peano4::datamanagement::CellMarker& marker) {
  return true;
}
"""      
    elif self.dof_association==DoFAssociation.Generic or self.dof_association==DoFAssociation.Global:
      pass
    else:
      assert False
    
    
    return result




