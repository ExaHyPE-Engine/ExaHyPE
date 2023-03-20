# **********************************************************************************************
# ***                                     !!!WARNING!!!                                      ***
# *** WARNING: AUTO GENERATED FILE! DO NOT MODIFY BY HAND! YOUR CHANGES WILL BE OVERWRITTEN! ***
# ***                                     !!!WARNING!!!                                      ***
# **********************************************************************************************
function(capitalize value)
    string(TOUPPER ${value} value_upper)
    string(SUBSTRING ${value_upper} 0 1 value_capitalized)
    string(SUBSTRING ${value} 1 -1 value_rest)
    string(TOLOWER ${value_rest} value_rest)
    string(APPEND value_capitalized ${value_rest})
    set(value_capitalized ${value_capitalized} PARENT_SCOPE)
endfunction()

find_package(PythonInterp 3.6 REQUIRED)
find_package(PythonLibs 3.6 REQUIRED)

if(NOT(PYTHON_LIBRARY AND PYTHON_INCLUDE_DIR))
    message(FATAL_ERROR "Couldn't find any Python3 library")
endif()

execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import jinja2" RESULT_VARIABLE JINJA2_FOUND)

# We need Jinja2 for code generation
if(NOT JINJA2_FOUND EQUAL 0)
    message(WARNING "Jinja2 package (needed for code generation) not found. Please install Jinja2 e.g.: 'pip3 install jinja2'")
endif()

# Mode, Generator Scripts, Dimensions and the Executable Name (Target_name) are variables of a Peano4 Project
# And they are necessary for the cimpiling
set(TARGET_NAME benchmark-plot-0-no-opt-asserts-FD4-5)
set(DIMENSION 3)
set(MODE ASSERTS)
set(GENERATOR create-benchmark-executable.py)

# These checks are performed to check whether the generated executable can be built with the configured core project,
# This means that the build_mode and the dimension of the generated application should be the same as the configured
# Project or core project's mode should be 'All' and dimension '2 and 3'
if (NOT ${MODE_LOWER} IN_LIST BUILD_MODES)
    capitalize(${MODE})
    message(FATAL_ERROR "Build mode of the generated Peano4 Project ${TARGET_NAME}'s build mode (${MODE}) is different than\
 the build of the configured CMake project. Please reconfigure the CMake project with -DCMAKE_BUILD_TYPE=${value_capitalized}\
 or -DCMAKE_BUILD_TYPE=All")
endif()

if (NOT ${DIM} IN_LIST PEANO_DIMENSIONS)
    message(FATAL_ERROR "Dimension of the generated Peano4 Project ${TARGET_NAME}'s dimension (${DIM}) is different than\
 the build of the configured CMake project. Please reconfigure the CMake project with -DCMAKE_BUILD_TYPE=${DIM} or\
 -DCMAKE_BUILD_TYPE='2 and 3'")
endif()

# Libraries used are taken from Peano4 project too, they are added without build more or dimensions
# Possible examples are Tarch, Peano4Core, ToolboxMultiprecision etc.
list(APPEND DEPENDS_CORE peano4 tarch exahype2 )
list(APPEND DEPENDS_OTHER )

# Non-generated soruce files added to the Peano4 Project
set(GENERATED_FILES AbstractSelfSimilarInfallFD4.cpp SelfSimilarInfallFD4.cpp tasks/SelfSimilarInfallFD4EnclaveTask.cpp repositories/SolverRepository.cpp ../../../../applications/exahype2/euler/EulerKernels.cpp ../../../../applications/exahype2/euler/spherical-accretion/MassAccumulator.cpp ../../../../applications/exahype2/euler/spherical-accretion/GravityModel.cpp celldata/SelfSimilarInfallFD4CellLabel.cpp celldata/SelfSimilarInfallFD4Q.cpp celldata/SelfSimilarInfallFD4QRhsEstimates.cpp facedata/SelfSimilarInfallFD4QOld.cpp facedata/SelfSimilarInfallFD4QNew.cpp facedata/SelfSimilarInfallFD4QUpdate.cpp facedata/SelfSimilarInfallFD4FaceLabel.cpp repositories/DataRepository.cpp ./observers/CreateGrid2exahype2_solvers_rkfd_actionsets_InitialCondition0.cpp ./observers/CreateGrid2exahype2_grid_FaceLabel1.cpp ./observers/CreateGrid2exahype2_grid_CellLabel2.cpp ./observers/CreateGrid2exahype2_solvers_rkfd_actionsets_AdaptivityCriterion3.cpp ./observers/CreateGrid.cpp ./observers/InitGrid2exahype2_solvers_rkfd_actionsets_DynamicAMR0.cpp ./observers/InitGrid2peano4_toolbox_blockstructured_BackupPatchOverlap1.cpp ./observers/InitGrid2exahype2_solvers_rkfd_actionsets_RollOverUpdatedFace2.cpp ./observers/InitGrid2exahype2_solvers_rkfd_actionsets_InitialCondition3.cpp ./observers/InitGrid2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces4.cpp ./observers/InitGrid2exahype2_grid_FaceLabel5.cpp ./observers/InitGrid2exahype2_grid_CellLabel6.cpp ./observers/InitGrid2exahype2_solvers_rkfd_actionsets_AdaptivityCriterion7.cpp ./observers/InitGrid2exahype2_solvers_rkfd_actionsets_PostprocessSolution8.cpp ./observers/InitGrid.cpp ./observers/CreateGridButPostponeRefinement2exahype2_solvers_rkfd_actionsets_InitialCondition0.cpp ./observers/CreateGridButPostponeRefinement2exahype2_grid_FaceLabel1.cpp ./observers/CreateGridButPostponeRefinement2exahype2_grid_CellLabel2.cpp ./observers/CreateGridButPostponeRefinement2exahype2_solvers_rkfd_actionsets_AdaptivityCriterion3.cpp ./observers/CreateGridButPostponeRefinement.cpp ./observers/CreateGridAndConvergeLoadBalancing2exahype2_solvers_rkfd_actionsets_InitialCondition0.cpp ./observers/CreateGridAndConvergeLoadBalancing2exahype2_grid_FaceLabel1.cpp ./observers/CreateGridAndConvergeLoadBalancing2exahype2_grid_CellLabel2.cpp ./observers/CreateGridAndConvergeLoadBalancing2exahype2_solvers_rkfd_actionsets_AdaptivityCriterion3.cpp ./observers/CreateGridAndConvergeLoadBalancing.cpp ./observers/PlotSolution2exahype2_solvers_rkfd_actionsets_DynamicAMR0.cpp ./observers/PlotSolution2exahype2_grid_FaceLabel1.cpp ./observers/PlotSolution2exahype2_grid_CellLabel2.cpp ./observers/PlotSolution2peano4_toolbox_blockstructured_PlotPatchesInPeanoBlockFormat3.cpp ./observers/PlotSolution.cpp ./observers/TimeStep2exahype2_solvers_rkfd_SeparateSweepsWithEnclaveTasking_MergeEnclaveTaskOutcome0.cpp ./observers/TimeStep2exahype2_solvers_rkfd_actionsets_DynamicAMR1.cpp ./observers/TimeStep2exahype2_solvers_rkfd_actionsets_RollOverUpdatedFace2.cpp ./observers/TimeStep2exahype2_grid_FaceLabel3.cpp ./observers/TimeStep2exahype2_grid_CellLabel4.cpp ./observers/TimeStep2exahype2_solvers_rkfd_actionsets_HandleBoundary5.cpp ./observers/TimeStep2exahype2_solvers_rkfd_SeparateSweepsWithEnclaveTasking_UpdateCell6.cpp ./observers/TimeStep2exahype2_solvers_rkfd_actionsets_ProjectPatchOntoFaces7.cpp ./observers/TimeStep2exahype2_solvers_rkfd_actionsets_ComputeFinalLinearCombination8.cpp ./observers/TimeStep2exahype2_solvers_rkfd_actionsets_AdaptivityCriterion9.cpp ./observers/TimeStep2exahype2_solvers_rkfd_actionsets_PostprocessSolution10.cpp ./observers/TimeStep.cpp repositories/StepRepository.cpp benchmark-main.cpp )
# Include directories added to a Peano4 Project
set(GENERATED_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/../../../../applications/exahype2/euler )

# Mode name is saved with uppercase in Peano4 Project, but libraries have the suffix _ + $mode in lower case, we need to the same
string(TOLOWER ${MODE} MODE_LOWER)

# Add Core libraries with right dimensions and build modes
# For Tarch
if("tarch" IN_LIST DEPENDS_CORE)
    list(APPEND DEPENDS_FULL "Tarch${SUFFIX_${mode}}")
endif()

# For ExaHyPE
if("exahype2" IN_LIST DEPENDS_CORE)
    list(APPEND DEPENDS_FULL "ExaHyPE2Core${DIMENSION}d${SUFFIX_${mode}}")
endif()

# For Peano4
if("peano4" IN_LIST DEPENDS_CORE)
    list(APPEND DEPENDS_FULL "Peano4Core${DIMENSION}d${SUFFIX_${mode}}")
endif()

# Toolboxes etc.
list(APPEND DEPENDS_FULL ${DEPENDS_OTHER})

# Create executable as a CMake target
add_executable(${TARGET_NAME})

# Set sources of the executable, we depend in the generated CMake file, the generator script,
# We also depend on the generated and non generated files 
target_sources(${TARGET_NAME} PRIVATE
    ${GENERATED_FILES}
    ${GENERATOR}
)

# Set default include directories as well as the generated ones
target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/src ${GENERATED_INCLUDE_DIRS}
)

# Link to full target libraries
target_link_libraries(${TARGET_NAME} PRIVATE ${DEPENDS_FULL})

# We need to set the dimension, the build mode, global build flags of the mode as well as the link flags
target_compile_definitions(${TARGET_NAME} PRIVATE Dimensions=${DIMENSION} ${${mode}_DEFINITIONS})
target_compile_options(${TARGET_NAME} PRIVATE ${${mode}_FLAGS})
target_link_options(${TARGET_NAME} PRIVATE ${${mode}_LDFLAGS})

# Libraries are generated at $cmake_binary_dir, just in case add it to the link directories
target_link_directories(${TARGET_NAME} PRIVATE ${CMAKE_BINARY_DIR})
