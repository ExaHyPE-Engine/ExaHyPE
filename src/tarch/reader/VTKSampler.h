// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

namespace tarch::reader {
  class VTKSampler;
} // namespace tarch::reader

/**
 * @brief Samples one time step of a previous run's plotter output.
 *
 * The typical use case is a warm start: instead of deriving the initial
 * condition analytically, a solver reads the state a previous run has dumped
 * and continues from there.
 *
 * The sampler understands both families of VTK output Peano writes:
 *
 * - the growing VTKHDF file of tarch::plotter::VTKHDFFileWriter, i.e. one
 *   <solver>.vtkhdf holding all steps, and
 * - the VTU output of tarch::plotter::VTUFileWriter, i.e. a <solver>.pvd
 *   collection which references one .pvtu per step, which in turn references
 *   one .vtu per rank. A .pvtu or a plain .vtu can be passed directly, too, in
 *   which case the sampler sees a file with a single step.
 *
 * The format follows from the file extension, so the only thing that changes
 * between the two is the path handed to the constructor:
 *
 *     previous-run/patch/ADERDGSolver.vtkhdf
 *     previous-run/patch/ADERDGSolver.pvd
 *
 * .vtkhdf requires a build with HDF5 support (PEANO_WITH_HDF5), which is also
 * the prerequisite for writing such a file in the first place. The VTU branch
 * has no such prerequisite, though ZLib (PEANO_WITH_ZLIB) is needed to read the
 * compressed files a ZLib-enabled build writes.
 *
 * The sampler picks one step (the last one by default), loads that step's
 * geometry plus the requested data arrays into memory, and answers
 * nearest-neighbour queries on them. It never interpolates: a query returns the
 * value stored at the closest sample point.
 *
 * Nearest neighbour is the natural choice for a restart because the plotters
 * dump exactly the solver's own degrees of freedom:
 *
 * - the nodal (ADER-DG, Runge-Kutta DG) patch plotter writes one point per
 *   quadrature point, which is exactly where ExaHyPE evaluates the initial
 *   condition, so an unchanged mesh and order reproduce the stored state one
 *   to one, and
 * - the cell (finite volume/difference) patch plotter writes one cell per
 *   finite volume, which the sampler represents through its centroid, i.e. the
 *   point where the initial condition of a volume is evaluated.
 *
 * Which of the two the sampler uses follows from the requested fields: a name
 * is looked up in the file's PointData first and then in its CellData, and the
 * sample points are the plotted vertices or the cells' centroids accordingly.
 * All requested fields have to share one association, since a point index has
 * to mean the same thing for all of them.
 *
 * Change the mesh, the order or the patch size between the two runs and the
 * restart degenerates into a piecewise-constant projection of the old state -
 * still a usable warm start, but no longer the same discrete solution.
 *
 * Two further caveats are worth keeping in mind. The plotters write single
 * precision by default, so the restart is only accurate to about seven digits,
 * and what they dump is the solution of the *previous* time step, stamped with
 * the average of the solvers' minimal and maximal time stamp. A restart
 * therefore never continues the old run bit-wise; it continues it physically.
 *
 * The constructor does all the I/O, so the intended use is a function-local
 * static in the initial condition (thread-safe initialisation, one read per
 * rank). All query routines are const and free of shared state, hence safe to
 * call from ExaHyPE's multithreaded traversals.
 *
 *     static tarch::reader::VTKSampler sampler(
 *       "previous-run/patch/ADERDGSolver.pvd", {"rho", "j", "E"}
 *     );
 *     const int point = sampler.getClosestPoint(x(0), x(1));
 *     Q[0] = sampler.getValue("rho", 0, point);
 *
 * Sampling all components of one location through a single getClosestPoint()
 * costs one search instead of one search per component, which matters when the
 * initial condition restores a dozen unknowns.
 */
class tarch::reader::VTKSampler {
public:
  /**
   * Step argument selecting the last step in the file. Negative steps count
   * from the end in general, i.e. -2 is the last but one.
   */
  static constexpr int LastStep = -1;

  /**
   * @brief Reads one step of a VTKHDF or VTU data set.
   *
   * @param filePath   path of the file, e.g.
   *                   "MySimulation.output/patch/ADERDGSolver.vtkhdf" or
   *                   "MySimulation.output/patch/ADERDGSolver.pvd". The
   *                   extension selects the reader: .vtkhdf (also .h5, .hdf5,
   *                   .hdf) versus .pvd, .pvtu and .vtu.
   * @param fieldNames names of the data arrays to load. These are the plot
   *                   descriptors of the solver, i.e. its unknown and
   *                   auxiliary variable names. Loading only what is needed
   *                   keeps the memory footprint down.
   * @param step       index of the step to read. Negative values count from
   *                   the end, so the default reads the last dumped step. A
   *                   .vtkhdf holds all steps, a .pvd knows the steps of its
   *                   collection, and a lone .pvtu or .vtu is one step.
   */
  VTKSampler(const std::string& filePath, const std::vector<std::string>& fieldNames, int step = LastStep);

  VTKSampler(const std::string& filePath, std::initializer_list<std::string> fieldNames, int step = LastStep);

  /**
   * @brief Index of the sample point closest to a position.
   *
   * The index is only meaningful in combination with getValue(). It is stable
   * for the lifetime of the sampler.
   */
  int getClosestPoint(double x, double y, double z = 0.0) const;

  /**
   * @brief Value of one component of one field at a point index.
   *
   * @param fieldName name as passed to the constructor.
   * @param component component within the field. Scalars only have component
   *                  0, while a vector of a 2d run still has three components
   *                  in the file (the third one is zero padding).
   * @param point     index obtained from getClosestPoint().
   */
  double getValue(const std::string& fieldName, int component, int point) const;

  /**
   * @brief Shorthand for getValue(fieldName, component, getClosestPoint(x,y,z)).
   *
   * Convenient for a one-off lookup. Prefer the two-step variant whenever
   * several components are read at the same position.
   */
  double sample(const std::string& fieldName, int component, double x, double y, double z = 0.0) const;

  /**
   * @brief Simulation time of the step that has been read.
   *
   * Use it to shift any explicitly time-dependent term of the restarted run,
   * whose own clock starts at zero again.
   *
   * A VTKHDF file stores the time stamps of its steps, and so does the .pvd of
   * a VTU series. A .pvtu or .vtu passed directly only carries the time in its
   * file name (<name>-<seconds>_<fraction>), which is where the sampler then
   * takes it from.
   */
  double getTimeStamp() const;

  /**
   * @brief Index of the step that has been read, after resolving LastStep.
   */
  int getStep() const;

  /**
   * @brief Number of steps in the file.
   */
  int getNumberOfSteps() const;

  /**
   * @brief Number of sample points of the step that has been read.
   */
  int getNumberOfPoints() const;

  /**
   * @brief Number of components a field holds in the file.
   */
  int getNumberOfComponents(const std::string& fieldName) const;

  /**
   * @brief Whether the sampled fields are cell data, i.e. whether the sample
   *        points are cell centroids rather than plotted vertices.
   */
  bool samplesCellData() const;

private:
  struct Field {
    std::vector<double> values;
    int                 numberOfComponents = 0;
  };

  std::string _filePath;
  int         _step            = 0;
  int         _numberOfSteps   = 0;
  double      _timeStamp       = 0.0;
  std::size_t _numberOfPoints  = 0;
  bool        _samplesCellData = false;

  /**
   * Positions of the sample points, three entries per point: the plotted
   * vertices for point data, the cells' centroids for cell data.
   */
  std::vector<double> _positions;

  std::unordered_map<std::string, Field> _fields;

  /**
   * Uniform bucket grid over the bounding box of _positions, stored as a CSR
   * structure: bucket b holds the point indices
   * _pointsInBuckets[_bucketStart[b]] ... _pointsInBuckets[_bucketStart[b+1]).
   */
  double           _boundingBoxMin[3]  = {0.0, 0.0, 0.0};
  double           _bucketSize[3]      = {1.0, 1.0, 1.0};
  int              _numberOfBuckets[3] = {1, 1, 1};
  std::vector<int> _bucketStart;
  std::vector<int> _pointsInBuckets;

  /**
   * Dispatches to the reader that matches the file's extension. Both readers
   * fill the same members, i.e. everything behind them is format-agnostic.
   */
  void read(const std::vector<std::string>& fieldNames, int step);

  /** @see VTKSampler.cpp resp. VTKSamplerVTKHDF.cpp and VTKSamplerVTU.cpp */
  void readVTKHDF(const std::vector<std::string>& fieldNames, int step);
  void readVTU(const std::vector<std::string>& fieldNames, int step);

  void createLookupStructure();

  int getBucketIndex(int i, int j, int k) const;
};
