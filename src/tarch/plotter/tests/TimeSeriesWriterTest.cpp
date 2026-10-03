// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "TimeSeriesWriterTest.h"

#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/plotter/PVDTimeSeriesWriter.h"
#include "tarch/plotter/TimeSeriesWriter.h"
#include "tarch/plotter/VTUFileWriter.h"

tarch::plotter::tests::TimeSeriesWriterTest::TimeSeriesWriterTest() :
  tarch::tests::TestCase("tarch::plotter::tests::TimeSeriesWriterTest") {}

void
tarch::plotter::tests::TimeSeriesWriterTest::run() {
  testMethod(testTheTimeStampReadsBackExactly);
  testMethod(testThePVDFileListsEverySnapshotTime);
}

/**
 * Every time stamp reads back as the same double.
 */
void
tarch::plotter::tests::TimeSeriesWriterTest::testTheTimeStampReadsBackExactly() {
  const double times[] = {0.0, 0.1, 1.0 / 3.0, 1000000.25, 1000000.5, 1.0e-300, 12345.678901234567};
  for (const double time : times) {
    const std::string timeStamp = tarch::plotter::getExactTimeStamp(time);
    validateEqualsWithParams1(std::stod(timeStamp), time, timeStamp);
  }
  validateEquals(tarch::plotter::getExactTimeStamp(0.1), std::string("0.1"));
  validate(tarch::plotter::getExactTimeStamp(1000000.25) != tarch::plotter::getExactTimeStamp(1000000.5));
}

/**
 * The PVD file lists the exact time of every snapshot.
 */
void
tarch::plotter::tests::TimeSeriesWriterTest::testThePVDFileListsEverySnapshotTime() {
  const std::string directory = tarch::tests::getWritableTestDirectory().string();
  const std::string filename  = "TimeSeriesWriterTestPVD";
  const double      times[]   = {1000000.25, 1000000.5};

  tarch::plotter::PVDTimeSeriesWriter series;
  for (const double time : times) {
    tarch::plotter::VTUFileWriter snapshotWriter(directory);
    snapshotWriter.acquirePiece(0);
    series.writeSnapshotFile(filename, time, snapshotWriter);
  }
  series.writeTimeSeriesMetaFile(directory, filename);
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    const std::filesystem::path pvdPath = std::filesystem::path(directory) / (filename + ".pvd");
    const std::ifstream         stream(pvdPath);
    std::stringstream           buffer;
    buffer << stream.rdbuf();
    const std::string content = buffer.str();
    validate(content.find("timestep=\"1000000.25\"") != std::string::npos);
    validate(content.find("timestep=\"1000000.5\"") != std::string::npos);
  }
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    std::vector<std::filesystem::path> writtenFiles;
    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
      if (entry.path().filename().string().starts_with(filename)) {
        writtenFiles.push_back(entry.path());
      }
    }
    for (const auto& path : writtenFiles) {
      std::filesystem::remove(path);
    }
  }
  tarch::mpi::barrier(tarch::mpi::Rank::getInstance().getCommunicator());
}
