// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include <string>

/**
 * @page tarch_NoncriticalAssertions Noncritical assertions
 *
 * @brief A noncritical assertion is an assertion which indicates that
 * something went horribly wrong due to a programming error, but the code
 * should still continue with this mesh traversal, then dump the data, and
 * eventually terminate.
 *
 *
 * You should never use an assertion
 * for user errors, as assertions are compiled out from the code in release
 * builds. Usual assertions make the code stop immediately. For many numerical
 * bugs, this is not very useful. Instead, you can trigger a noncritical assertion
 * which informs rank 0 that something went wrong. Rank 0 will now issue a data
 * dump (it ignores whether you have enabled outputs or not, it simply enforces
 * a plot of the grid) and then terminates deterministically. This way, you can
 * inspect the solution just when the assertion had been violated.
 *
 *
 * ## Behaviour
 *
 * As a noncritical assertion does not immediately terminate the code, you might
 * get a whole sequence of these guys in a row. Skip them all and read only the
 * first one in this case. I try to filter out the messages, i.e. to display only
 * the first one, but as the ranks try not to synchronise where possible, I can
 * only filter error messages from one rank. That is, you'll still get up to one
 * error/assertion message per rank.
 *
 *
 * ## Explicit triggering and behaviour in release mode
 *
 * The environment to handle non-critical assertions is always up, no matter if
 * you run in release mode or not. Therefore, trigger a non-critical assertion
 * explicitly through a call to triggerNonCriticalAssertion(). This call is not
 * removed by release builds.
 *
 *
 * ## Realisation
 *
 * Logically, the assertions are implemented via a single integer. As long as
 * this integer is zero, all is fine. If a non-critical assertion fails, we
 * set the variable - which is local per rank - to the number of the failing
 * rank.
 * The global simulation driver, i.e. main, has to check the flag prior to
 * every solver step. If the flag is non-zero, it has to stop and instead dump
 * the solution. After that, it has to quit.
 *
 * The crucial question here remains how you sync the individual ranks. If a
 * rank goes down, it has to notify rank 0 that it had an issue, as rank 0
 * makes the global decisions.
 *
 * I did play around with RMA and windows to realise the information
 * propagation. My idea had been that any rank where a non-critical assertion
 * arises dumps a flag into the window on rank 0 and rank 0 then takes it from
 * there. This approach does not work straightforwardly, as you always need
 * some synchronisation (fences, e.g.) in MPI.
 *
 * Therefore, I decided to switch back to old-fashioned P2P messages. The rank
 * that observes a failing assertion records this assertion and sends a message
 * to rank 0. Whenever rank 0 is asked if a problem has popped up, it first
 * looks into its own local variable (did a non-critical assertion arise on
 * rank 0), and then it looks if there's an assertion message in the queue. To
 * make this approach work, we need a dedicated assertion tag which is not used
 * for any other purpose: After all, the assertion message logically might have
 * to overtook other messages.
 *
 *
 * ## Modifications of main file
 *
 * If you want to support non-critical assertions, you have to alter your main
 * loop to check for the flag every now and then. This is not done
 * automatically, though some extensions such as Swift or ExaHyPE bring up the
 * environment for non-critical assertions up by default.
 *
 * Usually, the main driver's code snippet looks as follows:
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   if (tarch::hasNonCriticalAssertionBeenViolated() and not haveReceivedNoncriticialAssertion) {
 *     peano4::parallel::Node::getInstance().setNextProgramStep(
 *       repositories::StepRepository::toProgramStep( repositories::StepRepository::Steps::Plot )
 *   );
 *    haveReceivedNoncriticialAssertion = true;
 *    logError("non-critical assertion has been triggered in code. Dump final state and terminate"
 *);
 *   }
 *   else if (tarch::hasNonCriticalAssertionBeenViolated()) {
 *     continueToSolve = false;
 *   }
 *   else [...]
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 */
namespace tarch {
  /**
   *
   * peano4::shutdownParallelEnvironment().
   */
  void shutdownNonCriticalAssertionEnvironment();

  /**
   * Register the assertion tag from the global communicator.
   */
  void initNonCriticalAssertionEnvironment();

  /**
   * Switch noncritical assertions on/off.
   */
  void enableNonCriticalAssertions(bool value);

  /**
   * Trigger a non-critical assertion
   *
   * The routine reports only details about the very first non-critical
   * assertion. All follow-up logs on this rank are ignored, i.e. we only
   * get a note and no detail. Once logged locally, we send the assertion
   * message to the global master if we are not working on the global
   * marker ourselves.
   */
  void triggerNonCriticalAssertion(std::string file, int line, std::string expression, std::string parameterValuePairs);
  bool hasNonCriticalAssertionBeenViolated();
} // namespace tarch
