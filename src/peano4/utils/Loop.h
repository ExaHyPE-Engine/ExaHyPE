// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <bitset>

#include "config.h"
#include "peano4/utils/Globals.h"
#include "tarch/compiler/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

#if defined(SharedTBB)
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/blocked_range2d.h>
#include <oneapi/tbb/blocked_range3d.h>
#include <oneapi/tbb/parallel_for.h>

#include "tbb/blocked_rangeNd.h"
#endif

#if defined(SharedSYCL)
#include "tarch/multicore/sycl/Core.h"

/**
 * @see peano4::utils::dLinearised() and @ref tarch_accelerator_SYCL
 */
#define CPUGPUMethod SYCL_EXTERNAL
#else
#define CPUGPUMethod
#endif

namespace peano4 {
  namespace utils {
    /**
     * Guide loop-level parallelism
     *
     * Peano's loop macros allow users to define the logical concurrency
     * of loops: Is it serial (with dependencies), can it run in parallel
     * (though there might be dependencies/atomics/critical sections) or
     * is a loop of SIMT/SIMD type, i.e. without any dependencies.
     *
     * Further to that, codes can guide the placement of the loops:
     *
     * Value  | Semantics for parallel for | Semantics for simt loop
     * -------|----------------------------|---------------------------
     * Serial | Keep it on one core.       | Keep it on one GPU thread
     *        |                            | or don't use AVX.
     * Nested | If an outer parallel loop  | Stick to one SM or one
     *        | grabs a core or n cores,   | core's AVX units.
     *        | stick to these n cores.    |
     * SpreadOut | Try to grab additional  | Try to use multiple SMs.
     *        | cores outside of an        |
     *        | enclosing parallel region. |
     *
     * The flags help performance engineers to balance between overheads
     * and the maximum concurrency that's made available to a parallel
     * or SIMT loop.
     */
    enum class LoopPlacement { Serial, Nested, SpreadOut };

    /**
     * Is used by the z-loop. See macro dforz.
     */
    typedef std::bitset<Dimensions> LoopDirection;

    /**
     * d-dimensional counterpart of increment operator
     *
     * This operation performs a d-dimensional increment on a given integer vector:
     * The first component of the vector is incremented. If the first component is
     * greater than max-1, the component is set zero and the next component is
     * incremented by one. This operation is used often by d-dimensional for-loops.
     *
     * @see dLinearised() for a discussion of GPU annotation
     */
    CPUGPUMethod void dInc(tarch::la::Vector<Dimensions, int>& counter, int max);

    /**
     * d-dimensional counterpart of decrement operator
     *
     * This operation performs a d-dimensional decrement on a given integer vector:
     * The first component of the vector is decremented. If the first component is
     * smaller than 0, the component is set to max and the next component is
     * decremented by one.
     */
    void dDec(tarch::la::Vector<Dimensions, int>& counter, int max);

    /**
     * d-dimensional counterpart of increment, where individual vector components have different max values
     *
     * This operation performs a d-dimensional increment on a given integer vector:
     * The first component of the vector is incremented. If the first component is
     * greater than max(0)-1, the component is set zero and the next component is
     * incremented by one. This operation is used often by d-dimensional for-loops.
     */
    void dInc(tarch::la::Vector<Dimensions, int>& counter, const tarch::la::Vector<Dimensions, int>& max);

    /**
     * Perform a d-dimensional increment by value increment: The first component
     * of the counter is incremented by increment. Afterwards, the operation
     * checks the first entry: If it exceeds max, its module value is set, the
     * next component is incremented by increment, and the check continues.
     */
    void dIncByVector(tarch::la::Vector<Dimensions, int>& counter, int max, int increment);

    /**
     * Perform a scalar increment of a vector: The operation equals a sequence of
     * increment calls to dInc().
     */
    void dIncByScalar(tarch::la::Vector<Dimensions, int>& counter, int max, int increment);

    /**
     * Same operation as dInc(tarch::la::Vector<Dimensions,int>,int), but now one dimension is not taken
     * into consideration.
     */
    void dInc(tarch::la::Vector<Dimensions, int>& counter, int max, int doNotExamine);

    /**
     * Operation similar to dInc, but is given a direction bitset that identifies
     * whether the counters has to be incremented or decremented. See the dforz
     * macro for an example how to use dInc.
     */
    void dInc(tarch::la::Vector<Dimensions, int>& counter, int max, LoopDirection& direction);

    /**
     * Element-wise comparison for the for d-dimensional for loops
     *
     * Consult dLinearised() for a discussion of GPU aspect. Further to the
     * discussion therein, we have to disable assertions within a SYCL
     * context.
     *
     * @return true if all entries of counter are smaller max
     */
    CPUGPUMethod int dCmp(const tarch::la::Vector<Dimensions, int>& counter, int max);

    /**
     * Element-wise comparison for the for d-dimensional for loops
     *
     * Element-wise comparison for the loops. Different to the other dCmp()
     * operation, this one works fine even if the max range per vector entry is
     * different.
     *
     * @return true if all entries of counter are smaller than their corresponding
     *         entries in max
     */
    int dCmp(const tarch::la::Vector<Dimensions, int>& counter, const tarch::la::Vector<Dimensions, int>& max);

    /**
     * Compares two vectors with regards to their linearised value
     *
     * @returns true, if dLinearised(counter, XXX) < dLinearised(max, XXX)
     */
    bool dCmpLinearOrder(
      const tarch::la::Vector<Dimensions, int>& counter, const tarch::la::Vector<Dimensions, int>& max
    );

    /**
     * Map d-dimensional vector onto integer index
     *
     * This operation is called pretty often and, thus, might cause a significant
     * slowdown in the overall performance. Therefore, I introduced a aggressive
     * optimization based on lookup tables. This optimization is switched on if
     * DLOOP_AGGRESSIVE is specified (default in Peano project). Two preconditions
     * have to be fulfilled in this case: All parameters have to stay within
     * certain boundaries (all positive, max smaller or equal to 5)
     * and one has to call both setupLookupTableForDLinearised() and
     * setupLookupTableForDDelinearised() before using dLinearised() or
     * dDelinearised().
     *
     * Obviously, creating a lookup table for these two operations is not that
     * simple, since the parameter space has to be mapped onto a unique key. To
     * end up with a simple mapping, all the constraints from above are added.
     * Although the mapping might be slow, it is still faster than computing the
     * partial sums of a to the power of b.
     *
     *
     * ## GPU programming
     *
     * The d-linear loops are used mainly on the host, but I provide a SYCL
     * implementation of d-linear, too. SYCL does not really distinguish the
     * host queue from the accelerator queue at the moment. I therefore have to
     * mark the dLinearised() routine as GPU offloadable. See parallelDfor
     * for a discussion of when this is required.
     *
     * Once annotated, we might still get errors if we use SYCL on the host and
     * have no GPU offloading (via SYCL) enabled: The macro GPUCallableMethod
     * in this case is not defined. Actually, it is explicitly defined as
     * empty, so I cannot even overwrite it. Therefore, I introduce the new
     * macro
     *
     *          CPUGPUMethod
     *
     * It delegates to SYCL_EXTERNAL, once SYCL is enabled on the CPU.
     * Consult @ref tarch_accelerator_SYCL as well for further SYCL details.
     *
     *
     * @return the linearisation of the counter, i.e. the k-th component is
     *         multiplied by max^k and the results are accumulated.
     */
    CPUGPUMethod int dLinearised(const tarch::la::Vector<Dimensions, int>& counter, int max);
    CPUGPUMethod int dLinearised(const std::bitset<Dimensions>& counter);

    /**
     * Special 2d variant of dLinearised that works also if you compile with other
     * dimensions.
     */
    int d2Linearised(const tarch::la::Vector<2, int>& counter, int max);

    /**
     * Special 3d variant of dLinearised that works also if you compile with other
     * dimensions.
     */
    int d3Linearised(const tarch::la::Vector<3, int>& counter, int max);

    /**
     * Linearisation not Optimised
     *
     * This operation's semantics equals dLinearised, but the operation is not
     * optimised at all. It thus allows to have arbitrary argument values. Yet,
     * this version is not optimised, i.e. it might become a bottleneck.
     */
    int dLinearisedWithoutLookup(const tarch::la::Vector<Dimensions, int>& counter, int max);

    /**
     * Counterpart of dLinearised().
     *
     * This operation's semantics equals dDeLinearised, but the operation is not
     * optimised at all. It thus allows to have arbitrary argument values. Yet,
     * this version is not optimised, i.e. it might become a bottleneck.
     */
    tarch::la::Vector<Dimensions, int> dDelinearised(int value, int max);

    /**
     * Delinearization not optimised.
     */
    tarch::la::Vector<Dimensions, int> dDelinearisedWithoutLookup(int value, int max);


    void setupLookupTableForDLinearised();
    void setupLookupTableForDDelinearised();

    /**
     * Construct start vector (0,0,....) for d-dimensional loop
     *
     * @see dLinearised() for a discussion of GPU impact.
     *
     * @return a vector containing zero values only.
     */
    CPUGPUMethod tarch::la::Vector<Dimensions, int> dStartVector();

    /**
     * @return a vector containing only zero values besides the dim-th entry. This
     *         entry is set value.
     */
    tarch::la::Vector<Dimensions, int> dStartVector(int dim, int value);

    /**
     * Creates a start vector. Each component is set either 0 or max-1 depending
     * on direction: If direction is true, then the value 0 is zero.
     *
     * @return a start vector for an oscillating loop.
     */
    tarch::la::Vector<Dimensions, int> dStartVector(int max, const LoopDirection& direction);
  }
}


std::string toString(peano4::utils::LoopPlacement);


/**
 * d-dimensional Loop
 *
 *
 * Very often one needs a d-dimensional for loop. A d-dimensional for loop is
 * something like
 * \code
 *   for (x(0)=0; x(0)<N; x(0)++)
 *    for (x(1)=0; x(1)<N; x(1)++)
 *     for (x(2)=0; x(2)<N; x(2)++)
 * \endcode
 * with d nested for loops. Thus, one has to code such loops for every d
 * manually. This macro offers a d-independent alternative, just write
 * \code
 *   dfor (x,N) {
 *   ...
 *   }
 * \endcode
 * The precompiler extracts this macro and within the loop body, you are able
 * to use the integer tinyvector x.
 *
 * Here is an example:
 * \code
 *   dfor(a,2) {
 *     std::cout << a << ",";
 *   }
 * \endcode
 * results in [0,0], [1,0], [0,1], [1,1] if Dimensions equals 2. If DIMENSION
 * equals 3 the same construct gives you [0,0,0], [1,0,0], [0,1,0], [1,1,0],
 * [0,0,1], [1,0,1], [0,1,1], [1,1,1].
 *
 * There are some short-cuts for frequently used iteration ranges, i.e. popular
 * N. Please note that this routine works for quadratic iteration ranges, i.e.
 * if you make N a scalar. It also works if max has the type
 * tarch::la::Vector<Dimensions,int> and hosts different entries per dimension.
 */
#define dfor(counter, max) \
  for (tarch::la::Vector<Dimensions, int> counter = peano4::utils::dStartVector(); peano4::utils::dCmp(counter, max); \
       peano4::utils::dInc(counter, max))


/**
 * @page peano4_utils_Loop_parallel Parallel version of for loops and dfor
 *
 * We offer two main flavours of parallel for macros: a parallel dfor
 * and a dfor mapping onto SIMT, i.e. bare of internal and external
 * dependencies. The latter is subject to some restrictions:
 *
 * - Don't use virtual function calls.
 * - Take into account that the loop body works with copies of local variables
 *   (you can still use pointers).
 * - Don't use synchronisation primitives.
 *
 * All macros want to have a C++ identifier counter. This counter will be a
 * well-defined integer vector within the loop body. The counters always
 * start at 0, but you can shift them obviously internally to have another
 * offset. The max argument is either an integer, or you can paste in a
 * Vector. The usage for the latter looks similar to
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * parallelDforWithSchedulerInstructions(volume,
 * (tarch::la::Vector<2,int>({numberOfVolumesPerAxisInPatch+2*solversHaloSize,numberOfVolumesPerAxisInPatch})),
 * loopPlacement)
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * By using a vector, you can have rectangular iteration ranges and not only
 * squares/cubes. The syntax with the additional brackets is a little bit
 * annoying, but some LLVM front-ends otherwise get confused. Try without
 * them, but don't be surprised if it doesn't work.
 *
 * The mapping onto your variables of choice is very similar to what we see a
 * lot in CUDA and SYCL kernels. In the simplest case, you write
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * parallelDforWithSchedulerInstructions(volume, ...) {
 *   int firstIndex = volume(0);
 *   int secondIndex = volume(1);
 * } endParallelDfor
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * In this context, it is also interesting to study tarch::la::slice() which
 * allows you to extract whole subvectors from the index vector in one rush.
 *
 *
 * ## Generic realisation remarks
 *
 * - I convert the max argument always into counter##Max and then work against
 *   the vector. This way, I can support both vectors and scalars. In the
 *   former case, the copy constructor is used. In the latter case the
 *   constructor accepting a scalar.
 * - I embed the whole loop into another scope (brackets). Otherwise, I run
 *   into issues if users write things like
 *   ~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   parallelDfor(i,3) {
 *     [...]
 *   } endParallelDfor
 *   parallelDfor(i,3) {
 *     [...]
 *   } endParallelDfor
 *   ~~~~~~~~~~~~~~~~~~~~~~~~~~
 *   as the corresponding vector iMax then would be declared twice.
 *
 *
 * ## OpenMP
 *
 * OpenMP relies on pragmas around the nested for loops, which is tricky as
 * parallelDfor itself is a pragma. I hijack the construct
 *
 *              _Pragma( "my embedded pragma" )
 *
 * which is, to the best of my knowledge, a C99 instruction and not C++17. So I
 * simply assume and hope that this works. Note that the _Pragma statement
 * yields a #pragma, i.e. you are not allowed to write
 *
 *              _Pragma( "#pragma my embedded pragma" )
 *
 * as this would unfold into
 *
 *              #pragma #pragma my embedded pragma
 *
 * Further to that, variables of the pragma are not replaced in the unfolded
 * pragma. Therefore, I have to go this unorthodox way with the helper loop
 * guard.
 *
 * For the available OpenMP variants, it neither makes sense to have parallel
 * fors nor does it work properly:
 *
 * - If we run the kernel within a task (which we typically do), the task
 *   cannot spawn out onto multiple cores.
 * - I found no way to have multiple pragmas in a row where we toggle through
 *   the options via if.
 *
 * If we use a taskloop in OpenMP, the default visibility of all variables
 * will be taskfirst. Furthermore, the created copies will be subject to a
 * copy constructor and their destructor. This means that we alter the
 * visibility when we switch from parallel for or no parallelisation into a
 * taskloop. Therefore, I manually change the default visibility to shared.
 *
 *
 * ## TBB
 *
 * TBB is, in my opinion, the most straightforward implementation, as it
 * supports d-dimensional ranges: It has loop collapsing built-in. I use the
 * TBB ranges, embed a nested for loop into the range, and then puzzle the
 * counter of interest together from the ranges indices again.
 *
 *
 * ## SYCL
 *
 * Different to TBB, SYCL has this accelerator paradigm in mind, even though we
 * submit to the host queue. Therefore, we have to capture the variables via =,
 * we have to ensure all routines (such as dLinearised() are marked as
 * GPU offloadable), we do not have any dynamic scheduling, grain sizes, and so
 * forth. Most importantly, we cannot call virtual functions or functors from
 * SYCL kernels.
 *
 * For all of these reasons, we do not support parallel for for SYCL, and we do
 * not support SIMT parallelism either on the host. In theory, the latter should
 * work straightforwardly, but I got major issues with copy constructors et al
 * trying to go down this route, so I eventually dismissed it as a stupid idea.
 *
 * SIMT loops are always to be embedded into parallel loops. We do not embed
 * the other way round. Codes like the batched kernels will use SIMT loops
 * right from the start. So it would be important that the SIMT loops exploit
 * parallelism. If we make compromises, we have to make compromises on the
 * parallel loop level. Once SYCL abandons the need for call by value in the
 * capture clause of the functors, we should be able to use the SYCLised simt
 * macros for Peano's loops.
 */
#define parallelFor(counter, max) \
  parallelForWithSchedulerInstructions(counter, max, ::peano4::utils::LoopPlacement::SpreadOut)

#define simtFor(counter, max) \
  simtForWithSchedulerInstructions(counter, max, ::peano4::utils::LoopPlacement::SpreadOut)

#define parallelDfor(counter, max) \
  parallelDforWithSchedulerInstructions(counter, max, ::peano4::utils::LoopPlacement::SpreadOut)

#if Dimensions == 2
#define parallelDforWithSchedulerInstructions(counter, max, loopPlacement) \
  parallelDforWithSchedulerInstructions2d(counter, max, loopPlacement)

#define parallelDPlusOneForWithSchedulerInstructions(counter, max, loopPlacement) \
  parallelDforWithSchedulerInstructions3d(counter, max, loopPlacement)

#define parallelDPlusTwoForWithSchedulerInstructions(counter, max, loopPlacement) \
  parallelDforWithSchedulerInstructions4d(counter, max, loopPlacement)

#define simtDforWithSchedulerInstructions(counter, max, loopPlacement) \
  simtDforWithSchedulerInstructions2d(counter, max, loopPlacement)

#define simtDPlusOneForWithSchedulerInstructions(counter, max, loopPlacement) \
  simtDforWithSchedulerInstructions3d(counter, max, loopPlacement)

#define simtDPlusTwoForWithSchedulerInstructions(counter, max, loopPlacement) \
  simtDforWithSchedulerInstructions4d(counter, max, loopPlacement)
#elif Dimensions == 3
#define parallelDforWithSchedulerInstructions(counter, max, loopPlacement) \
  parallelDforWithSchedulerInstructions3d(counter, max, loopPlacement)

#define parallelDPlusOneForWithSchedulerInstructions(counter, max, loopPlacement) \
  parallelDforWithSchedulerInstructions4d(counter, max, loopPlacement)

#define parallelDPlusTwoForWithSchedulerInstructions(counter, max, loopPlacement) \
  parallelDforWithSchedulerInstructions5d(counter, max, loopPlacement)

#define simtDforWithSchedulerInstructions(counter, max, loopPlacement) \
  simtDforWithSchedulerInstructions3d(counter, max, loopPlacement)

#define simtDPlusOneForWithSchedulerInstructions(counter, max, loopPlacement) \
  simtDforWithSchedulerInstructions4d(counter, max, loopPlacement)

#define simtDPlusTwoForWithSchedulerInstructions(counter, max, loopPlacement) \
  simtDforWithSchedulerInstructions5d(counter, max, loopPlacement)
#endif


#if defined(SharedOMP) and defined(OpenMPTaskGroup)
#include "omp/Loop.h"
#elif defined(SharedOMP) and !defined(OpenMPTaskGroup)
#include "omp/LoopWithoutTaskLoop.h"
#elif defined(SharedTBB)
#include "tbb/Loop.h"
#elif defined(SharedCPP)
//and __cplusplus >= 202101L
#include "cpp/Loop.h"
#elif defined(SharedSYCL)
#include "sycl/Loop.h"
#else
#define parallelForWithSchedulerInstructions(counter, max, loopPlacement) \
  for (int counter = 0; counter < max; counter++) {

#define parallelDforWithSchedulerInstructions2d(counter, max, loopPlacement) \
  { \
    tarch::la::Vector<2, int> counter##Max(max); \
    for (int counter##0 = 0; counter##0 < counter##Max(0); counter##0 ++) \
      for (int counter##1 = 0; counter##1 < counter##Max(1); counter##1 ++) { \
        tarch::la::Vector<2, int> counter = {counter##0, counter##1};

#define parallelDforWithSchedulerInstructions3d(counter, max, loopPlacement) \
  { \
    tarch::la::Vector<3, int> counter##Max(max); \
    for (int counter##0 = 0; counter##0 < counter##Max(0); counter##0 ++) \
      for (int counter##1 = 0; counter##1 < counter##Max(1); counter##1 ++) \
        for (int counter##2 = 0; counter##2 < counter##Max(2); counter##2 ++) { \
          tarch::la::Vector<3, int> counter = {counter##0, counter##1, counter##2};

#define parallelDforWithSchedulerInstructions4d(counter, max, loopPlacement) \
  { \
    tarch::la::Vector<4, int> counter##Max(max); \
    for (int counter##0 = 0; counter##0 < counter##Max(0); counter##0 ++) \
      for (int counter##1 = 0; counter##1 < counter##Max(1); counter##1 ++) \
        for (int counter##2 = 0; counter##2 < counter##Max(2); counter##2 ++) \
          for (int counter##3 = 0; counter##3 < counter##Max(3); counter##3 ++) { \
            tarch::la::Vector<4, int> counter = {counter##0, counter##1, counter##2, counter##3};

#define parallelDforWithSchedulerInstructions5d(counter, max, loopPlacement) \
  { \
    tarch::la::Vector<5, int> counter##Max(max); \
    for (int counter##0 = 0; counter##0 < counter##Max(0); counter##0 ++) \
      for (int counter##1 = 0; counter##1 < counter##Max(1); counter##1 ++) \
        for (int counter##2 = 0; counter##2 < counter##Max(2); counter##2 ++) \
          for (int counter##3 = 0; counter##3 < counter##Max(3); counter##3 ++) \
            for (int counter##4 = 0; counter##4 < counter##Max(4); counter##4 ++) { \
              tarch::la::Vector<5, int> counter = {counter##0, counter##1, counter##2, counter##3, counter##4};

#define endParallelFor }
#define endParallelDfor \
  } \
  }


#define simtForWithSchedulerInstructions(counter, max, loopPlacement) \
  for (int counter = 0; counter < max; counter++) {

#define endSimtFor endParallelFor

#define simtDforWithSchedulerInstructions2d(counter, max, loopPlacement) \
  parallelDforWithSchedulerInstructions2d(counter, max, loopPlacement)

#define simtDforWithSchedulerInstructions3d(counter, max, loopPlacement) \
  parallelDforWithSchedulerInstructions3d(counter, max, loopPlacement)

#define simtDforWithSchedulerInstructions4d(counter, max, loopPlacement) \
  parallelDforWithSchedulerInstructions4d(counter, max, loopPlacement)

#define simtDforWithSchedulerInstructions5d(counter, max, loopPlacement) \
  parallelDforWithSchedulerInstructions5d(counter, max, loopPlacement)

#define endSimtDfor endParallelDfor

#endif


/**
 * Shortcut For dfor(counter,4)
 *
 * The usage of this optimised shortcut differs from dfor: You have to
 * replace both the dfor and the opening bracket by this macro, i.e.
 *
 * \code
 * dfor(counter,4) {
 * \endcode
 *
 * becomes
 *
 * \code
 * dfor4(counter)
 * \endcode
 *
 * You usually use this macro with
 * \code
 * #pragma unroll(FOUR_POWER_D)
 * \endcode
 * or
 * \code
 * #pragma omp parallel for schedule(static)
 * \endcode
 *
 * If you work with this specialised version of dfor on a variable k, two
 * counter variables are available within the loop's scope. The variable k
 * itself with type tarch::la::Vector<Dimensions,int>. Furthermore, there's always a variable
 * kScalar giving you k's value linearised.
 */
#define dfor4(counter) \
  for (int counter##Scalar = 0; counter##Scalar < FOUR_POWER_D; counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = Dimensions - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 4; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }


/**
 * Shortcut For dfor(counter,3)
 *
 * The usage of this optimised shortcut differs from dfor: You have to
 * replace both the dfor and the opening bracket by this macro, i.e.
 *
 * \code
 * dfor(counter,3) {
 * \endcode
 *
 * becomes
 *
 * \code
 * dfor3(counter)
 * \endcode
 *
 * You usually use this macro with
 * \code
 * #pragma unroll(ThreePowerD)
 * \endcode
 * or
 * \code
 * #pragma omp parallel for schedule(static)
 * \endcode
 *
 * If you work with this specialised version of dfor on a variable k, two
 * counter variables are available within the loop's scope. The variable k
 * itself with type tarch::la::Vector<Dimensions,int>. Furthermore, there's always a variable
 * kScalar giving you k's value linearised.
 */
#define dfor3(counter) \
  for (int counter##Scalar = 0; counter##Scalar < ThreePowerD; counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = Dimensions - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 3; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }


#define dfor5(counter) \
  for (int counter##Scalar = 0; counter##Scalar < FIVE_POWER_D; counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = Dimensions - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 5; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }

#define dfor7(counter) \
  for (int counter##Scalar = 0; counter##Scalar < SEVEN_POWER_D; counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = Dimensions - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 7; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }


#define dfor9(counter) \
  for (int counter##Scalar = 0; counter##Scalar < NINE_POWER_D; counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = Dimensions - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 9; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }

/**
 * If Dimensions is not set to two, we might nevertheless need
 * two-dimensional loops. So this is the corresponding macro. It is
 * way slower than dfor if you compile with -DDimensions=2.
 *
 * Please use this macro with an enddforx macro closing your scope rather than
 * brackets.
 *
 * Please note that counterScalar is already a linearised version of your counter.
 *
 * Please note that you need a specialised linearisation function (depending on d
 * explicitly) to work with 2d index vectors within such a loop. Do not just use
 * dLinearised, but use the d2Linearised or d3Linearised variant instead.
 */
#define d2for(counter, max) \
  for (int counter##Scalar = 0; counter##Scalar < tarch::la::aPowI(2, max); counter##Scalar++) { \
    tarch::la::Vector<2, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = 2 - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= max; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }

/**
 * If Dimensions is not set to two, we might nevertheless need
 * two-dimensional loops. So this is the corresponding macro.
 *
 * Please use enddforx to close a loop started with this macro.
 */
#define d2for2(counter) \
  for (int counter##Scalar = 0; counter##Scalar < tarch::la::aPowI(2, 2); counter##Scalar++) { \
    tarch::la::Vector<2, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = 2 - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 2; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }

/**
 * If Dimensions is not set to three, we might nevertheless need
 * three-dimensional loops. So this is the corresponding macro.
 *
 * Please use enddforx to close a loop started with this macro.
 */
#define d3for2(counter) \
  for (int counter##Scalar = 0; counter##Scalar < tarch::la::aPowI(3, 2); counter##Scalar++) { \
    tarch::la::Vector<3, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = 3 - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 2; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }


/**
 * If Dimensions is not set to three, we might nevertheless need
 * two-dimensional loops. So this is the corresponding macro. It is
 * way slower than dfor if you compile with -DDimensions=2.
 *
 * Please use this macro with an enddforx macro closing your scope rather than
 * brackets.
 *
 * Please note that counterScalar is already a linearised version of your counter.
 *
 * Please note that you need a specialised linearisation function (depending on d
 * explicitly) to work with 2d index vectors within such a loop. Do not just use
 * dLinearised, but use the d2Linearised or d3Linearised variant instead.
 */
#define d3for(counter, max) \
  for (int counter##Scalar = 0; counter##Scalar < tarch::la::aPowI(3, max); counter##Scalar++) { \
    tarch::la::Vector<3, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = 3 - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= max; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }


/**
 * If Dimensions is not set to two, we might nevertheless need
 * two-dimensional loops. So this is the corresponding macro.
 *
 * Please use enddforx to close a loop started with this macro.
 */
#define d2for3(counter) \
  for (int counter##Scalar = 0; counter##Scalar < tarch::la::aPowI(2, 3); counter##Scalar++) { \
    tarch::la::Vector<2, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = 2 - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 3; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }

/**
 * If Dimensions is not set to three, we might nevertheless need
 * three-dimensional loops. So this is the corresponding macro.
 *
 * Please use enddforx to close a loop started with this macro.
 */
#define d3for3(counter) \
  for (int counter##Scalar = 0; counter##Scalar < tarch::la::aPowI(3, 3); counter##Scalar++) { \
    tarch::la::Vector<3, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = 3 - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 3; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }


/**
 * Shortcut For dfor(counter,2)
 *
 * The usage of this optimised shortcut differs from dfor: You have to
 * replace both the dfor and the opening bracket by this macro, i.e.
 *
 * \code
 * dfor(counter,2) {
 * \endcode
 *
 * becomes
 *
 * \code
 * dfor2(counter)
 * \endcode
 *
 * You usually use this macro with
 * \code
 * #pragma unroll(TwoPowerD)
 * \endcode
 * or
 * \code
 * #pragma omp parallel for schedule(static)
 * \endcode
 *
 * If you work with this specialised version of dfor on a variable k, two
 * counter variables are available within the loop's scope. The variable k
 * itself with type tarch::la::Vector<Dimensions,int>. Furthermore, there's always a variable
 * kScalar giving you k's value linearised.
 */

/*
 * bit flipping used for Dimensions = 2, and Dimensions = 3
 * for more information about the idea principle used refer to https://opt-patterns.wiki.tum.de/dfor
 */
#if Dimensions == 2
#define dfor2(counter) \
  for (int counter##Scalar = 0, AA##counter = 0, BB##counter = 0; counter##Scalar < TwoPowerD; counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    counter(0)  = AA##counter; \
    counter(1)  = BB##counter; \
    AA##counter = !AA##counter; \
    BB##counter = !(AA##counter ^ BB##counter);

#elif Dimensions == 3
#define dfor2(counter) \
  for (int counter##Scalar = 0, AA##counter = 0, BB##counter = 0, CC##counter = 0; counter##Scalar < TwoPowerD; \
       counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    counter(0)  = AA##counter; \
    counter(1)  = BB##counter; \
    counter(2)  = CC##counter; \
    AA##counter = !AA##counter; \
    BB##counter = !(AA##counter ^ BB##counter); \
    CC##counter = CC##counter || (!AA##counter && !BB##counter && !CC##counter);

#else
#define dfor2(counter) \
  for (int counter##Scalar = 0; counter##Scalar < TwoPowerD; counter##Scalar++) { \
    tarch::la::Vector<Dimensions, int> counter; \
    { \
      int copy##counter##Scalar = counter##Scalar; \
      for (int counter##ddd = Dimensions - 1; counter##ddd >= 0; counter##ddd--) { \
        int counter##aPowI = 1; \
        for (int counter##jjj = 0; counter##jjj < counter##ddd; counter##jjj++) { \
          counter##aPowI *= 2; \
        } \
        counter(counter##ddd) = copy##counter##Scalar / counter##aPowI; \
        copy##counter##Scalar -= counter(counter##ddd) * counter##aPowI; \
      } \
    }

#endif


/**
 * I prefer to use this macro for dforx instead of a closing bracket as many
 * syntax parser fail otherwise.
 */
#define enddforx }


/**
 * This is an exclusive d-dimensional for loop. Exclusive means, there is one
 * dimension that is not manipulated during the for loop. This dimension
 * (entry of the counter) is specified by dim and has the value value
 * throughout the for-loop.
 */
#define dfore(counter, max, dim, value) \
  for (tarch::la::Vector<Dimensions, int> counter = peano4::utils::dStartVector(dim, value); \
       peano4::utils::dCmp(counter, max); \
       peano4::utils::dInc(counter, max, dim))


/**
 * This is a d-dimensional z-loop. A z-loop is a d-dimensional loop the
 * counter direction changes every time an inner loop direction has changed.
 * So this is the loop corresponding to a Peano curve. The for loop is passed
 * a counter name, the number of steps to perform in each direction and a
 * direction flag that identifies the initial direction. Note that this
 * argument has to be a real variable, it might not be a constant. The
 * direction flag array identifies for each direction, whether the initial
 * loop goes along the axis or not. The type of direction is LoopDirection.
 *
 * Here are some examples for two dimensions:
 * \code
 *   LoopDirection d(3);  // equals {true,true} and identifies the standard
 *                        // Peano Leitmotiv
 *   zfor( a, 3, d ) {
 *     std::cout << a;
 *   }
 * \endcode
 * yields in [0,0],[1,0],[2,0],[2,1],[1,1],[0,1],[0,2],[1,2],[2,2].
 *
 * \code
 *   LoopDirection d(1);  // equals {true, false} and specifies a Peano curve
 *                        // from the left top to right bottom
 *   zfor( a, 3, d ) {
 *     std::cout << a;
 *   }
 * \endcode
 * yields in [0,2],[1,2],[2,2],[2,1],[1,1],[0,1],[0,0],[1,0],[2,0].
 */


#define zfor(counter, max, direction) \
  { \
    for (tarch::la::Vector<Dimensions, int> counter = peano4::utils::dStartVector(max, direction); \
         peano4::utils::dCmp(counter, max); \
         peano4::utils::dInc(counter, max, direction)) {


/*
 * zfor3 is an optimized version of zfor for max = 3
 * A lookup table is used for dim=2 and dim=3, for higher dimensions
 * the standard zfor is used instead
 */
#if Dimensions == 2
// first entry is direction, second entry is cell, third entry is tuples
static const int lookupzfor3[4][9][2] = {
  {{2, 2}, {1, 2}, {0, 2}, {0, 1}, {1, 1}, {2, 1}, {2, 0}, {1, 0}, {0, 0}},
  {{0, 2}, {1, 2}, {2, 2}, {2, 1}, {1, 1}, {0, 1}, {0, 0}, {1, 0}, {2, 0}},
  {{2, 0}, {1, 0}, {0, 0}, {0, 1}, {1, 1}, {2, 1}, {2, 2}, {1, 2}, {0, 2}},
  {{0, 0}, {1, 0}, {2, 0}, {2, 1}, {1, 1}, {0, 1}, {0, 2}, {1, 2}, {2, 2}}};

#define zfor3(counter, direction) \
  { \
    tarch::la::Vector<Dimensions, int> counter; \
    int                                counter##initDir = static_cast<int>(direction.to_ulong()); \
    for (int counter##i = 0; counter##i < 9; ++counter##i) { \
      counter(0) = lookupzfor3[counter##initDir][counter##i][0]; \
      counter(1) = lookupzfor3[counter##initDir][counter##i][1];

#elif Dimensions == 3
static const int lookupzfor3[8][27][3] = {
  {{2, 2, 2}, {1, 2, 2}, {0, 2, 2}, {0, 1, 2}, {1, 1, 2}, {2, 1, 2}, {2, 0, 2}, {1, 0, 2}, {0, 0, 2},
   {0, 0, 1}, {1, 0, 1}, {2, 0, 1}, {2, 1, 1}, {1, 1, 1}, {0, 1, 1}, {0, 2, 1}, {1, 2, 1}, {2, 2, 1},
   {2, 2, 0}, {1, 2, 0}, {0, 2, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {2, 0, 0}, {1, 0, 0}, {0, 0, 0}},
  {{0, 2, 2}, {1, 2, 2}, {2, 2, 2}, {2, 1, 2}, {1, 1, 2}, {0, 1, 2}, {0, 0, 2}, {1, 0, 2}, {2, 0, 2},
   {2, 0, 1}, {1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {2, 1, 1}, {2, 2, 1}, {1, 2, 1}, {0, 2, 1},
   {0, 2, 0}, {1, 2, 0}, {2, 2, 0}, {2, 1, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}, {1, 0, 0}, {2, 0, 0}},
  {{2, 0, 2}, {1, 0, 2}, {0, 0, 2}, {0, 1, 2}, {1, 1, 2}, {2, 1, 2}, {2, 2, 2}, {1, 2, 2}, {0, 2, 2},
   {0, 2, 1}, {1, 2, 1}, {2, 2, 1}, {2, 1, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {2, 0, 1},
   {2, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {2, 2, 0}, {1, 2, 0}, {0, 2, 0}},
  {{0, 0, 2}, {1, 0, 2}, {2, 0, 2}, {2, 1, 2}, {1, 1, 2}, {0, 1, 2}, {0, 2, 2}, {1, 2, 2}, {2, 2, 2},
   {2, 2, 1}, {1, 2, 1}, {0, 2, 1}, {0, 1, 1}, {1, 1, 1}, {2, 1, 1}, {2, 0, 1}, {1, 0, 1}, {0, 0, 1},
   {0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {2, 1, 0}, {1, 1, 0}, {0, 1, 0}, {0, 2, 0}, {1, 2, 0}, {2, 2, 0}},
  {{2, 2, 0}, {1, 2, 0}, {0, 2, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {2, 0, 0}, {1, 0, 0}, {0, 0, 0},
   {0, 0, 1}, {1, 0, 1}, {2, 0, 1}, {2, 1, 1}, {1, 1, 1}, {0, 1, 1}, {0, 2, 1}, {1, 2, 1}, {2, 2, 1},
   {2, 2, 2}, {1, 2, 2}, {0, 2, 2}, {0, 1, 2}, {1, 1, 2}, {2, 1, 2}, {2, 0, 2}, {1, 0, 2}, {0, 0, 2}},
  {{0, 2, 0}, {1, 2, 0}, {2, 2, 0}, {2, 1, 0}, {1, 1, 0}, {0, 1, 0}, {0, 0, 0}, {1, 0, 0}, {2, 0, 0},
   {2, 0, 1}, {1, 0, 1}, {0, 0, 1}, {0, 1, 1}, {1, 1, 1}, {2, 1, 1}, {2, 2, 1}, {1, 2, 1}, {0, 2, 1},
   {0, 2, 2}, {1, 2, 2}, {2, 2, 2}, {2, 1, 2}, {1, 1, 2}, {0, 1, 2}, {0, 0, 2}, {1, 0, 2}, {2, 0, 2}},
  {{2, 0, 0}, {1, 0, 0}, {0, 0, 0}, {0, 1, 0}, {1, 1, 0}, {2, 1, 0}, {2, 2, 0}, {1, 2, 0}, {0, 2, 0},
   {0, 2, 1}, {1, 2, 1}, {2, 2, 1}, {2, 1, 1}, {1, 1, 1}, {0, 1, 1}, {0, 0, 1}, {1, 0, 1}, {2, 0, 1},
   {2, 0, 2}, {1, 0, 2}, {0, 0, 2}, {0, 1, 2}, {1, 1, 2}, {2, 1, 2}, {2, 2, 2}, {1, 2, 2}, {0, 2, 2}},
  {{0, 0, 0}, {1, 0, 0}, {2, 0, 0}, {2, 1, 0}, {1, 1, 0}, {0, 1, 0}, {0, 2, 0}, {1, 2, 0}, {2, 2, 0},
   {2, 2, 1}, {1, 2, 1}, {0, 2, 1}, {0, 1, 1}, {1, 1, 1}, {2, 1, 1}, {2, 0, 1}, {1, 0, 1}, {0, 0, 1},
   {0, 0, 2}, {1, 0, 2}, {2, 0, 2}, {2, 1, 2}, {1, 1, 2}, {0, 1, 2}, {0, 2, 2}, {1, 2, 2}, {2, 2, 2}}};

#define zfor3(counter, direction) \
  { \
    tarch::la::Vector<Dimensions, int> counter; \
    int                                counter##initDir = static_cast<int>(direction.to_ulong()); \
    for (int counter##i = 0; counter##i < 27; ++counter##i) { \
      counter(0) = lookupzfor3[counter##initDir][counter##i][0]; \
      counter(1) = lookupzfor3[counter##initDir][counter##i][1]; \
      counter(2) = lookupzfor3[counter##initDir][counter##i][2];

#else
#define zfor3(counter, direction) zfor(counter, 3, direction)

#endif


#define endzfor \
  } \
  }
