// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_NON_CRITICAL_ASSERTIONS_H_
#define _EXAHYPE2_NON_CRITICAL_ASSERTIONS_H_


#include <sstream>
#include <string>


/**
 * @page Noncritical Assertions
 *
 * A noncritical assertion is an assertion which indicates that something went
 * horribly wrong due to a programming error. You should never use an assertion
 * for user errors, as assertions are compiled out from teh code in release
 * builds. Usual assertions make the code stop immediately. For many numerical
 * bugs, this is not very useful. Instead, you can trigger a noncritical assertion
 * which informs rank 0 that something went wrong. Rank 0 will now issue a data
 * dump (it ignores whether you have enabled outputs or not, it simply enforces
 * a plot of the grid) and then terminates deterministically. This way, you can
 * inspect the solution just when the assertion had been violated.
 *
 * As a noncritical assertion does not immediately terminate the code, you might
 * get a whole sequence of these guys in a row. Skip them all and read only the
 * first one in this case. I try to filter out the messages, i.e. to display only
 * the first one, but as the ranks try not to synchronise where possible, I can
 * only filter error messages from one rank. That is, you'll still get up to one
 * error/assertion message per rank.
 *
 * As noncritical assertions rely internally on MPI RMA, you have to call init
 * and shutdown for the environment. Do so immediately after you've initialised
 * MPI or just before you shut it down, respectively.
 */
namespace exahype2 {
  /**
   * The management of the critical assertions relies on a shared memory region
   * (windows). To free the window, all ranks have to enter this routine at the
   * same time. In Peano 4, you hence have to call this routine after the Node
   * is shutdown(). Otherwise, the main rank would run into this routine, but the
   * other ranks migth not know that they have to go down, too. But please call
   * it before you shut down the total code, i.e. before you invoke
   * peano4::shutdownParallelEnvironment().
   */
  void shutdownNonCritialAssertionEnvironment();
  void initNonCritialAssertionEnvironment();

  /**
   * Switch noncritical assertions on/off.
   */
  void enableNonCriticalAssertions(bool value);

  void triggerNonCriticalAssertion( std::string file, int line, std::string expression, std::string parameterValuePairs );
  bool hasNonCriticalAssertionBeenViolated();
}



#if PeanoDebug>=2
#define nonCriticalAssertion(expr) if (!(expr)) { \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, "" ); \
}


#define nonCriticalAssertion1(expr,param0) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion2(expr,param0,param1) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion3(expr,param0,param1,param2) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion4(expr,param0,param1,param2,param3) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion5(expr,param0,param1,param2,param3,param4) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  msg << "parameter " << #param4 << ": " << param4 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion6(expr,param0,param1,param2,param3,param4,param5) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  msg << "parameter " << #param4 << ": " << param4 << std::endl; \
  msg << "parameter " << #param5 << ": " << param5 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion7(expr,param0,param1,param2,param3,param4,param5,param6) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  msg << "parameter " << #param4 << ": " << param4 << std::endl; \
  msg << "parameter " << #param5 << ": " << param5 << std::endl; \
  msg << "parameter " << #param6 << ": " << param6 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion8(expr,param0,param1,param2,param3,param4,param5,param6,param7) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  msg << "parameter " << #param4 << ": " << param4 << std::endl; \
  msg << "parameter " << #param5 << ": " << param5 << std::endl; \
  msg << "parameter " << #param6 << ": " << param6 << std::endl; \
  msg << "parameter " << #param7 << ": " << param7 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion9(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  msg << "parameter " << #param4 << ": " << param4 << std::endl; \
  msg << "parameter " << #param5 << ": " << param5 << std::endl; \
  msg << "parameter " << #param6 << ": " << param6 << std::endl; \
  msg << "parameter " << #param7 << ": " << param7 << std::endl; \
  msg << "parameter " << #param8 << ": " << param8 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion10(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8,param9) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  msg << "parameter " << #param4 << ": " << param4 << std::endl; \
  msg << "parameter " << #param5 << ": " << param5 << std::endl; \
  msg << "parameter " << #param6 << ": " << param6 << std::endl; \
  msg << "parameter " << #param7 << ": " << param7 << std::endl; \
  msg << "parameter " << #param8 << ": " << param8 << std::endl; \
  msg << "parameter " << #param9 << ": " << param9 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion11(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  msg << "parameter " << #param4 << ": " << param4 << std::endl; \
  msg << "parameter " << #param5 << ": " << param5 << std::endl; \
  msg << "parameter " << #param6 << ": " << param6 << std::endl; \
  msg << "parameter " << #param7 << ": " << param7 << std::endl; \
  msg << "parameter " << #param8 << ": " << param8 << std::endl; \
  msg << "parameter " << #param9 << ": " << param9 << std::endl; \
  msg << "parameter " << #param10 << ": " << param10 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion12(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,param11) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  msg << "parameter " << #param4 << ": " << param4 << std::endl; \
  msg << "parameter " << #param5 << ": " << param5 << std::endl; \
  msg << "parameter " << #param6 << ": " << param6 << std::endl; \
  msg << "parameter " << #param7 << ": " << param7 << std::endl; \
  msg << "parameter " << #param8 << ": " << param8 << std::endl; \
  msg << "parameter " << #param9 << ": " << param9 << std::endl; \
  msg << "parameter " << #param10 << ": " << param10 << std::endl; \
  msg << "parameter " << #param11 << ": " << param11 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion13(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,param11,param12) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  msg << "parameter " << #param4 << ": " << param4 << std::endl; \
  msg << "parameter " << #param5 << ": " << param5 << std::endl; \
  msg << "parameter " << #param6 << ": " << param6 << std::endl; \
  msg << "parameter " << #param7 << ": " << param7 << std::endl; \
  msg << "parameter " << #param8 << ": " << param8 << std::endl; \
  msg << "parameter " << #param9 << ": " << param9 << std::endl; \
  msg << "parameter " << #param10 << ": " << param10 << std::endl; \
  msg << "parameter " << #param11 << ": " << param11 << std::endl; \
  msg << "parameter " << #param12 << ": " << param12 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}


#define nonCriticalAssertion14(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,param11,param12,param13) if (!(expr)) { \
  std::ostringstream msg; \
  msg << "parameter " << #param0 << ": " << param0 << std::endl; \
  msg << "parameter " << #param1 << ": " << param1 << std::endl; \
  msg << "parameter " << #param2 << ": " << param2 << std::endl; \
  msg << "parameter " << #param3 << ": " << param3 << std::endl; \
  msg << "parameter " << #param4 << ": " << param4 << std::endl; \
  msg << "parameter " << #param5 << ": " << param5 << std::endl; \
  msg << "parameter " << #param6 << ": " << param6 << std::endl; \
  msg << "parameter " << #param7 << ": " << param7 << std::endl; \
  msg << "parameter " << #param8 << ": " << param8 << std::endl; \
  msg << "parameter " << #param9 << ": " << param9 << std::endl; \
  msg << "parameter " << #param10 << ": " << param10 << std::endl; \
  msg << "parameter " << #param11 << ": " << param11 << std::endl; \
  msg << "parameter " << #param12 << ": " << param12 << std::endl; \
  msg << "parameter " << #param13 << ": " << param13 << std::endl; \
  ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, #expr, msg.str() ); \
}
#else
#define nonCriticalAssertion(expr)
#define nonCriticalAssertion1(expr,param0)
#define nonCriticalAssertion2(expr,param0,param1)
#define nonCriticalAssertion3(expr,param0,param1,param2)
#define nonCriticalAssertion4(expr,param0,param1,param2,param3)
#define nonCriticalAssertion5(expr,param0,param1,param2,param3,param4)
#define nonCriticalAssertion6(expr,param0,param1,param2,param3,param4,param5)
#define nonCriticalAssertion7(expr,param0,param1,param2,param3,param4,param5,param6)
#define nonCriticalAssertion8(expr,param0,param1,param2,param3,param4,param5,param6,param7)
#define nonCriticalAssertion9(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8)
#define nonCriticalAssertion10(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8,param9)
#define nonCriticalAssertion11(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10)
#define nonCriticalAssertion12(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,param11)
#define nonCriticalAssertion13(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,param11,param12)
#define nonCriticalAssertion14(expr,param0,param1,param2,param3,param4,param5,param6,param7,param8,param9,param10,param11,param12,param13)
#endif


#endif

