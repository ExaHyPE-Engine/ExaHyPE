#include "DependencyChecks.h"


std::string swift2::dependencychecks::toString(Invariant policy) {
  switch (policy) {
    case Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce:
      return "touch-first-usage-mask-out-afterwards-all-previous-steps-update-at-least-once";
    case Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun:
      return "touch-first-usage-mask-out-afterwards-all-previous-steps-update-at-least-once-sweep-may-rerun";
    case Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce:
      return "touch-at-most-once-mask-out-otherwise-all-previous-steps-update-at-least-once";
    case Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun:
      return "touch-at-most-once-mask-out-otherwise-all-previous-steps-update-at-least-once-sweep-may-rerun";
    case Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce:
      return "touch-at-least-once-all-previous-steps-update-at-least-once";
    case Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep:
      return "touch-at-least-once-all-previous-steps-update-at-least-once-may-overwrite-previous-cell-updates-from-same-sweep";
  }
  return "<undef>";
}


bool swift2::dependencychecks::internal::isValid(
  Invariant policy,
  bool workOnParticle,
  bool particleIsLocal,
  int numberOfUpdates,
  int numberOfMaskOuts
) {
  bool result = false;

  if (workOnParticle and not particleIsLocal) {
    result = false;
  }

  else if (not workOnParticle and not particleIsLocal) {
    result = true;
  }

  else if (
    policy == Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun
    or
    policy == Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun
  ) {
    result = true;
  }

  else if (
    policy == Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce
  ) {
    result = (workOnParticle and numberOfUpdates == 1 and numberOfMaskOuts == 0)
             or (not workOnParticle and numberOfUpdates == 1 and numberOfMaskOuts > 0);
  }

  else if (policy == Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce or policy == Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep) {
    result = (workOnParticle and numberOfUpdates >= 1) or (not workOnParticle and numberOfUpdates >= 0);
  }

  else if (
    policy == Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce
  ) {
    result = (numberOfUpdates <= 1 and numberOfMaskOuts >= 0);
  }

  return result;
}


bool swift2::dependencychecks::internal::previousStepsAllHaveToAccessAtLeastOnce(Invariant policy) {
  switch (policy) {
    case Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce:
    case Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun:
    case Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce:
    case Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun:
    case Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce:
    case Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep:
      return true;
  }
  return true;
}


int swift2::dependencychecks::internal::firstFutureStepThatHasToBeCleared(Invariant policy, int numberOfSteps) {
  switch (policy) {
    case Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce:
      return 0;
    case Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun:
      return 1;
    case Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce:
      return 0;
    case Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce_SweepMayRerun:
      return 1;
    case Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce:
      return 0;
    case Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce_MayOverwritePreviousCellUpdatesFromSameSweep:
      return numberOfSteps;
  }
  return true;
}


int swift2::dependencychecks::internal::checkFutureStages(
  Invariant  policy,
  const int  currentStep,
  const int  currentStage,
  const int  nsteps,
  const int  nstages,
  const int* sweepStateProgression
) {
  int errors = 0;
  int firstFutureStepToCheck = firstFutureStepThatHasToBeCleared(policy,nsteps);

  for (int step = currentStep + (firstFutureStepToCheck==0 ? 1 : firstFutureStepToCheck); step < nsteps; step++) {
    for (int stage = 0; stage < nstages; stage++) {
      const int index     = step * nstages + stage;
      const int partStage = sweepStateProgression[index];

      if (partStage<0) errors++;
    }
  }

  // Check everything *after* this stage, and check that everything
  // that shouldn't have run until now actually hasn't run.
  for (int step = currentStep + (firstFutureStepToCheck==0 ? 1 : firstFutureStepToCheck); step < nsteps; step++) {
    for (int stage = 0; stage < nstages; stage++) {
      const int index     = step * nstages + stage;
      const int partStage = sweepStateProgression[index];

      if (partStage != 0) errors++;
    }
  }

  if (firstFutureStepToCheck==0) {
    // for the current sweep, only check beginning with the next stage.
    for (int stage = currentStage + 1; stage < nstages; stage++) {

      const int index     = currentStep * nstages + stage;
      const int partStage = sweepStateProgression[index];

      if (partStage != 0) errors++;
    }
  }

  return errors;
}


int swift2::dependencychecks::internal::checkPastStages(
  Invariant  policy,
  bool       isLocal,
  const int  currentStep,
  const int  currentStage,
  const int  nsteps,
  const int  nstages,
  const int* sweepStateProgression
) {
  int errors = 0;

  for (int step = 0; step < currentStep; step++) {
    for (int stage = 0; stage < nstages; stage++) {
      const int index     = step * nstages + stage;
      const int partStage = sweepStateProgression[index];

      if (partStage<0) errors++;
    }
  }

  if (isLocal and previousStepsAllHaveToAccessAtLeastOnce(policy)) {
    // Check everything else before this stage, and check that everything
    // that should've run until now actually has run.
    for (int step = 0; step < currentStep; step++) {
      for (int stage = 0; stage < nstages; stage++) {
        const int index     = step * nstages + stage;
        const int partStage = sweepStateProgression[index];

        if (partStage == 0) errors++;
      }
    }


    // for the current sweep, only check up to the current stage.
    for (int stage = 0; stage < currentStage; stage++) {
      const int index     = currentStep * nstages + stage;
      const int partStage = sweepStateProgression[index];

      if (partStage == 0) errors++;
    }
  }

  return errors;
}
