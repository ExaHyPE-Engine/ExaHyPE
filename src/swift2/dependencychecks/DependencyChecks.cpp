#include "../dependencychecks/DependencyChecks.h"


std::string swift2::dependencychecks::toString( Invariant policy ) {
  switch (policy) {
    case Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce:
      return "touch-first-usage-mask-out-afterwards-all-previous-steps-update-at-least-once";
    case Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce:
      return "touch-at-most-once-mask-out-otherwise-all-previous-steps-update-at-least-once";
    case Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce:
      return "touch-at-least-once-all-previous-steps-update-at-least-once";
  }
  return "<undef>";
}


bool swift2::dependencychecks::internal::isValid( Invariant policy, bool workOnParticle, int numberOfUpdates, int numberOfMaskOuts ) {
  bool result = false;

  if ( policy == Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce ) {
    result = (workOnParticle and numberOfUpdates==1 and numberOfMaskOuts==0)
             or
             (not workOnParticle and numberOfUpdates==1 and numberOfMaskOuts>0);
  }

  if ( policy == Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce ) {
    result = (workOnParticle and numberOfUpdates>=1)
             or
             (not workOnParticle and numberOfUpdates>=0);
  }

  if ( policy == Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce ) {
    result = (numberOfUpdates<=1 and numberOfMaskOuts>=0);
  }

  return result;
}


bool swift2::dependencychecks::internal::previousStepsAllHaveToAccessAtLeastOnce( Invariant policy ) {
  switch (policy) {
    case Invariant::TouchFirstUsage_MaskOutAfterwards_AllPreviousStepsUpdateAtLeastOnce:
      return true;
    case Invariant::TouchAtMostOnce_MaskOutOtherwise_AllPreviousStepsUpdateAtLeastOnce:
      return true;
    case Invariant::TouchAtLeastOnce_AllPreviousStepsUpdateAtLeastOnce:
      return true;
  }
  return true;
}
