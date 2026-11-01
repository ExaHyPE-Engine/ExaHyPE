// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "exahype2/TimeStepping.h"

#include "exahype2/Limits.h"
#include "tarch/mpi/Rank.h"
#include "tarch/mpi/ReductionRecord.h"
#include "tarch/testing/UnitTest.h"

namespace {
  using Rule      = exahype2::TimeStepping::Rule;
  using Stability = exahype2::TimeStepping::Stability;

  // The test runner traps every ordered comparison against a NaN. The faulty wave speeds are a
  // negative and an infinite value.
  constexpr double NegativeWaveSpeed = -1.25;
  constexpr double InfiniteWaveSpeed = std::numeric_limits<double>::infinity();

  std::uint64_t getBitsOf(const double value) { return std::bit_cast<std::uint64_t>(value); }

  exahype2::TimeStepping makeAdaptive(const Stability stability) {
    return exahype2::TimeStepping{Rule::Adaptive, 1.0, stability};
  }

  void placeBeforeTheTerminalTime(exahype2::TimeStepping& timeStepping, const double terminalTime) {
    exahype2::Limits limits;
    limits.terminalTime = terminalTime;
    timeStepping.setLimits(limits);
    timeStepping.advanceTimeStamp(1.0);
  }

  void reduceOverTheRanks(exahype2::TimeStepping& timeStepping) {
    tarch::mpi::ReductionRecord record;
    timeStepping.addLocalValuesToGlobalReduction(record);
    record.allreduce(tarch::mpi::Rank::getInstance().getCommunicator());
    timeStepping.readValuesFromGlobalReduction(record);
  }
} // namespace

UNIT_TEST("The reduction of two physical wave speeds returns the bits of their maximum, signed zeroes included") {
  GIVEN("The physical wave speeds minus zero, zero, the smallest subnormal number, one half, one and two") {
    static constexpr std::array PhysicalWaveSpeeds{-0.0, 0.0, std::numeric_limits<double>::denorm_min(), 0.5, 1.0, 2.0};

    WHEN("The reduction returns its identity and combines every ordered pair of the wave speeds") {
      const double                     identity = exahype2::MaxEigenvalueReduction::identity();
      std::vector<std::vector<double>> combinations;
      for (const double left : PhysicalWaveSpeeds) {
        std::vector<double>& combinationsWithLeft = combinations.emplace_back();
        for (const double right : PhysicalWaveSpeeds) {
          combinationsWithLeft.push_back(exahype2::MaxEigenvalueReduction::combine(left, right));
        }
      }

      THEN("The identity is positive zero, every speed is physical and every pair gives the bits of its maximum") {
        VALIDATE_EQUALS(getBitsOf(identity), getBitsOf(0.0));
        for (std::size_t leftIndex = 0; leftIndex < PhysicalWaveSpeeds.size(); leftIndex++) {
          const double left = PhysicalWaveSpeeds[leftIndex];
          VALIDATE(exahype2::MaxEigenvalueReduction::isPhysicalValue(left));
          for (std::size_t rightIndex = 0; rightIndex < PhysicalWaveSpeeds.size(); rightIndex++) {
            const double right = PhysicalWaveSpeeds[rightIndex];
            VALIDATE_EQUALS(
              getBitsOf(combinations[leftIndex][rightIndex]),
              getBitsOf(std::max(left, right)),
              left,
              right
            );
          }
        }
      }
    }
  }
}

UNIT_TEST("A faulty wave speed survives the reduction in either operand position and in every later reduction") {
  GIVEN("A negative and an infinite wave speed") {
    static constexpr std::array FaultyWaveSpeeds{NegativeWaveSpeed, InfiniteWaveSpeed};

    WHEN("Every fault combines with one in both operand positions and with the identity and one result with two") {
      std::vector<double> combinedAsRightOperand;
      std::vector<double> combinedAsLeftOperand;
      std::vector<double> combinedAgainWithTwo;
      std::vector<double> combinedWithTheIdentity;
      for (const double fault : FaultyWaveSpeeds) {
        combinedAsRightOperand.push_back(exahype2::MaxEigenvalueReduction::combine(1.0, fault));
        combinedAsLeftOperand.push_back(exahype2::MaxEigenvalueReduction::combine(fault, 1.0));
        combinedAgainWithTwo.push_back(exahype2::MaxEigenvalueReduction::combine(combinedAsRightOperand.back(), 2.0));
        combinedWithTheIdentity.push_back(
          exahype2::MaxEigenvalueReduction::combine(exahype2::MaxEigenvalueReduction::identity(), fault)
        );
      }

      THEN("isPhysicalValue() rejects every fault and every combination returns the bits of its fault") {
        for (std::size_t index = 0; index < FaultyWaveSpeeds.size(); index++) {
          const double fault = FaultyWaveSpeeds[index];
          VALIDATE(not exahype2::MaxEigenvalueReduction::isPhysicalValue(fault), fault);
          VALIDATE_EQUALS(getBitsOf(combinedAsRightOperand[index]), getBitsOf(fault), fault);
          VALIDATE_EQUALS(getBitsOf(combinedAsLeftOperand[index]), getBitsOf(fault), fault);
          VALIDATE_EQUALS(getBitsOf(combinedAgainWithTwo[index]), getBitsOf(fault), fault);
          VALIDATE_EQUALS(getBitsOf(combinedWithTheIdentity[index]), getBitsOf(fault), fault);
        }
      }
    }
  }
}

UNIT_TEST("A new time stepping stands at the time zero without mesh widths and without terminal time") {
  GIVEN("A new adaptive time stepping with the factor one") {
    const exahype2::TimeStepping timeStepping = makeAdaptive(Stability::PhysicalEigenvalue);

    THEN("The time stamp and both step sizes are zero and the factor is one") {
      VALIDATE_NUMERICAL_EQUALS(timeStepping.getTimeStamp(), 0.0);
      VALIDATE_NUMERICAL_EQUALS(timeStepping.getTimeStepSize(), 0.0);
      VALIDATE_NUMERICAL_EQUALS(timeStepping.getTimeStepSizeOfLastStep(), 0.0);
      VALIDATE_NUMERICAL_EQUALS(timeStepping.getFactor(), 1.0);
    }

    THEN("The mesh widths are infinite and zero and the terminal time is the largest double") {
      VALIDATE(std::isinf(timeStepping.getMinMeshWidth()));
      VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxMeshWidth(), 0.0);
      VALIDATE_EQUALS(timeStepping.getLimits().terminalTime, std::numeric_limits<double>::max());
    }
  }
}

UNIT_TEST("The limits of a time stepping read back their values") {
  GIVEN("An adaptive time stepping and limits of a terminal time, two patch sizes and a mesh width") {
    exahype2::TimeStepping timeStepping = makeAdaptive(Stability::PhysicalEigenvalue);
    exahype2::Limits       limits;
    limits.terminalTime           = 2.0;
    limits.minAdmissiblePatchSize = 0.125;
    limits.maxAdmissiblePatchSize = 0.5;
    limits.maxAdmissibleMeshWidth = 0.25;

    WHEN("The time stepping takes the limits") {
      timeStepping.setLimits(limits);

      THEN("Every limit reads back with its value") {
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getLimits().terminalTime, 2.0);
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getLimits().minAdmissiblePatchSize, 0.125);
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getLimits().maxAdmissiblePatchSize, 0.5);
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getLimits().maxAdmissibleMeshWidth, 0.25);
      }
    }
  }
}

UNIT_TEST("Advancing the time stamp adds the step size and records it as the size of the last step") {
  GIVEN("An adaptive time stepping at the time zero") {
    exahype2::TimeStepping timeStepping = makeAdaptive(Stability::PhysicalEigenvalue);

    WHEN("The time stamp advances by one quarter and then by one eighth") {
      timeStepping.advanceTimeStamp(0.25);
      timeStepping.advanceTimeStamp(0.125);

      THEN("The time stamp is the sum of both steps and the size of the last step is one eighth") {
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getTimeStamp(), 0.375);
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getTimeStepSizeOfLastStep(), 0.125);
      }
    }
  }
}

UNIT_TEST("The mesh widths reduce to the smallest minimum and the largest maximum over every rank") {
  GIVEN("An adaptive time stepping, the rank and the number of ranks") {
    const auto             rank          = static_cast<double>(tarch::mpi::Rank::getInstance().getRank());
    const auto             numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
    exahype2::TimeStepping timeStepping  = makeAdaptive(Stability::PhysicalEigenvalue);

    WHEN("Every rank sets the mesh widths one plus its rank and ten plus its rank and the ranks reduce them") {
      timeStepping.setLocalMeshWidths(1.0 + rank, 10.0 + rank);
      reduceOverTheRanks(timeStepping);

      THEN("The minimum mesh width is one and the maximum mesh width is nine plus the number of ranks") {
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getMinMeshWidth(), 1.0);
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxMeshWidth(), 9.0 + numberOfRanks);
      }
    }
  }
}

UNIT_TEST("The reduction reports a value as new only if a rank set it since the previous reduction") {
  GIVEN("An adaptive time stepping") {
    exahype2::TimeStepping timeStepping = makeAdaptive(Stability::PhysicalEigenvalue);

    WHEN("The ranks reduce before any rank sets a value") {
      reduceOverTheRanks(timeStepping);

      THEN("The reduction keeps the mesh widths and the eigenvalue maxima") {
        VALIDATE(not timeStepping.hasNewMeshWidths());
        VALIDATE(not timeStepping.hasNewMaxEigenvalues());
      }

      AND_WHEN("Every rank sets the mesh widths one and two and the ranks reduce them") {
        timeStepping.setLocalMeshWidths(1.0, 2.0);
        reduceOverTheRanks(timeStepping);

        THEN("The reduction reports new mesh widths only") {
          VALIDATE(timeStepping.hasNewMeshWidths());
          VALIDATE(not timeStepping.hasNewMaxEigenvalues());
        }

        AND_WHEN("Every rank raises the maximum eigenvalue to three and the ranks reduce it") {
          timeStepping.raiseMaxEigenvalue(3.0);
          reduceOverTheRanks(timeStepping);

          THEN("The reduction reports new eigenvalue maxima only and keeps the mesh widths") {
            VALIDATE(not timeStepping.hasNewMeshWidths());
            VALIDATE(timeStepping.hasNewMaxEigenvalues());
            VALIDATE_NUMERICAL_EQUALS(timeStepping.getMinMeshWidth(), 1.0);
            VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxEigenvalue(), 3.0);
          }
        }
      }
    }
  }
}

UNIT_TEST("The eigenvalue maxima of a step keep the largest raised value, even a tiny positive one") {
  GIVEN("An adaptive time stepping and the small eigenvalue 1e-15") {
    exahype2::TimeStepping timeStepping    = makeAdaptive(Stability::PhysicalEigenvalue);
    constexpr double       SmallEigenvalue = 1e-15;

    THEN("The reduced maximum eigenvalue is zero") { VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxEigenvalue(), 0.0); }

    WHEN("Both maxima of the step rise to the small eigenvalue") {
      timeStepping.raiseMaxEigenvalue(SmallEigenvalue);
      timeStepping.raiseMaxScaledEigenvalue(SmallEigenvalue);

      THEN("Both maxima equal the small eigenvalue and the stability eigenvalue is positive") {
        VALIDATE_EQUALS(timeStepping.getMaxEigenvalueOfThisStep(), SmallEigenvalue);
        VALIDATE_EQUALS(timeStepping.getMaxScaledEigenvalueOfThisStep(), SmallEigenvalue);
        VALIDATE_EQUALS(timeStepping.hasPositiveStabilityEigenvalue(), true);
      }

      AND_WHEN("The eigenvalues 0.5, 2, 1 and 0 and the scaled eigenvalues 3 and 4 raise the maxima") {
        timeStepping.raiseMaxEigenvalue(0.5);
        timeStepping.raiseMaxEigenvalue(2.0);
        timeStepping.raiseMaxEigenvalue(1.0);
        timeStepping.raiseMaxEigenvalue(0.0);
        timeStepping.raiseMaxScaledEigenvalue(3.0);
        timeStepping.raiseMaxScaledEigenvalue(4.0);

        THEN("The maxima of the step are two and four and the reduced maximum stays zero") {
          VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxEigenvalueOfThisStep(), 2.0);
          VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxScaledEigenvalueOfThisStep(), 4.0);
          VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxEigenvalue(), 0.0);
        }
      }
    }
  }
}

UNIT_TEST("Starting a time step commits the proposed step size and clears the eigenvalue maxima of the step") {
  GIVEN("An adaptive time stepping at the time one before the terminal time ten, with the maxima two and three") {
    exahype2::TimeStepping timeStepping = makeAdaptive(Stability::PhysicalEigenvalue);
    placeBeforeTheTerminalTime(timeStepping, 10.0);
    timeStepping.raiseMaxEigenvalue(2.0);
    timeStepping.raiseMaxScaledEigenvalue(3.0);

    WHEN("The time stepping admits the step size one half") {
      timeStepping.setAdmissibleTimeStepSize(0.5);

      THEN("The step size stays zero and the size of the next step is one half") {
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getTimeStepSize(), 0.0);
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getTimeStepSizeOfNextStep(), 0.5);
      }

      AND_WHEN("The time step starts") {
        const double timeStepSize = timeStepping.startTimeStep();

        THEN("The step takes the size one half and both maxima of the step are zero") {
          VALIDATE_NUMERICAL_EQUALS(timeStepSize, 0.5);
          VALIDATE_NUMERICAL_EQUALS(timeStepping.getTimeStepSize(), 0.5);
          VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxEigenvalueOfThisStep(), 0.0);
          VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxScaledEigenvalueOfThisStep(), 0.0);
        }
      }
    }
  }
}

UNIT_TEST("The rank reduction takes the eigenvalue maxima of all ranks") {
  GIVEN("A time stepping whose stability eigenvalue is the scaled maximum, the rank and the number of ranks") {
    const auto             rank          = static_cast<double>(tarch::mpi::Rank::getInstance().getRank());
    const auto             numberOfRanks = static_cast<double>(tarch::mpi::Rank::getInstance().getNumberOfRanks());
    exahype2::TimeStepping timeStepping  = makeAdaptive(Stability::ScaledEigenvalue);

    WHEN("Every rank raises the maxima to one and ten plus its rank and the ranks reduce them") {
      timeStepping.raiseMaxEigenvalue(1.0 + rank);
      timeStepping.raiseMaxScaledEigenvalue(10.0 + rank);
      reduceOverTheRanks(timeStepping);

      THEN("Every rank receives the maxima of the last rank, the reduced maximum included") {
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxEigenvalue(), numberOfRanks);
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxEigenvalueOfThisStep(), numberOfRanks);
        VALIDATE_NUMERICAL_EQUALS(timeStepping.getMaxScaledEigenvalueOfThisStep(), 9.0 + numberOfRanks);
      }
    }
  }
}

UNIT_TEST("The stability eigenvalue is the physical or the scaled maximum, as the stability measure selects") {
  GIVEN("An adaptive time stepping whose stability eigenvalue is the physical maximum") {
    exahype2::TimeStepping physical = makeAdaptive(Stability::PhysicalEigenvalue);

    WHEN("The scaled maximum rises to one") {
      physical.raiseMaxScaledEigenvalue(1.0);

      THEN("The stability eigenvalue is not positive") {
        VALIDATE_EQUALS(physical.hasPositiveStabilityEigenvalue(), false);
      }

      AND_WHEN("The physical maximum rises to one") {
        physical.raiseMaxEigenvalue(1.0);

        THEN("The stability eigenvalue is positive") {
          VALIDATE_EQUALS(physical.hasPositiveStabilityEigenvalue(), true);
        }
      }
    }
  }

  GIVEN("An adaptive time stepping whose stability eigenvalue is the scaled maximum") {
    exahype2::TimeStepping scaled = makeAdaptive(Stability::ScaledEigenvalue);

    WHEN("The physical maximum rises to one") {
      scaled.raiseMaxEigenvalue(1.0);

      THEN("The stability eigenvalue is not positive") {
        VALIDATE_EQUALS(scaled.hasPositiveStabilityEigenvalue(), false);
      }

      AND_WHEN("The scaled maximum rises to one") {
        scaled.raiseMaxScaledEigenvalue(1.0);

        THEN("The stability eigenvalue is positive") { VALIDATE_EQUALS(scaled.hasPositiveStabilityEigenvalue(), true); }
      }
    }
  }
}

UNIT_TEST("The proposed step size is the CFL step size or the distance to the terminal time when that is smaller") {
  GIVEN("An adaptive time stepping at the time one before the terminal time 1.25") {
    exahype2::TimeStepping nearTheEnd = makeAdaptive(Stability::PhysicalEigenvalue);
    placeBeforeTheTerminalTime(nearTheEnd, 1.25);

    WHEN("The time stepping admits the CFL step size one half and starts the step") {
      nearTheEnd.setAdmissibleTimeStepSize(0.5);
      const double timeStepSize = nearTheEnd.startTimeStep();

      THEN("The step size is the distance one quarter to the terminal time") {
        VALIDATE_NUMERICAL_EQUALS(timeStepSize, 0.25);
      }
    }
  }

  GIVEN("An adaptive time stepping at the time one before the terminal time three") {
    exahype2::TimeStepping farFromTheEnd = makeAdaptive(Stability::PhysicalEigenvalue);
    placeBeforeTheTerminalTime(farFromTheEnd, 3.0);

    WHEN("The time stepping admits the CFL step size one half and starts the step") {
      farFromTheEnd.setAdmissibleTimeStepSize(0.5);
      const double timeStepSize = farFromTheEnd.startTimeStep();

      THEN("The step size is the CFL step size one half") { VALIDATE_NUMERICAL_EQUALS(timeStepSize, 0.5); }
    }
  }
}

UNIT_TEST("A CFL step size within rounding of the remaining time grows to the remaining time") {
  GIVEN("An adaptive time stepping at the time one before the terminal time two") {
    exahype2::TimeStepping withinRoundingOfTheEnd = makeAdaptive(Stability::PhysicalEigenvalue);
    placeBeforeTheTerminalTime(withinRoundingOfTheEnd, 2.0);

    WHEN("The time stepping admits the CFL step size one minus 1e-12 and starts the step") {
      withinRoundingOfTheEnd.setAdmissibleTimeStepSize(1.0 - 1.0e-12);
      const double timeStepSize = withinRoundingOfTheEnd.startTimeStep();

      THEN("The step size is the remaining time one bitwise") { VALIDATE_EQUALS(timeStepSize, 1.0); }
    }
  }
}

UNIT_TEST("A fixed rule keeps its step size across steps") {
  GIVEN("A time stepping with the fixed rule and the factor one half") {
    exahype2::TimeStepping fixed{Rule::Fixed, 0.5, Stability::PhysicalEigenvalue};

    THEN("The step size is zero") { VALIDATE_NUMERICAL_EQUALS(fixed.getTimeStepSize(), 0.0); }

    WHEN("The test sets the fixed step size one quarter") {
      fixed.setFixedTimeStepSize(0.25);

      THEN("The step size and the size of the next step are one quarter") {
        VALIDATE_NUMERICAL_EQUALS(fixed.getTimeStepSize(), 0.25);
        VALIDATE_NUMERICAL_EQUALS(fixed.getTimeStepSizeOfNextStep(), 0.25);
      }

      AND_WHEN("Two time steps start") {
        const double firstTimeStepSize  = fixed.startTimeStep();
        const double secondTimeStepSize = fixed.startTimeStep();

        THEN("Both steps take the size one quarter") {
          VALIDATE_NUMERICAL_EQUALS(firstTimeStepSize, 0.25);
          VALIDATE_NUMERICAL_EQUALS(secondTimeStepSize, 0.25);
        }
      }
    }
  }
}
