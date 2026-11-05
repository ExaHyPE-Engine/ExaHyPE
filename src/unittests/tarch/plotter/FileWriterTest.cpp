// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "tarch/plotter/FileWriter.h"

#include "tarch/testing/UnitTest.h"

UNIT_TEST("Every time stamp reads back as the same double") {
  GIVEN("Seven times from zero to a million, among them a third, two times a quarter apart and 1e-300") {
    constexpr std::array<double, 7> Times{0.0, 0.1, 1.0 / 3.0, 1000000.25, 1000000.5, 1.0e-300, 12345.678901234567};

    WHEN("getExactTimeStamp() writes every time as a time stamp") {
      std::vector<std::string> timeStamps;
      for (const double time : Times) {
        timeStamps.push_back(tarch::plotter::getExactTimeStamp(time));
      }

      THEN("Every time stamp reads back as its time, 0.1 reads 0.1 and the two times a quarter apart differ") {
        for (std::size_t index = 0; index < Times.size(); index++) {
          VALIDATE_EQUALS(std::stod(timeStamps[index]), Times[index], timeStamps[index]);
        }
        VALIDATE_EQUALS(timeStamps[1], std::string("0.1"));
        VALIDATE(timeStamps[3] != timeStamps[4], timeStamps[3], timeStamps[4]);
      }
    }
  }
}
