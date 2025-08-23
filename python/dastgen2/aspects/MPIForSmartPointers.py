# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .Aspect import Aspect
#import dastgen2

from dastgen2.Utils import construct_ifdef_string

from .MPI import MPI


class MPIForSmartPointers(Aspect):
    """!

    Represents the MPI aspect injected into a DaStGen model.
    
    This object replaces the MPI aspect when we stream a DaStGen object onto 
    disk. The aspect makes the object behave like a non-persistent wrapper:
    The very moment we issue a send or a receive, we start to hold an instance
    of the flattened object to which we delegate all function calls.
    
    The routine replacing the generic MPI aspect with this one is typically
    peano4.datamodel.DaStGen2GeneratorForObjectsWithSmartPointers.
    
    """

    def __init__(self, data_model):
        """!
        
        This constructor has to know the underlying data model. So you have
        to tell it to which object you attach the aspect to.
        
        If we replace an existing MPI aspect with an instance of this object,
        we can pass in aspect._data_model as argument. For peano4.datamodel.DaStGen2,
        this reads as 
        
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        for aspect in old_aspects:
            if isinstance(aspect,dastgen2.aspects.MPI):
                self._data.data._aspects.append( dastgen2.aspects.MPIForSmartPointers(aspect._data_model) )
        ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
        
        
        """
        self._data_model = data_model
        pass

    def get_include(self):
        return """
#ifdef Parallel
  #include <mpi.h>
  #include <functional>
#endif

#include "{}_Flattened.h"            
""".format( self._data_model.name )


    def get_attributes(self):
        return """
    #ifdef Parallel
    private:
      int                  _senderDestinationRank;
    #endif
"""

    def get_method_declarations(self, full_qualified_name):
        return """    
    using SmartType = {};
    using FlattenedType = {}_Flattened;

    #ifdef Parallel
    /**
     * Hands out MPI datatype if we work without the LLVM MPI extension.
     * If we work with this additional feature, this is the routine where
     * the lazy initialisation is done and the datatype is also cached.
     */
    static MPI_Datatype  getForkDatatype();

    static MPI_Datatype  getJoinDatatype();

    static MPI_Datatype  getBoundaryExchangeDatatype();

    static MPI_Datatype  getMultiscaleDataExchangeDatatype();

    static MPI_Datatype  getGlobalCommunciationDatatype();

    static void  freeForkDatatype();

    static void  freeJoinDatatype();

    static void  freeBoundaryExchangeDatatype();

    static void  freeMultiscaleDataExchangeDatatype();

    static void  freeGlobalCommunciationDatatype();

    int getSenderRank() const;

    static void initDatatype();

    static void shutdownDatatype();

    static void send(const SmartType& buffer, int destination, int tag, MPI_Comm communicator );
    static void receive(SmartType& buffer, int source, int tag, MPI_Comm communicator );

    static void send(const SmartType& buffer, int destination, int tag, std::function<void()> startCommunicationFunctor, std::function<void()> waitFunctor, MPI_Comm communicator );
    static void receive(SmartType& buffer, int source,      int tag, std::function<void()> startCommunicationFunctor, std::function<void()> waitFunctor, MPI_Comm communicator );
    #endif
    
    static FlattenedType flatten(const SmartType& copy);
    static SmartType     unflatten(const FlattenedType& copy);
""".format(full_qualified_name,
           full_qualified_name)


    def get_implementation(self, full_qualified_name):
        copy_statements = ""
        for attribute in self._data_model._attributes:
            if attribute.ifdefs==[]:
               copy_statements += """
result.set{}( static_cast<decltype(result.get{}())>( copy.get{}() ) );
""".format(attribute.get_accessor_name(),
           attribute.get_accessor_name(),
           attribute.get_accessor_name()
           )
            else:
                copy_statements += """
{}
result.set{}( static_cast<decltype(result.get{}())>( copy.get{}() ) );
#endif
""".format(dastgen2.Utils.construct_ifdef_string(attribute.ifdefs),
           attribute.get_accessor_name(),
           attribute.get_accessor_name(),
           attribute.get_accessor_name(),
           )

        return """
#ifdef Parallel
void """ + full_qualified_name + """::freeForkDatatype() {
  FlattenedType::freeForkDatatype();
}

void """ + full_qualified_name + """::freeGlobalCommunciationDatatype() {
  FlattenedType::freeGlobalCommunciationDatatype();
}

void """ + full_qualified_name + """::freeJoinDatatype() {
  FlattenedType::freeJoinDatatype();
}

void """ + full_qualified_name + """::freeBoundaryExchangeDatatype() {
  FlattenedType::freeBoundaryExchangeDatatype();
}

void """ + full_qualified_name + """::freeMultiscaleDataExchangeDatatype() {
  FlattenedType::freeMultiscaleDataExchangeDatatype();
}

int """ + full_qualified_name + """::getSenderRank() const {
  return _senderDestinationRank;
}

void """ + full_qualified_name + """::initDatatype() {
  FlattenedType::initDatatype();
}

void """ + full_qualified_name + """::shutdownDatatype() {
  FlattenedType::shutdownDatatype();
}

void """  + full_qualified_name + """::send(const SmartType& buffer, int destination, int tag, MPI_Comm communicator ) {
  FlattenedType sendReceiveObject( flatten(buffer) );
  FlattenedType::send( sendReceiveObject, destination, tag, communicator );
}

void """ + full_qualified_name + """::receive(SmartType& buffer, int source, int tag, MPI_Comm communicator ) {
  FlattenedType sendReceiveObject;
  FlattenedType::receive(sendReceiveObject, source, tag, communicator);
  buffer = unflatten( sendReceiveObject );
  buffer._senderDestinationRank = sendReceiveObject.getSenderRank();
}

void """ + full_qualified_name + """::send(
  const SmartType& buffer,
  int destination,
  int tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm communicator
) {
  FlattenedType sendReceiveObject( flatten(buffer) );
  FlattenedType::send( sendReceiveObject, destination, tag, startCommunicationFunctor, waitFunctor, communicator );
}

void """ + full_qualified_name + """::receive(
  SmartType& buffer,
  int source,
  int tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm communicator
) {
  FlattenedType sendReceiveObject;
  FlattenedType::receive(sendReceiveObject, source, tag, startCommunicationFunctor, waitFunctor, communicator);
  buffer = unflatten( sendReceiveObject );
  buffer._senderDestinationRank = sendReceiveObject.getSenderRank();
}
#endif

""" + full_qualified_name + """::FlattenedType """ + full_qualified_name + """::flatten(const SmartType& copy ) {
  FlattenedType result;
  """ + copy_statements + """
  return result;
}

""" + full_qualified_name + """::SmartType """ + full_qualified_name + """::unflatten(const FlattenedType& copy ) {
  SmartType result;
  """ + copy_statements + """
  return result;
}
"""
