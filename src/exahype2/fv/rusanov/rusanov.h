// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/CellData.h"
#include "exahype2/VolumeIndex.h"
#include "Functors.h"
#include "LoopBodies.h"
#include "peano4/utils/Loop.h"
#include "tarch/timing/Measurement.h"
#include "tarch/timing/Watch.h"

/**
 * @page exahype2_fv_rusanov ExaHyPE's finite volumes Rusanov kernel implementation
 *
 * ExaHyPE's Finite Volume compute kernels rely on a block-structured mesh.
 * Each cell of the spacetree carries a patch of @f$ p \times p \times p @f$
 * Finite Volumes. This is basically a simple double array.
 * We can also read the patch as a regular Cartesian mesh. It is
 * supplemented by a halo layer of volumes of size k. Halo layers are also
 * called ghost cells here. ExaHyPE currently does not support transversal
 * waves (diagonals). So while we effectively have data of a
 * @f$ (p+2k) \times (p+2k) \times (p+2k) @f$ available, the diagonal corners
 * or blocks do not hold valid data. Parallelisation, adaptivity and the
 * proper synchronisation of the ghost cells with neighbouring cells is none
 * of the kernel's business.
 *
 *
 * ## Function signature
 *
 * A compute kernel accepts a @f$ (p+2k) \times (p+2k) \times (p+2k) @f$ patch
 * of the solution, where each volume carries N+M unknowns. N is the number of
 * PDE unknowns, and M is the number of auxiliary (material) parameters. The
 * latter are not evolved by the PDE. M can be zero, N has to be positive. The
 * input is organised as Array of Structs (AoS) and the enumeration is lexicographic
 * along the coordinate axes.
 *
 * In 2d, the left bottom finite volume (an uninitialised ghost volume) is the
 * first voxel.
 * It determines the first N+M double entries in the input.
 * From here, we first walk along the x-axis, then along the y-axis and finally
 * along the z-axis. Per volume, we always read the N+M entries from the input
 * before we proceed with the next volume.
 *
 * The output (image) of a kernel is a @f$ p \times p \times p @f$ patch of
 * N+M values. It is stored in the same AoS ordering. The output data represent
 * the next time step. We do not advance the ghost volumes in time.
 *
 * @image html Rusanov00.png
 *
 * In the pictures above, we map the left regular patch to the right one (with
 * the darker grey). The bright grey data are the core input data supplemented
 * with a ghost layer of one from the left, upper, right and bottom neighbour.
 * The empty volumes are in the data structure to make things easier, i.e., to
 * avoid weird index re-calculations. But they do not hold valid data. We work
 * with a halo layer of one in this illustration.
 *
 * All in all, we get @f$ (p+2k)^d \cdot (N+M) @f$ entries as input and return
 * @f$ p^d \cdot (N+M) @f$ entries for the next time step. While we typically
 * advance the N unknowns, the M usually are plain copies from the previous
 * time step.
 *
 *
 * ## Mathematics
 *
 * For the Rusanov solver, we rely on a simple weak formulation of the
 * underlying first-order hyperbolic PDE:
 *
 * @f$ \partial _t Q + div F_i(Q) + \sum _i B_i \nabla_i Q = S(Q) @f$
 *
 * We assume this ordering in the compute kernels, i.e., all terms but the
 * source term S are found on the left-hand side in the PDE formulation.
 * Integration over space and time and the assumption that each Finite
 * Volume carries, per PDE unknown, a constant function both in space (over
 * this volume) and time yields
 *
 * @f$ \int _\Omega Q(t+\Delta t) - Q(t) dx + \Delta t \cdot \int _\Omega div F_i(Q) dx + \Delta t \cdot \sum _i \int _\Omega B_i \nabla_i Q dx = \Delta t \cdot \int _\Omega S dx
@f$
 *
 * where we have already evaluated the time integral. Next, we read this as
 * a weak formulation with a test function, where the characteristic function
 * over the individual Finite Volumes serves as test function.
 *
 * \f{eqnarray*}{
 \int _\Omega (Q(t+\Delta t) - Q(t))\chi _v dx
  & = & - \Delta t \cdot \int _\Omega div \ F_i(Q) \chi _v dx
        - \Delta t \cdot \sum _i \int _\Omega B_i \nabla_i Q \chi _v dx
        + \Delta t \cdot \int _\Omega S(Q) \chi _v dx
        \\
  \forall v: \qquad
  h^d \cdot (Q(t+\Delta t)-Q(t))
  & = & -\Delta t \cdot \oint _{\partial v} (F_i(Q),n) dS(x) \\
    && - \Delta t \cdot \int _v div \left( B_i \ Q \right) dx
     + \Delta t \cdot \int _v Q div B_i  dx
    \\
    && + \Delta t \cdot h^d S(Q) \\
  Q(t+\Delta t)
  & = & Q(t)-\frac{\Delta t}{h^d} \cdot \oint _{\partial v} ([[F_i(Q)+B_i Q]],n) dS(x)
    + \frac{\Delta t}{h^d} \int _{v} Q div B_i dx
    + \Delta t S(Q)
   \f}
 *
 * which exploits the chain rule to separate the non-conservative term into a
 * conservative part and the remainder: @f$ div \left( B_i\ Q \right) = Q div B_i + \sum_i B_i \nabla_i Q @f$
 * i.e., @f$ \sum_i B_i \nabla_i Q = div \left( B_i\ Q \right) - Q div B_i @f$. double brackets @f$ [[]] @f$ means
 * the quantities needs to be evaluated on the face position.
 *
 *
 * As we cannot provide expressions for the terms on the faces, we provide
 * numerical approximations
 *
 * \f{eqnarray*}{
 \oint _{\partial v} ([[F_i(Q)+B_i Q]],n) dS(x)
 & = &\sum_{\partial v} h^{d-1} (n, \Big[ \frac{1}{2} \left( F_i(Q^+) + F_i(Q^-) \right)  \\
  && + \frac{1}{2}(Q^+ + Q^-) B_i( \frac{1}{2}(Q^+ + Q^-) )    \\
  &&  - \frac{1}{2} max(\lambda _{max}(Q^-),\lambda _{max}(Q^+)) (Q^+ - Q^-) \Big])
   \f}
 *
 * where we average @f$ [[Q]] \approx \frac{1}{2}(Q^+ + Q^-) @f$ and assume a linear F. the @f$ Q^+ @f$
 * and @f$ Q^- @f$ represent the unknowns values on the right and on the left of the considered face respectively. Notice the inner
 * product with the face normal vector provides a minus sign for the left faces in the following calculation.
 *
 * The update of a volume hence depends on the source term contribution within the volume,
 * an averaged conservative flux over the faces (which now is the sum of @f$ 2d @f$ flat faces),
 * a damping term using the maximum of the eigenvalues
 * in the left and right volume (see, local Lax-Friedrich scheme), a non-conservative flux over the average on the faces and
 * a non-conservative flux within the volume.
 *
 * The last missing piece of the puzzle is the treatment of the remaining volumetric
 * contribution over the non-conservative term.
 * We notice that we can move the unknown @f$ Q @f$ out of the integral, as it is constant.
 * The remaining volume integral over @f$ div B_i @f$ gives us again a surface contribution of @f$ B_i @f$.
 * multiplied with the normal. These
 * contributions depend on the relative position of the face and volume:
 * \f{eqnarray*}{
\int _{v} Q div B_i dx =Q \int_v div B_i dx =Q \oint_{\partial v} ([[B_i]],n) dS(x)=
Q\sum_{\partial v}h^{d-1}(B_i(\frac{1}{2}(Q^+ + Q^-)),n)
   \f}
 * Now we can combine these contributions with other flux terms, but we need to be careful:
 * if the face is on the right of the volume, we should use @f$ Q^- @f$ while for left face we use
 * @f$ Q^+ @f$. So to be clear we write the total flux contribution separately for left and right face:
 * \f{eqnarray*}{
{\rm flux}^d_{\rm left} &=& \left[-\frac{\Delta t}{h^d} \cdot \oint _{\partial v} ([[F_i(Q)+B_i Q]],n) dS(x)
    + \frac{\Delta t}{h^d} \int _{v} Q div B_i dx \right]_{\rm for \ a \ single \ left \ face \ in \ d \ direction}\\
&=& \frac{\Delta t}{2h}\left[ \left( F_d(Q^+) + F_d(Q^-) \right)
  -(Q^+ - Q^-) B_d( \frac{1}{2}(Q^+ + Q^-) )
  - max(\lambda _{max}(Q^-),\lambda _{max}(Q^+)) (Q^+ - Q^-) \right]
   \f}
 * \f{eqnarray*}{
{\rm flux}^d_{\rm right} &=& \left[-\frac{\Delta t}{h^d} \cdot \oint _{\partial v} ([[F_i(Q)+B_i Q]],n) dS(x)
    + \frac{\Delta t}{h^d} \int _{v} Q div B_i dx \right]_{\rm for \ a \ single \ right \ face \ in \ d \ direction}\\
&=& \frac{\Delta t}{2h}\left[- \left( F_d(Q^+) + F_d(Q^-) \right)
  -(Q^+ - Q^-) B_d( \frac{1}{2}(Q^+ + Q^-) )
  + max(\lambda _{max}(Q^-),\lambda _{max}(Q^+)) (Q^+ - Q^-) \right]
   \f}
 * Here @f$ F_d, \ B_d @f$ are the component of @f$ F_i, \ B_i @f$ in d direction. And @f$ Q^+ @f$ and @f$ Q^- @f$
 * are referring to the value on the left and the right of the considered face.
 * we now can write down the final volume updating scheme in our FV solver
 * \f{eqnarray*}{
  \forall v: \qquad Q(t+\Delta t)
   =  Q(t)+\sum_d ({\rm flux}^d_{\rm left}+{\rm flux}^d_{\rm right}) + \Delta t S(Q)
   \f}
 *
 *
 * ## The linearity assumption in the F term
 *
 * ## Compute steps
 *
 * The derivation of the Finite Volume update scheme identifies a couple
 * of compute steps.
 *
 * - (1) Per volume: Copy @f$ Q(t) @f$ into @f$ Q(t+\Delta t) @f$. This is the
 *   initialisation of @f$ Q(t+\Delta t)@f$.
 * - (2.1) Per volume: Compute @f$ \lambda _{max} Q(t) @f$.
 * - (2.2) Per face: Take the maximum
 *   @f$ \lambda _{max} Q(t) @f$ of the two adjacent volumes and multiply it
 *   with the delta @f$ Q^+(t)-Q^-(t) @f$ between the adjacent volumes' values.
 * - (2.3) Per volume: Take the outcome of (2.2) and update the volume. For this,
 *   we have to multiple this term with @f$ \Delta t / h @f$. The C is a problem-specific
 *   constant and the h is the mesh size. It stems from the fact that the original
 *   weak (integral) formulation of the Finite Volume scheme has a volume integral
 *   over time derivative and a surface integral over the @f$ F @f$ terms to which
 *   the flux serves as damping. So we have an integral over the submanifold with
 *   a scaling of @f$ h^{d-1} @f$ on the update side and divide by @f$ h^d @f$ which
 *   yields a factor @f$ h^{-1} @f$.
 * - (3.1) Per volume: Compute @f$ F(Q) @f$ (the flux)
 * - (3.2) Per face: Compute the average of the fluxes of two adjacent volumes' fluxes
 * - (3.3) Per volume: Update the volume with the fluxes from their @f$ 2d @f$
 *   adjacent faces subject to a scaling with @f$ \Delta t/h @f$.
 * - (4.1) Per face: Compute @f$ \frac{1}{2}\ B_d \cdot \Delta Q = \frac{1}{2}\ B_d \cdot (Q^+ - Q^-)@f$.
 *   This is the Finite Volume's non-conservative product.
 * - (4.2) Per volume: Add the ncp face contributions subject ot a scaling of
 *   @f$ \Delta t/h @f$.
 * - (5) Per volume: Add @f$ \Delta t \cdot S(Q(t)) @f$ to @f$ Q(t+\Delta t) @f$.
 * - (6.1) Per volume: Compute @f$ \lambda _{max} Q(t+\Delta a) @f$.
 * - (6.2) Reduce @f$ \lambda _{max} Q(t+\Delta a) @f$ over all the Finite
 *   Volumes, i.e., compute the patch-global maximum eigenvalue.
 *
 *
 * ## High level design of kernels
 *
 * We start from a few simple observations:
 *
 * - As long as we copy @f$ Q(t) @f$ over into @f$ Q(t+\Delta t) @f$ first, all the
 *   other steps can be permuted, as they solely add values to the outcome.
 * - We can combine different steps such as (2.2) and (2.3) into one step.
 * - We can arbitrary permute the order in which we run over the faces or volumes
 *   per step, and we can even do these calculations concurrently.
 * - For steps (2), (3) and (4), we can either compute the PDE terms (eigenvalue,
 *   flux, ncp) face-wisely, store these outcomes somewhere, and then run over the
 *   volumes and collect the @f$ 2d @f$ face contributions into an update, or we can
 *   compute the face contribution to the two neighbouring volumes and immediately
 *   (in situ) update their values.
 *
 * For our implementations we make these observations guide our design:
 *
 * - The Rusanov solver is first of all a collection of compute steps which realise
 *   (2.1)+(2.2), (3.1)+(3.2) or (4.1). These computeXXX routines can all be found
 *   in LoopBodies.h. Each one accepts an integer vector which clarifies on which
 *   voxel or face it operates and a pointer to the input and output.
 * - Actually, we work with a scratchpad design, i.e., each compute routine gets the
 *   pointer to some large array and then an enumerator which allows the routine to
 *   map a logical index (volume position plus unknown index) onto a memory location.
 *   This is a strict separation of concern: If we change the enumeration of any
 *   data structure under the hood, we don't have to rewrite the actual computations.
 *   We simply feed in another enumerator which takes care what memory location an
 *   actual piece of data is. The key idea is: the compute kernels write down what
 *   is computed, but they don't care in which order things are computed or in
 *   which order input and output are stored.
 * - Each compute routine is accompanied with a corresponding updateXX routine which
 *   takes the outcome(s) and adds it to the volume's @f$ Q(t+\Delta t) @f$.
 * - If a term does not exist in the PDE, we simply ignore the corresponding for loops.
 * - Once the core compute steps are realised within LoopBodies.h, we can write the
 *   loops around these steps.
 * - If we have a set of P patches each $f@ p \times p \times p $f@, we end up with
 *   three to five nested loops how to update or write into the image array. These
 *   loops can be permuted or even run in parallel.
 *
 * With the above software design, we can explore a couple of different options:
 *
 * - We can either run over the faces and immediately (in-situ) update the two
 *   adjacent volumes. For this, we invoke computeX, pipe the outcome into some
 *   temporary vector
 * - If we decide to first compute some terms and then to update in one rush, we
 *   can play around with the order in which we store the intermediate results.
 * - We have different realisation of these outer loops: We obviously can permute
 *   their order, but we can also realise them through plain C loops or SYCL
 *   range iterators, or loops annotated with OpenMP.
 *
 * The last observation implies that we can port codes to GPUs. In this context, a
 * GPU developer only focuses on the for loops around the compute steps, while the
 * same compute steps are used both on the device and the host. This idea has been
 * proposed by Dominic E. Charrier within ExaHyPE 2, while the ExaHyPE (1) compute
 * kernels did resemble the in-situ code variant.
 *
 *
 * ## In-situ updates
 *
 * The in-situ update requires us to interleave the column/row updates if we work
 * in parallel: We first do all the odd faces with a normal along the x-axis, then
 * all the even ones, then all the odd ones with normals along y, and so forth.
 * This way, we avoid any race condition as we write to the output.
 *
 * @image html ApplyRiemannToPatchInsitu.png
 *
 *
 * ## Patch updates with temporary scratchpads
 *
 * If we update the patches in big steps where each step basically evaluates only
 * one type of basic step over all volumes, we end up with large for loops which
 * we can vectorise or parallelise straightforwardly.
 *
 * I offer different versions of the scratchpad: They differ in their internal
 * ordering (SoA or AoS or AoSoA), and we can hold them either on the call stack
 * or on the heap. The call stack variant fails for larger patches.
 *
 * The choice of the ordering within the scratchpad means that we kind of permute
 * the unknown ordering on-the-fly: input and output data always are AoS, as this
 * is the format ExaHyPE 2 uses internally. When we compute the fluxes for example
 * and pipe the outcome as SoA, we can read this as the fusion of a permutation
 * and the actual computation. Actually, nothing stops you from doing the reordering
 * manually as post- or pre-processing step. You also don't have to change anything
 * within the core compute routines. You simply have to ensure that you use the
 * right enumerators when you invoke the compute bodies.
 *
 * @image html Rusanov01.png
 *
 * The example above tries to illustrate this for a PDE over a three unknowns
 * (blue, green, red) with a patch size of view and a halo of one. In the input,
 * these data are stored as blue, green, red, blue, green, red, blue, ...
 * We get three fluxes per face as outcome. Again, we can store them alternating
 * (AoS) or we can store all the blue data first, then all the green, then all
 * the red. This ordering is encoded within the enumerator over the double array.
 * The actual double array (scratchpad) has always the same size.
 *
 * @author Baojiu Li
 * @author Han Zhang
 * @author Tobias Weinzierl
 */

namespace exahype2::fv::rusanov {
  /**
   * Apply the Rusanov Riemann solver over a set of patches.
   *
   * Here's the explanation of the implementation details marked via _subscript:
   *
   * _patchwise: This solver runs through the individual patches one by one.
   *   All the operations on patch 0 are completed, then we trigger all steps on
   *   patch 1, and so forth.
   *
   * _batched: We run through the individual steps of the finite volume update,
   *   and in each step we complete the work on all the patches handed in.
   *
   * There are two variants of the kernels: One that accepts a functor, and one
   * that works with the solver object and involves the static (stateless) PDE term variants.
   * Both are annotated with Otter pragmas, so you can find out how much parallelism
   * there is.
   *
   *
   * ## Implementation details (signatures)
   *
   * The function is marked as stateless and put into a header file. I still want to
   * separate the declaration from the definition, so the actual implementation is
   * in a file with the extension cpph. This indicates that it should (logically)
   * be in a cpp file, but to allow the compiler to inline aggressively, it has to
   * be in the header.
   *
   * Please study the comments on the namespace loopbodies (hosted in LoopBodies.h) for
   * further details why certain code parts are written the way they are written.
   * The punchline is that the routines here host the loops, but the actual loop
   * bodies then are separated into routines of their own. We inline them aggressively,
   * so that doesn't come at additional cost. This way, we can re-use the same loop
   * body within different loop implementations, orderings, etc.
   *
   *
   * ## Vectorisation
   *
   * Most of the time, I discuss within the individual loop bodies how to vectorise
   * and what things have to be taken into account. That is, the knowledge around
   * optimisation is documented with the corresponding loop bodies. There is one
   * exception:
   *
   * If I evaluate the fluxes, I have to take into account that diagonal volumes
   * don't hold valid data. After all, we only use split or face-normal fluxes, but
   * do not support transversal waves. So we should mask out those diagonal volumes
   * and should not compute anything on them. Indeed, some applications use
   * assertions to ensure that their user functions are not invoked on uninitialised
   * data and then fail. Unfortunately, such a mask stops the vectoriser from doing
   * its job, as the control flow simply becomes too complicated. So I enable the
   * masking if and only if I am not in release mode.
   *
   *
   * ## Memory management
   *
   * See ::exahype2::CellData() for details
   *
   *
   * ## Headers vs. implementation files
   *
   * This should be a proper routine (and thus not static). But I decided
   * to make it consistent with the templates for the static evaluation and
   * to move the routine into the header, too. The advantage is that the
   * compiler now cans inline routines straightforwardly without any ipo.
   *
   * Next, all the integers and booleans are known. So, once inlined, the
   * compiler can trivialise the compute kernel and perform vectorisation et
   * al even though we have functors which can not trivially be inlined and
   * optimised.
   */
  template <
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseHeapFunctors(
    CellData&                            patchData,
    const FluxFunctor&                   fluxFunctor,
    const NonconservativeProductFunctor& nonconservativeProductFunctor,
    const SourceFunctor&                 sourceFunctor,
    const MaxEigenvalueFunctor&          maxEigenvalueFunctor,
    tarch::timing::Measurement&          measurement,
    peano4::utils::LoopPlacement       loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseHeapFunctors(
    CellData&                            patchData,
    const FluxFunctor&                   fluxFunctor,
    const NonconservativeProductFunctor& nonconservativeProductFunctor,
    const SourceFunctor&                 sourceFunctor,
    const MaxEigenvalueFunctor&          maxEigenvalueFunctor,
    peano4::utils::LoopPlacement       loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapFunctors()
   */
  template <
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedHeapFunctors(
    CellData&                            patchData,
    const FluxFunctor&                   fluxFunctor,
    const NonconservativeProductFunctor& nonconservativeProductFunctor,
    const SourceFunctor&                 sourceFunctor,
    const MaxEigenvalueFunctor&          maxEigenvalueFunctor,
    tarch::timing::Measurement&          measurement,
    peano4::utils::LoopPlacement       loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedHeapFunctors(
    CellData&                            patchData,
    const FluxFunctor&                   fluxFunctor,
    const NonconservativeProductFunctor& nonconservativeProductFunctor,
    const SourceFunctor&                 sourceFunctor,
    const MaxEigenvalueFunctor&          maxEigenvalueFunctor,
    peano4::utils::LoopPlacement       loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  /**
   * @see timeStepWithRusanovPatchwiseHeapFunctors()
   */
  template <
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovVolumewiseFunctors(
    CellData&                            patchData,
    const FluxFunctor&                   fluxFunctor,
    const NonconservativeProductFunctor& nonconservativeProductFunctor,
    const SourceFunctor&                 sourceFunctor,
    const MaxEigenvalueFunctor&          maxEigenvalueFunctor,
    tarch::timing::Measurement&          measurement,
    peano4::utils::LoopPlacement         loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovVolumewiseFunctors(
    CellData&                            patchData,
    const FluxFunctor&                   fluxFunctor,
    const NonconservativeProductFunctor& nonconservativeProductFunctor,
    const SourceFunctor&                 sourceFunctor,
    const MaxEigenvalueFunctor&          maxEigenvalueFunctor,
    peano4::utils::LoopPlacement         loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseCallStackStateless(
    CellData& patchData, tarch::timing::Measurement& measurement, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseCallStackStateless(
    CellData& patchData, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseHeapStateless(
    CellData& patchData, tarch::timing::Measurement& measurement, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseHeapStateless(
    CellData& patchData, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseInsituStateless(
    CellData& patchData, tarch::timing::Measurement& measurement, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovPatchwiseInsituStateless(
    CellData& patchData, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedInsituStateless(
    CellData& patchData, tarch::timing::Measurement& measurement, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedInsituStateless(
    CellData& patchData, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedCallStackStateless(
    CellData& patchData, tarch::timing::Measurement& measurement, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedCallStackStateless(
    CellData& patchData, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedHeapStateless(
    ::exahype2::CellData& patchData, tarch::timing::Measurement& measurement, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovBatchedHeapStateless(
    ::exahype2::CellData& patchData, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovVolumewiseStateless(
    ::exahype2::CellData& patchData, tarch::timing::Measurement& measurement, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  template <
    class SolverType,
    int  NumberOfVolumesPerAxisInPatch,
    int  HaloSize,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class TempDataEnumeratorType>
  KeywordToAvoidDuplicateSymbolsForInlinedFunctions void timeStepWithRusanovVolumewiseStateless(
    ::exahype2::CellData& patchData, peano4::utils::LoopPlacement loopParallelism = peano4::utils::LoopPlacement::Serial
  ) InlineMethod;


  namespace internal {
    tarch::la::Vector<Dimensions + 1, int> rangeOverVolumesTimesUnknowns(int numberOfVolumesPerAxisInPatch, int unknowns);
    tarch::la::Vector<Dimensions + 1, int> rangeOverVolumesTimesUnknownsPlusAuxiliaryVariables(int numberOfVolumesPerAxisInPatch, int unknowns, int auxiliaryVariables);

    tarch::la::Vector<Dimensions + 1, int> rangeOverVolumesTimesPatches(int numberOfVolumesPerAxisInPatch, int patches);
    tarch::la::Vector<Dimensions + 2, int> rangeOverVolumesTimesUnknownsTimesPatches(int numberOfVolumesPerAxisInPatch, int unknowns, int patches);
    tarch::la::Vector<Dimensions + 2, int> rangeOverVolumesTimesUnknownsPlusAuxiliaryVariablesTimesPatches(
      int numberOfVolumesPerAxisInPatch, int unknowns, int auxiliaryVariables, int patches
    );

    /**
     * Construct iteration range
     *
     * If you have a 6x6x6 range and a halo of 3, then you get
     *
     * - (6+2*3)x6x6 if extendInBothDirections is true;
     * - (6+3)x6x6 if extendInBothDirections is false.
     */
    tarch::la::Vector<Dimensions, int> rangeOverVolumesPlusHaloInXDirection(int numberOfVolumesPerAxisInPatch, int haloSize, bool extendInBothDirections);
    tarch::la::Vector<Dimensions, int> rangeOverVolumesPlusHaloInYDirection(int numberOfVolumesPerAxisInPatch, int haloSize, bool extendInBothDirections);
    tarch::la::Vector<3, int>          rangeOverVolumesPlusHaloInZDirection(int numberOfVolumesPerAxisInPatch, int haloSize, bool extendInBothDirections);

    tarch::la::Vector<Dimensions + 1, int> rangeOverVolumesTimesPatchesPlusHaloInXDirection(int numberOfVolumesPerAxisInPatch, int haloSize, int patches);
    tarch::la::Vector<Dimensions + 1, int> rangeOverVolumesTimesPatchesPlusHaloInYDirection(int numberOfVolumesPerAxisInPatch, int haloSize, int patches);
    tarch::la::Vector<3 + 1, int>          rangeOverVolumesTimesPatchesPlusHaloInZDirection(int numberOfVolumesPerAxisInPatch, int haloSize, int patches);
  } // namespace internal
} // namespace exahype2::fv::rusanov

#include "BatchedFunctors.cpph"
#include "BatchedInsitu.cpph"
#include "BatchedStateless.cpph"
#include "PatchwiseFunctors.cpph"
#include "PatchwiseInsitu.cpph"
#include "PatchwiseStateless.cpph"
#include "VolumewiseFunctors.cpph"
#include "VolumewiseStateless.cpph"

#if defined(GPUOffloadingOMP)
#include "exahype2/fv/rusanov/omp/rusanov.h"
#endif

#if defined(GPUOffloadingCPP)
#include "exahype2/fv/rusanov/cpp/rusanov.h"
#endif

#if defined(GPUOffloadingSYCL)
#include "exahype2/fv/rusanov/sycl/rusanov.h"
#endif
