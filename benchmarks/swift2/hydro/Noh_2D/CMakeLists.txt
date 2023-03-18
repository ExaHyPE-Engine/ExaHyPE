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
set(TARGET_NAME noh2D)
set(DIMENSION 2)
set(MODE RELEASE)
set(GENERATOR noh.py)

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
list(APPEND DEPENDS_CORE peano4 tarch )
list(APPEND DEPENDS_OTHER )

# Non-generated soruce files added to the Peano4 Project
set(GENERATED_FILES repositories/GlobalState.cpp celldata/hydroPartSetCellStatistics.cpp vertexdata/hydroPartSet.cpp globaldata/hydroPart.cpp repositories/DataRepository.cpp ./observers/CreateGrid2peano4_toolbox_CreateRegularGrid0.cpp ./observers/CreateGrid.cpp ./observers/Init2peano4_toolbox_particles_UpdateParallelState0.cpp ./observers/Init2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1.cpp ./observers/Init2swift2_input_InsertParticlesByCoordinates2.cpp ./observers/Init.cpp ./observers/Sort2peano4_toolbox_particles_UpdateParallelState0.cpp ./observers/Sort2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1.cpp ./observers/Sort.cpp ./observers/Plot2peano4_toolbox_particles_UpdateParallelState0.cpp ./observers/Plot2peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1.cpp ./observers/Plot2peano4_toolbox_PlotGridInPeanoBlockFormat2.cpp ./observers/Plot2peano4_toolbox_particles_PlotParticlesInVTKFormat3.cpp ./observers/Plot.cpp ./observers/StephydroPartFlagBoundaryParticles_sweep02swift2_actionsets_UpdateParticleMarker0.cpp ./observers/StephydroPartFlagBoundaryParticles_sweep02peano4_toolbox_particles_ParticlePerCellUpdate1.cpp ./observers/StephydroPartFlagBoundaryParticles_sweep0.cpp ./observers/StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep12swift2_actionsets_UpdateParticleMarker0.cpp ./observers/StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep12peano4_toolbox_particles_ParticleParticleInteraction_Sets1.cpp ./observers/StephydroPartDensityLoopWithConstantSmoothingLengthInitial_sweep1.cpp ./observers/StephydroPartForceLoopInitial_sweep22swift2_actionsets_UpdateParticleMarker0.cpp ./observers/StephydroPartForceLoopInitial_sweep22peano4_toolbox_particles_ParticleParticleInteraction_Sets1.cpp ./observers/StephydroPartForceLoopInitial_sweep2.cpp ./observers/StephydroPartKick1_sweep32swift2_actionsets_UpdateParticleMarker0.cpp ./observers/StephydroPartKick1_sweep32peano4_toolbox_particles_ParticlePerCellUpdate1.cpp ./observers/StephydroPartKick1_sweep3.cpp ./observers/StephydroPartKickExtra1_sweep42swift2_actionsets_UpdateParticleMarker0.cpp ./observers/StephydroPartKickExtra1_sweep42peano4_toolbox_particles_ParticlePerCellUpdate1.cpp ./observers/StephydroPartKickExtra1_sweep4.cpp ./observers/StephydroPartDrift_sweep52peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop0.cpp ./observers/StephydroPartDrift_sweep52swift2_actionsets_UpdateParticleMarker1.cpp ./observers/StephydroPartDrift_sweep52peano4_toolbox_particles_ParticlePerCellUpdate2.cpp ./observers/StephydroPartDrift_sweep5.cpp ./observers/StephydroPartPredictHydro_sweep62peano4_toolbox_particles_UpdateParallelState0.cpp ./observers/StephydroPartPredictHydro_sweep62peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1.cpp ./observers/StephydroPartPredictHydro_sweep62swift2_actionsets_DynamicMeshRefinementAnalysis2.cpp ./observers/StephydroPartPredictHydro_sweep62swift2_actionsets_UpdateParticleMarker3.cpp ./observers/StephydroPartPredictHydro_sweep62peano4_toolbox_particles_ParticlePerCellUpdate4.cpp ./observers/StephydroPartPredictHydro_sweep6.cpp ./observers/StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_UpdateParticleMarker0.cpp ./observers/StephydroPartDensityLoopWithConstantSmoothingLength_sweep72peano4_toolbox_particles_ParticleParticleInteraction_Sets1.cpp ./observers/StephydroPartDensityLoopWithConstantSmoothingLength_sweep72swift2_actionsets_DynamicMeshRefinementTrigger2.cpp ./observers/StephydroPartDensityLoopWithConstantSmoothingLength_sweep7.cpp ./observers/StephydroPartForceLoop_sweep82peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop0.cpp ./observers/StephydroPartForceLoop_sweep82swift2_actionsets_UpdateParticleMarker1.cpp ./observers/StephydroPartForceLoop_sweep82peano4_toolbox_particles_ParticleParticleInteraction_Sets2.cpp ./observers/StephydroPartForceLoop_sweep8.cpp ./observers/StephydroPartKick2_sweep92peano4_toolbox_particles_UpdateParallelState0.cpp ./observers/StephydroPartKick2_sweep92peano4_toolbox_particles_UpdateParticleGridAssociation_LiftDrop1.cpp ./observers/StephydroPartKick2_sweep92swift2_actionsets_UpdateParticleMarker2.cpp ./observers/StephydroPartKick2_sweep92peano4_toolbox_particles_ParticlePerCellUpdate3.cpp ./observers/StephydroPartKick2_sweep9.cpp ./observers/StephydroPartKickExtra2_sweep102swift2_actionsets_UpdateParticleMarker0.cpp ./observers/StephydroPartKickExtra2_sweep102peano4_toolbox_particles_ParticlePerCellUpdate1.cpp ./observers/StephydroPartKickExtra2_sweep10.cpp ./observers/StephydroPartReduceGlobalQuantities_sweep112swift2_actionsets_UpdateParticleMarker0.cpp ./observers/StephydroPartReduceGlobalQuantities_sweep112peano4_toolbox_particles_ParticlePerCellUpdate1.cpp ./observers/StephydroPartReduceGlobalQuantities_sweep11.cpp repositories/StepRepository.cpp Noh-main.cpp )
# Include directories added to a Peano4 Project
set(GENERATED_INCLUDE_DIRS )

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
