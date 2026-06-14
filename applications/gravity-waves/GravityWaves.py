# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2
from CouplingSynchronization import (
    setLastSolver,
    synchronizePatchGravityWaves,
    coupleTimeStepping,
)

R = 8.31446261815324  # ideal gas constant in J / (mol * K)
GRAVITY = 9.8067  # gravitational acceleration in m/s^2

EINSTEIN_COEFFICIENTS = ["goldman", "langhoff", "turnbullLowe"]
CSV_CONSTANTS = """
    // CSV constants
    static const char csvSeparator            = ';';
    static const int csvHeightIndex           = 5;
    static const int csvAtomicOxygenIndex     = 8;
    static const int csvNitrogenIndex         = 9;
    static const int csvMolecularOxygenIndex  = 10;
    static const int csvDensityIndex          = 11;
    static const int csvTemperatureIndex      = 12;
    static const int csvHydrogenIndex         = 16;
  """


def get_CSV_initializer(
    csv_file_path, initialize_density: bool, initialize_hydrogen: bool
):
    density_interpolation = ""
    density_csv = ""

    hydrogen_interpolation = ""
    hydrogen_csv = ""

    if initialize_density:
        density_interpolation = """// density is in g/cm^3, needs conversion into kg/m^3 = 1000 * g/cm^3
        Q[Shortcuts::rho] = 1000.0 * interpolateBetweenRows(row, nextRow, csvDensityIndex, x(1));
        #if !defined(WITH_GPU)
        assertion1(!std::isnan(Q[Shortcuts::rho]), Q[Shortcuts::rho]);
        assertion1(Q[Shortcuts::rho] > 0, Q[Shortcuts::rho]);
        #endif"""
        density_csv = """// density is in g/cm^3, needs conversion into kg/m^3 = 1000 * g/cm^3
        Q[Shortcuts::rho] = 1000.0 * tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(';', csvDensityIndex, row); 
        #if !defined(WITH_GPU)
        assertion1(!std::isnan(Q[Shortcuts::rho]), Q[Shortcuts::rho]);
        assertion1(Q[Shortcuts::rho] > 0, Q[Shortcuts::rho]);
        #endif"""

    if initialize_hydrogen:
        hydrogen_interpolation = """// populate hydrogen as it is also given in the model
        Q[Shortcuts::H]   = interpolateBetweenRows(row, nextRow, csvHydrogenIndex, x(1));"""
        hydrogen_csv = """// populate hydrogen as it is also given in the model
        Q[Shortcuts::H]   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHydrogenIndex, row);"""

    return (
        """
      std::fill_n(Q, Shortcuts::NumberOfDistinctVariables, 0.0);

      // initialize CSV reader and skip first line of file, which contains column names
      static tarch::reader::CSVReader csvReader(\""""
        + csv_file_path
        + """\", 1);
      const int csvNumberOfRows = csvReader.getNumberOfRows();

      // height in CSV is in km, so convert to m
      const double csvMaxHeight = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHeightIndex, csvReader.getRow(csvNumberOfRows - 1)) * 1000.0;
      
      // height of x is in m
      const int rowIndex = tarch::la::round(x(1) / (csvMaxHeight) * (csvNumberOfRows - 1));
      
      // extract row
      const std::string row = csvReader.getRow(rowIndex);

      // if not last row, interpolate between rows for stratification in Euler solver
      const bool interpolate = rowIndex < csvNumberOfRows - 1;

      if (interpolate) {
        const std::string nextRow = csvReader.getRow(rowIndex + 1);

        auto interpolateBetweenRows = [](const std::string bottomRow, const std::string topRow, const int csvIndex, const double currentHeight) {
          const auto bottomRowValue = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvIndex, bottomRow);
          const auto topRowValue    = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvIndex, topRow);
          const auto topRowHeight   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(csvSeparator, csvHeightIndex, topRow) * 1000.0;

          return bottomRowValue + currentHeight * (topRowValue - bottomRowValue) / topRowHeight;
        };

        """
        + density_interpolation
        + """

        // populate major species
        // all number densities retreived from csv are cm^-3
        // all production rates are also in cm^-3 s^-1 
        Q[Shortcuts::O]   = interpolateBetweenRows(row, nextRow, csvAtomicOxygenIndex, x(1));
        Q[Shortcuts::O2]  = interpolateBetweenRows(row, nextRow, csvMolecularOxygenIndex, x(1));
        Q[Shortcuts::N2]  = interpolateBetweenRows(row, nextRow, csvNitrogenIndex, x(1));
        """
        + hydrogen_interpolation
        + """
        // temperature is in Kelvin
        Q[Shortcuts::T]   = interpolateBetweenRows(row, nextRow, csvTemperatureIndex, x(1));
      } else {

        """
        + density_csv
        + """    

        // populate major species
        // all number densities retreived from csv are cm^-3
        // all production rates are also in cm^-3 s^-1 
        Q[Shortcuts::O2]  = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(';', csvMolecularOxygenIndex, row);
        Q[Shortcuts::O]   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(';', csvAtomicOxygenIndex, row);
        Q[Shortcuts::N2]  = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(';', csvNitrogenIndex, row);
        """
        + hydrogen_csv
        + """
        // temperature is in Kelvin
        Q[Shortcuts::T]   = tarch::reader::CSVReader::extractFundamentalTypeFromRow<double>(';', csvTemperatureIndex, row);
      }
"""
    )


def get_atmospheric_boundary_conditions(outside_accessor="Qoutside"):
    return (
        """
    // normal == 0 => left
    // normal == 1 => bottom
    // normal == 2 => right 
    // normal == 3 => top

    for (int unknown = 0; unknown < NumberOfUnknowns + NumberOfAuxiliaryVariables; ++unknown) {
      """
        + outside_accessor
        + """[unknown] = Qinside[unknown];
    }

    if (normal == 1) {
      // Reflective at bottom to keep atmosphere
      // from escaping the domain through the Earth's surface
      """
        + outside_accessor
        + """[Shortcuts::u] = Qinside[Shortcuts::u];
      """
        + outside_accessor
        + """[Shortcuts::v] = -Qinside[Shortcuts::v];
      #if DIMENSIONS == 3
      """
        + outside_accessor
        + """[Shortcuts::w] = Qinside[Shortcuts::w];
      #endif
    }
"""
    )


def create_peano_project(
    project, atmospheric_solver, diffusion_solver, chemical_solver
):
    for solver in (atmospheric_solver, diffusion_solver, chemical_solver):
        if solver is not None:
            solver._dimensions = project.dimensions

    setLastSolver(
        atmospheric_solver=atmospheric_solver,
        diffusion_solver=diffusion_solver,
        chemical_solver=chemical_solver,
    )

    if atmospheric_solver is not None:
        project.add_solver(atmospheric_solver)

    if diffusion_solver is not None:
        project.add_solver(diffusion_solver)

    if chemical_solver is not None:
        project.add_solver(chemical_solver)

    coupleTimeStepping(
        atmospheric_solver=atmospheric_solver,
        diffusion_solver=diffusion_solver,
        chemical_solver=chemical_solver,
    )

    synchronizePatchGravityWaves(
        atmospheric_solver=atmospheric_solver,
        diffusion_solver=diffusion_solver,
        chemical_solver=chemical_solver,
    )

    project = project.generate_Peano4_project(verbose=True)

    project.output.makefile.add_CXX_flag(f"""-DGRAVITY={GRAVITY}""")
    project.output.makefile.add_CXX_flag(f"""-DUNIVERSAL_GAS_CONSTANT={R}""")
    project.output.makefile.add_CXX_flag(
        f"""-DMOLAR_MASS_ATOMIC_OXYGEN=0.016"""
    )  # in kg/mol
    project.output.makefile.add_CXX_flag(
        f"""-DMOLAR_MASS_MOLECULAR_OXYGEN=0.032"""
    )  # in kg/mol
    project.output.makefile.add_CXX_flag(
        f"""-DMOLAR_MASS_NITROGEN=0.028"""
    )  # in kg/mol
    return project
