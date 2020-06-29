# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from dastgen2 import *

from peano4.datamodel.DoF import DoFAssociation


class MPI(object):
  """ 
  
  Represents Peano's MPI aspect injected into a DaStGen model. 
  It mainly ensures that we include the right headers.
  
  The instance is to be added to a DaStGen2 model through add_aspect().
    
  @param dof_association Should be of type DoFAssociation
  """
  def __init__(self, dof_association):
    self._dof_association = dof_association
    pass


  def set_model(self,data_model):
    self._data_model = data_model

  def get_include(self):
    result = """
#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/services/ServiceRepository.h"
#include "peano4/utils/Globals.h"
"""
    if self._dof_association!=DoFAssociation.Generic:
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
    static void sendAndPollDanglingMessages(const """ + full_qualified_name + """& message, int destination, int tag );
    static void receiveAndPollDanglingMessages(""" + full_qualified_name + """& message, int source, int tag );
#endif


    static bool receiveAndMerge();
    static bool send();

    """
    
    if self._dof_association==DoFAssociation.Vertex:
      result += """
void merge(const """ + full_qualified_name + """& neighbour, const peano4::datamanagement::VertexMarker& marker);
""" 
    elif self._dof_association==DoFAssociation.Face:
      result += """
void merge(const """ + full_qualified_name + """& neighbour, const peano4::datamanagement::FaceMarker& marker);
""" 
    elif self._dof_association==DoFAssociation.Cell:
      pass
    elif self._dof_association==DoFAssociation.Generic:
      pass
    else:
      assert False
    
    return result


  def get_implementation(self,full_qualified_name):
    result = """
bool """ + full_qualified_name + """::receiveAndMerge() {
  return true;
}


bool """ + full_qualified_name + """::send() {
  return true;
}
    
    
#ifdef Parallel
void """ + full_qualified_name + """::sendAndPollDanglingMessages(const """ + full_qualified_name + """& message, int destination, int tag ) {
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
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}


void """ + full_qualified_name + """::receiveAndPollDanglingMessages(""" + full_qualified_name + """& message, int source, int tag ) {
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
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
}
#endif
    """


    if self._dof_association==DoFAssociation.Vertex:
      result += """
void """ + full_qualified_name + "::merge(const """ + full_qualified_name + """& neighbour, const peano4::datamanagement::VertexMarker& marker) {
}
""" 
    elif self._dof_association==DoFAssociation.Face:
      result += """
void """ + full_qualified_name + "::merge(const """ + full_qualified_name + """& neighbour, const peano4::datamanagement::FaceMarker& marker) {
}
""" 
    elif self._dof_association==DoFAssociation.Cell:
      pass
    elif self._dof_association==DoFAssociation.Generic:
      pass
    else:
      assert False
    
    
    return result




