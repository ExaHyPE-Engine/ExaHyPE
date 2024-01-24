# This file is part of the DaStGen2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import dastgen2
from dastgen2.Utils import construct_ifdef_string


class MPI(object):
    """

    Represents the MPI aspect injected into a DaStGen model.

    """

    def __init__(self):
        pass

    def set_model(self, data_model):
        self._data_model = data_model

    def get_include(self):
        return """
#ifdef Parallel
  #include <mpi.h>
  #include <functional>
#endif
"""

    def get_attributes(self):
        return """
    #ifdef Parallel
    private:
      int                  _senderDestinationRank;

      #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
      /**
       * Whenever we use LLVM's MPI extension (DaStGe), we rely on lazy
       * initialisation of the datatype. However, Peano calls init explicitly
       * in most cases. Without the LLVM extension which caches the MPI
       * datatype once constructed, this field stores the type.
       */
      static MPI_Datatype  Datatype;
      #endif
    #endif
"""

    def get_method_declarations(self, full_qualified_name):
        return (
            """
    #ifdef Parallel
    /**
     * Hands out MPI datatype if we work without the LLVM MPI extension.
     * If we work with this additional feature, this is the routine where
     * the lazy initialisation is done and the datatype is also cached.
     */
    [[clang::map_mpi_datatype]]
    static MPI_Datatype  getForkDatatype();

    [[clang::map_mpi_datatype]]
    static MPI_Datatype  getJoinDatatype();

    [[clang::map_mpi_datatype]]
    static MPI_Datatype  getBoundaryExchangeDatatype();

    [[clang::map_mpi_datatype]]
    static MPI_Datatype  getMultiscaleDataExchangeDatatype();

    [[clang::map_mpi_datatype]]
    static MPI_Datatype  getGlobalCommunciationDatatype();

    [[clang::map_mpi_datatype]]
    static void  freeForkDatatype();

    [[clang::map_mpi_datatype]]
    static void  freeJoinDatatype();

    [[clang::map_mpi_datatype]]
    static void  freeBoundaryExchangeDatatype();

    [[clang::map_mpi_datatype]]
    static void  freeMultiscaleDataExchangeDatatype();

    [[clang::map_mpi_datatype]]
    static void  freeGlobalCommunciationDatatype();

    /**
     * @return The rank of the sender of an object. It only make ssense to call
     *         this routine after you've invoked receive with MPI_ANY_SOURCE.
     */
    int getSenderRank() const;

    /**
     * Wrapper around getDatatype() to trigger lazy evaluation if we
     * use the lazy initialisation.
     */
    static void initDatatype();

    /**
     * Free the underlying MPI datatype.
     */
    static void shutdownDatatype();

    /**
     * In DaStGen (the first version), I had a non-static version of the send
     * as well as the receive. However, this did not work with newer C++11
     * versions, as a member function using this as pointer usually doesn't
     * see the vtable while the init sees the object from outside, i.e.
     * including a vtable. So this routine now is basically an alias for a
     * blocking MPI_Send.
     */
    static void send(const """
            + full_qualified_name
            + """& buffer, int destination, int tag, MPI_Comm communicator );
    static void receive("""
            + full_qualified_name
            + """& buffer, int source, int tag, MPI_Comm communicator );

    /**
     * Alternative to the other send() where I trigger a non-blocking send an
     * then invoke the functor until the corresponding MPI_Test tells me that
     * the message went through. In systems with heavy MPI usage, this can
     * help to avoid deadlocks.
     */
    static void send(const """
            + full_qualified_name
            + """& buffer, int destination, int tag, std::function<void()> startCommunicationFunctor, std::function<void()> waitFunctor, MPI_Comm communicator );
    static void receive(   """
            + full_qualified_name
            + """& buffer, int source,      int tag, std::function<void()> startCommunicationFunctor, std::function<void()> waitFunctor, MPI_Comm communicator );
    #endif
"""
        )

    def get_implementation(self, full_qualified_name):
        result = (
            """
#ifdef Parallel

#if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
MPI_Datatype """
            + full_qualified_name
            + """::Datatype = MPI_DATATYPE_NULL;
#endif


[[clang::map_mpi_datatype]]
MPI_Datatype """
            + full_qualified_name
            + """::getForkDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype """
            + full_qualified_name
            + """::getGlobalCommunciationDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype """
            + full_qualified_name
            + """::getJoinDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype """
            + full_qualified_name
            + """::getBoundaryExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
MPI_Datatype """
            + full_qualified_name
            + """::getMultiscaleDataExchangeDatatype() {
  return Datatype;
}


[[clang::map_mpi_datatype]]
void """
            + full_qualified_name
            + """::freeForkDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void """
            + full_qualified_name
            + """::freeGlobalCommunciationDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void """
            + full_qualified_name
            + """::freeJoinDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void """
            + full_qualified_name
            + """::freeBoundaryExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


[[clang::map_mpi_datatype]]
void """
            + full_qualified_name
            + """::freeMultiscaleDataExchangeDatatype() {
  if (Datatype != MPI_DATATYPE_NULL){
    MPI_Type_free(&Datatype);
    Datatype = MPI_DATATYPE_NULL;
  }
}


int """
            + full_qualified_name
            + """::getSenderRank() const {
  return _senderDestinationRank;
}



void """
            + full_qualified_name
            + """::initDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  """
            + full_qualified_name
            + """  instances[2];

  int NumberOfAttributes = 0;
"""
        )

        for i in self._data_model._attributes:
            if i._is_static or i._is_const_static or i._is_constexpr or i._is_const:
                # No comms for static/const members
                continue
            if i.ifdefs != []:
                result += construct_ifdef_string(i.ifdefs)
                result += "  NumberOfAttributes++;\n#endif \n"
            else:
                result += "  NumberOfAttributes++;\n"

        result += """
  MPI_Datatype* subtypes = new MPI_Datatype[NumberOfAttributes];
  int*          blocklen = new int[NumberOfAttributes];
  MPI_Aint*     disp     = new MPI_Aint[NumberOfAttributes];

  int counter            = 0;
"""

        for i in self._data_model._attributes:
            if i._is_static or i._is_const_static or i._is_constexpr or i._is_const:
                # No comms for static/const members
                continue
            if i.ifdefs != []:
                result += construct_ifdef_string(i.ifdefs)
            for ii in i.get_native_MPI_type():
                result += "  subtypes[counter] = " + ii[0] + ";\n"
                result += "  blocklen[counter] = " + str(ii[1]) + ";\n"
                result += "  counter++;\n"
            if i.ifdefs != []:
                result += "#endif\n"

        result += """
  MPI_Aint  baseFirstInstance;
  MPI_Aint  baseSecondInstance;
  MPI_Get_address( &instances[0], &baseFirstInstance );
  MPI_Get_address( &instances[1], &baseSecondInstance );

  counter = 0;
"""

        for i in self._data_model._attributes:
            if i._is_static or i._is_const_static or i._is_constexpr or i._is_const:
                # no comms for static/const members
                continue
            if i.ifdefs != []:
                result += "\n"
                result += construct_ifdef_string(i.ifdefs)
            for ii in i.get_first_plain_C_attribute():
                result += "  MPI_Get_address( &(instances[0]."
                result += ii[0]
                result += "), &disp[counter] );\n"
                result += "  counter++;\n"
            if i.ifdefs != []:
                result += "#endif\n"

        result += (
            """
  MPI_Aint offset = disp[0] - baseFirstInstance;
  MPI_Aint extent = baseSecondInstance - baseFirstInstance - offset;
  for (int i=NumberOfAttributes-1; i>=0; i--) {
    disp[i] = disp[i] - disp[0];
  }

  int errorCode = 0;
  MPI_Datatype tmpType;
  errorCode += MPI_Type_create_struct( NumberOfAttributes, blocklen, disp, subtypes, &tmpType );
  errorCode += MPI_Type_create_resized( tmpType, offset, extent, &Datatype );
  errorCode += MPI_Type_commit( &Datatype );
  errorCode += MPI_Type_free( &tmpType );
  if (errorCode) std::cerr << "error constructing MPI datatype in " << __FILE__ << ":" << __LINE__ << std::endl;

  delete[] subtypes;
  delete[] blocklen;
  delete[] disp;

  #else
  // invoke routine once to trigger lazy initialisation
  getForkDatatype();
  getJoinDatatype();
  getBoundaryExchangeDatatype();
  getMultiscaleDataExchangeDatatype();
  getGlobalCommunciationDatatype();
  #endif
}


void """
            + full_qualified_name
            + """::shutdownDatatype() {
  #if !defined(__MPI_ATTRIBUTES_LANGUAGE_EXTENSION__)
  freeForkDatatype();
  freeJoinDatatype();
  freeBoundaryExchangeDatatype();
  freeMultiscaleDataExchangeDatatype();
  freeGlobalCommunciationDatatype();
  #else
  MPI_Datatype type = Datatype;
  MPI_Type_free( &type );
  #endif
}


void """
            + full_qualified_name
            + """::send(const """
            + full_qualified_name
            + """& buffer, int destination, int tag, MPI_Comm communicator ) {
  MPI_Send( &buffer, 1, Datatype, destination, tag, communicator);
}


void """
            + full_qualified_name
            + """::receive("""
            + full_qualified_name
            + """& buffer, int source, int tag, MPI_Comm communicator ) {
  MPI_Status status;
  MPI_Recv( &buffer, 1, Datatype, source, tag, communicator, &status);
  buffer._senderDestinationRank = status.MPI_SOURCE;
}


void """
            + full_qualified_name
            + """::send(
  const """
            + full_qualified_name
            + """& buffer,
  int destination,
  int tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm communicator
) {
  MPI_Request sendRequestHandle;
  int         flag = 0;
  MPI_Isend( &buffer, 1, Datatype, destination, tag, communicator, &sendRequestHandle );
  MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  startCommunicationFunctor();
  while (!flag) {
    waitFunctor();
    MPI_Test( &sendRequestHandle, &flag, MPI_STATUS_IGNORE );
  }
}


void """
            + full_qualified_name
            + """::receive(
  """
            + full_qualified_name
            + """& buffer,
  int source,
  int tag,
  std::function<void()> startCommunicationFunctor,
  std::function<void()> waitFunctor,
  MPI_Comm communicator
) {
  MPI_Status  status;
  MPI_Request receiveRequestHandle;
  int         flag = 0;
  MPI_Irecv( &buffer, 1, Datatype, source, tag, communicator, &receiveRequestHandle );
  MPI_Test( &receiveRequestHandle, &flag, &status );
  startCommunicationFunctor();
  while (!flag) {
    waitFunctor();
    MPI_Test( &receiveRequestHandle, &flag, &status );
  }
  buffer._senderDestinationRank = status.MPI_SOURCE;
}
#endif
"""
        )

        return result
