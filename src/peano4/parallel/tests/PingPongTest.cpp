#include "PingPongTest.h"
#include "../StartTraversalMessage.h"
#include "../TreeManagementMessage.h"

#include "tarch/mpi/IntegerMessage.h"


#include "tarch/la/Vector.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Core.h"

#include "tarch/multicore/Tasks.h"


tarch::logging::Log peano4::parallel::tests::PingPongTest::_log("peano4::parallel::tests::PingPongTest");


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",off)
#endif


peano4::parallel::tests::PingPongTest::PingPongTest():
  TestCase( "peano4::parallel::tests::PingPongTest" ) {
}


void peano4::parallel::tests::PingPongTest::testBuiltInType() {
  #ifdef Parallel
  int out = 23;
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    MPI_Send(&out,1,MPI_INT,1,0,MPI_COMM_WORLD);
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    int in = 25;
    MPI_Recv(&in,1,MPI_INT,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    validateEquals( in, out );
  }
  MPI_Barrier(MPI_COMM_WORLD);
  #endif
}


namespace {
  int testErrors = 0;

  class PingPongSendTask: public tarch::multicore::Task {
    private:
      const bool _blockingMPI;
    public:
      PingPongSendTask(int id, bool blockingMPI):
        Task(id,0),
        _blockingMPI(blockingMPI) {}
   
      bool run() {
        #ifdef Parallel
        const int out = 23+_id;
        static tarch::logging::Log _log( "peano4::parallel::tests::PingPongSendTask" );
        if (_blockingMPI) {
          MPI_Send(&out,1,MPI_INT,1,_id,MPI_COMM_WORLD);
          logDebug( "PingPongSendTask()", "sent blocking message " << out << " to rank 1 with tag " << _id );
        }
        else {
          MPI_Request request;
          MPI_Isend(&out,1,MPI_INT,1,_id,MPI_COMM_WORLD,&request);
          logDebug( "PingPongSendTask()", "sent non-blocking message " << out << " to rank 1 with tag " << _id );
          tarch::multicore::yield();
          MPI_Wait(&request,MPI_STATUS_IGNORE);
        }
        #endif
        return false;
      }
  };


  class PingPongReceiveTask: public tarch::multicore::Task {
    private:
      const bool _blockingMPI;
    public:
      PingPongReceiveTask(int id, bool blockingMPI):
        Task(id,0),
        _blockingMPI(blockingMPI) {}

      bool run() {
        #ifdef Parallel
        int in = -12;
        static tarch::logging::Log _log( "peano4::parallel::tests::PingPongSendTask" );
        logDebug( "PingPongReceiveTask()", "receive message from rank 1 with tag " << _id );
        if (_blockingMPI) {
          MPI_Recv(&in,1,MPI_INT,0,_id,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
        }
        else {
          MPI_Request request;
          MPI_Irecv(&in,1,MPI_INT,0,_id,MPI_COMM_WORLD,&request);
          tarch::multicore::yield();
          MPI_Wait(&request,MPI_STATUS_IGNORE);
        }
        logDebug( "PingPongReceiveTask()", "got content " << in );
        if ( in != 23+_id) {
          logError( "testMultithreadedPingPong()", "received " << in << " instead of " << (23+_id) << " (blocking mode=" << _blockingMPI << ", tag=" << _id << ")" );
          testErrors++;
        }
        #endif
        return false;
      }
  };
}


void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithBlockingReceives() {
  #ifdef Parallel
  int out;
  testErrors = 0;
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      out = 23 + i;
      MPI_Send(&out,1,MPI_INT,1,i,MPI_COMM_WORLD);
    }
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    std::vector< tarch::multicore::Task* > tasks;
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back( new PingPongReceiveTask(i,true) );
    }
    tarch::multicore::spawnAndWait( tasks );
  }
  MPI_Barrier(MPI_COMM_WORLD);
  validate( testErrors==0 );
  #endif
}


void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithNonblockingReceives() {
  #ifdef Parallel
  int out;
  testErrors = 0;
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      out = 23 + i;
      MPI_Send(&out,1,MPI_INT,1,i,MPI_COMM_WORLD);
    }
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    std::vector< tarch::multicore::Task* > tasks;
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back( new PingPongReceiveTask(i,false) );
    }
    tarch::multicore::spawnAndWait( tasks );
  }
  MPI_Barrier(MPI_COMM_WORLD);
  validate( testErrors==0 );
  #endif
}



void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithBlockingSends() {
  #ifdef Parallel
  int out = 23;
  testErrors = 0;
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    std::vector< tarch::multicore::Task* > tasks;
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back( new PingPongSendTask(i,true) );
    }
    tarch::multicore::spawnAndWait( tasks );
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    int in = -12;
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      MPI_Recv(&in,1,MPI_INT,0,i,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      validateEquals( in, out+i );
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  validate( testErrors==0 );
  #endif
}


void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithNonblockingSends() {
  #ifdef Parallel
  int out = 23;
  testErrors = 0;
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    std::vector< tarch::multicore::Task* > tasks;
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back( new PingPongSendTask(i,false) );
    }
    tarch::multicore::spawnAndWait( tasks );
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    int in = -12;
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      MPI_Recv(&in,1,MPI_INT,0,i,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
      validateEquals( in, out+i );
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  validate( testErrors==0 );
  #endif
}


void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithBlockingSendsAndReceives() {
  #ifdef Parallel
  int out = 23;
  testErrors = 0;
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    std::vector< tarch::multicore::Task* > tasks;
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back( new PingPongSendTask(i,true) );
    }
    tarch::multicore::spawnAndWait( tasks );
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    std::vector< tarch::multicore::Task* > tasks;
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back( new PingPongReceiveTask(i,true) );
    }
    tarch::multicore::spawnAndWait( tasks );
  }
  MPI_Barrier(MPI_COMM_WORLD);
  validate( testErrors==0 );
  #endif
}


void peano4::parallel::tests::PingPongTest::testMultithreadedPingPongWithNonblockingSendsAndReceives() {
  #ifdef Parallel
  int out = 23;
  testErrors = 0;
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    std::vector< tarch::multicore::Task* > tasks;
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back( new PingPongSendTask(i,false) );
    }
    tarch::multicore::spawnAndWait( tasks );
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    std::vector< tarch::multicore::Task* > tasks;
    for (int i=0; i<tarch::multicore::Core::getInstance().getNumberOfThreads(); i++) {
      tasks.push_back( new PingPongReceiveTask(i,false) );
    }
    tarch::multicore::spawnAndWait( tasks );
  }
  MPI_Barrier(MPI_COMM_WORLD);
  validate( testErrors==0 );
  #endif
}


void peano4::parallel::tests::PingPongTest::testDaStGenTypeStartTraversalMessage() {
  #ifdef Parallel
  StartTraversalMessage out;
  out.setStepIdentifier(23);
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    StartTraversalMessage::send( out, 1, 0, MPI_COMM_WORLD );
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    StartTraversalMessage in;
    StartTraversalMessage::receive( in, 0, 0, MPI_COMM_WORLD );
    validateEqualsWithParams2( in.getStepIdentifier(), out.getStepIdentifier(), in.toString(), out.toString() );
  }
  MPI_Barrier(MPI_COMM_WORLD);
  #endif
}


void peano4::parallel::tests::PingPongTest::testDaStGenTypeIntegerMessage() {
  #ifdef Parallel
  tarch::mpi::IntegerMessage out;
  out.setValue(23);
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    MPI_Send(&out,1,tarch::mpi::IntegerMessage::Datatype,1,0,MPI_COMM_WORLD);
    tarch::mpi::IntegerMessage::send(
      out, 1, 0,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    tarch::mpi::IntegerMessage inThroughMPI;
    inThroughMPI.setValue(72);
    MPI_Recv(&inThroughMPI,1,tarch::mpi::IntegerMessage::Datatype,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    validateEqualsWithParams2( inThroughMPI.getValue(),     out.getValue(), inThroughMPI.toString(),     out.toString() );

    tarch::mpi::IntegerMessage inThroughDaStGen;
    inThroughDaStGen.setValue(73);
    tarch::mpi::IntegerMessage::receive(
      inThroughDaStGen, 0, 0,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    validateEqualsWithParams2( inThroughDaStGen.getValue(), out.getValue(), inThroughDaStGen.toString(), out.toString() );
  }
  MPI_Barrier(MPI_COMM_WORLD);
  #endif
}


void peano4::parallel::tests::PingPongTest::testDaStGenArray() {
  #ifdef Parallel
  StartTraversalMessage out[10];
  out[0].setStepIdentifier(23);
  out[1].setStepIdentifier(24);
  out[2].setStepIdentifier(25);
  out[3].setStepIdentifier(26);
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    MPI_Send(out,4,StartTraversalMessage::Datatype,1,0,MPI_COMM_WORLD);
  }
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    StartTraversalMessage in[10];
    MPI_Recv(in,4,StartTraversalMessage::Datatype,0,0,MPI_COMM_WORLD,MPI_STATUS_IGNORE);
    validateEqualsWithParams8(
      in[0].getStepIdentifier(), out[0].getStepIdentifier(),
	  in[0].toString(), out[0].toString(),
	  in[1].toString(), out[1].toString(),
	  in[2].toString(), out[2].toString(),
	  in[3].toString(), out[3].toString()
	);
    validateEqualsWithParams8(
      in[1].getStepIdentifier(), out[1].getStepIdentifier(),
	  in[0].toString(), out[0].toString(),
	  in[1].toString(), out[1].toString(),
	  in[2].toString(), out[2].toString(),
	  in[3].toString(), out[3].toString()
	);
    validateEqualsWithParams8(
      in[2].getStepIdentifier(), out[2].getStepIdentifier(),
	  in[0].toString(), out[0].toString(),
	  in[1].toString(), out[1].toString(),
	  in[2].toString(), out[2].toString(),
	  in[3].toString(), out[3].toString()
	);
    validateEqualsWithParams8(
      in[3].getStepIdentifier(), out[3].getStepIdentifier(),
	  in[0].toString(), out[0].toString(),
	  in[1].toString(), out[1].toString(),
	  in[2].toString(), out[2].toString(),
	  in[3].toString(), out[3].toString()
	);
  }
  MPI_Barrier(MPI_COMM_WORLD);
  #endif
}


void peano4::parallel::tests::PingPongTest::testDaStGenArrayTreeManagementMessage() {
  #ifdef Parallel
  peano4::parallel::TreeManagementMessage message;
  const int Tag = 14;
  if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==0) {
    message.setAction( TreeManagementMessage::Action::Acknowledgement );
    peano4::parallel::TreeManagementMessage::send( message, 1, Tag, MPI_COMM_WORLD );
  }
  else if ( tarch::mpi::Rank::getInstance().getNumberOfRanks()>=2 and tarch::mpi::Rank::getInstance().getRank()==1) {
    peano4::parallel::TreeManagementMessage::receive( message, 0, Tag, MPI_COMM_WORLD );
    validateWithParams1( message.getAction()==TreeManagementMessage::Action::Acknowledgement, message.toString() );
  }
  MPI_Barrier(MPI_COMM_WORLD);
  #endif
}


void peano4::parallel::tests::PingPongTest::run() {
  logTraceIn( "run()" );

  testMethod(  testBuiltInType );

  testMethod(  testDaStGenTypeIntegerMessage );
  testMethod(  testDaStGenTypeStartTraversalMessage );
  testMethod(  testDaStGenArray );

  testMethod(  testDaStGenArrayTreeManagementMessage );

  testMethod(  testMultithreadedPingPongWithBlockingReceives );
  testMethod(  testMultithreadedPingPongWithBlockingSends );
  testMethod(  testMultithreadedPingPongWithBlockingSendsAndReceives );

  testMethod(  testMultithreadedPingPongWithNonblockingReceives );
  testMethod(  testMultithreadedPingPongWithNonblockingSends );
  testMethod(  testMultithreadedPingPongWithNonblockingSendsAndReceives );

  logTraceOut( "run()" );
}


#ifdef UseTestSpecificCompilerSettings
#pragma optimize("",on)
#endif
