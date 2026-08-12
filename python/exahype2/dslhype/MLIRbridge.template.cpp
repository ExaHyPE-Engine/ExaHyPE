#include <iostream>
#include <cstring>
#include <cstddef>
#include <omp.h>

#ifdef MLIR_GPU
#include <cuda_runtime.h>
#include <cuda.h>
#endif

{{SOLVER_INCLUDES}}

// Include physics implementations directly first
#include "Constants.h" 
#include "{{SOLVER_CLASS}}.h"

// Include memref descriptor definitions (used below)
#include "MemRefDescriptor.cpph"

#include "MLIRbridge.cpph"

// Global variable definitions for MLIR
// Initialized from Constants.h to match C++ template parameters
extern "C" {
    {{TEMPLATE_VALUE_DEFINITIONS}}
}

#ifdef MLIR_GPU
extern void setupMLIRContextGPU(CUdevice);
extern void releaseMLIRContextGPU(CUdevice);
#endif

// Use existing classes and enums
using OffloadableType = exahype2::Solver::Offloadable;

// GPU device bridge functions for MLIR GPU kernel integration
// These functions are compiled as device code via OpenMP offloading
// They can be called from within MLIR-generated GPU kernels
#pragma omp declare target
extern "C" {
    void flux_bridge(void* Q_ptr, void* x_ptr, void* h_ptr, double t, double dt, int normal, void* F_ptr) {
        double* Q = static_cast<double*>(Q_ptr);
        tarch::la::Vector<Dimensions, double>* x = static_cast<tarch::la::Vector<Dimensions, double>*>(x_ptr);
        tarch::la::Vector<Dimensions, double>* h = static_cast<tarch::la::Vector<Dimensions, double>*>(h_ptr);
        double* F = static_cast<double*>(F_ptr);
        {{SOLVER_CLASS_NAMESPACE}}::Abstract{{SOLVER_CLASS}}::flux(Q, *x, *h, t, dt, normal, F, OffloadableType::Yes);
    }
    
    void nonconservativeProduct_bridge(void* Q_ptr, void* deltaQ_ptr, void* x_ptr, void* h_ptr, double t, double dt, int normal, void* result_ptr) {
        double* Q = static_cast<double*>(Q_ptr);
        double* deltaQ = static_cast<double*>(deltaQ_ptr);
        tarch::la::Vector<Dimensions, double>* x = static_cast<tarch::la::Vector<Dimensions, double>*>(x_ptr);
        tarch::la::Vector<Dimensions, double>* h = static_cast<tarch::la::Vector<Dimensions, double>*>(h_ptr);
        double* result = static_cast<double*>(result_ptr);
        {{SOLVER_CLASS_NAMESPACE}}::Abstract{{SOLVER_CLASS}}::nonconservativeProduct(Q, deltaQ, *x, *h, t, dt, normal, result, OffloadableType::Yes);
    }
    
    void sourceTerm_bridge(void* Q_ptr, void* x_ptr, void* h_ptr, double t, double dt, void* S_ptr) {
        double* Q = static_cast<double*>(Q_ptr);
        tarch::la::Vector<Dimensions, double>* x = static_cast<tarch::la::Vector<Dimensions, double>*>(x_ptr);
        tarch::la::Vector<Dimensions, double>* h = static_cast<tarch::la::Vector<Dimensions, double>*>(h_ptr);
        double* S = static_cast<double*>(S_ptr);
        {{SOLVER_CLASS_NAMESPACE}}::Abstract{{SOLVER_CLASS}}::sourceTerm(Q, *x, *h, t, dt, S, OffloadableType::Yes);
    }
    
    void maxEigenvalue_bridge(void* Q_ptr, void* x_ptr, void* h_ptr, double t, double dt, int normal, void* result_ptr) {
        double* Q = static_cast<double*>(Q_ptr);
        tarch::la::Vector<Dimensions, double>* x = static_cast<tarch::la::Vector<Dimensions, double>*>(x_ptr);
        tarch::la::Vector<Dimensions, double>* h = static_cast<tarch::la::Vector<Dimensions, double>*>(h_ptr);
        double* result = static_cast<double*>(result_ptr);
        {{SOLVER_CLASS_NAMESPACE}}::Abstract{{SOLVER_CLASS}}::maxEigenvalue(Q, *x, *h, t, dt, normal, result, OffloadableType::Yes);
    }
}
#pragma omp end declare target

// MLIR-generated function declaration (parsed from functor_arguments)
// Note: Function pointers removed - physics functions are compiled into GPU kernels
// First parameter is targetDevice (GPU device ID), not function pointers
extern "C" void {{KERNEL_NAME}}_gpu(
    void* patchData
);

extern "C" void {{KERNEL_NAME}}(
    MLIRCellData* patchData,
    void (*flux_fn)(void*, void*, void*, double, double, int, void*),
    void (*ncp_fn)(void*, void*, void*, void*, double, double, int, void*),
    void (*source_fn)(void*, void*, void*, double, double, void*),
    void (*maxeig_fn)(void*, void*, void*, double, double, int, void*)
);

extern "C" void {{KERNEL_NAME}}_omp(
    MLIRCellData* patchData,
    void (*flux_fn)(void*, void*, void*, double, double, int, void*),
    void (*ncp_fn)(void*, void*, void*, void*, double, double, int, void*),
    void (*source_fn)(void*, void*, void*, double, double, void*),
    void (*maxeig_fn)(void*, void*, void*, double, double, int, void*)
);

// Layout-aware conversion creating proper memref descriptors
// Note: This function allocates memory that must be cleaned up by the caller
void convertToMLIRStruct(exahype2::CellData<double, double>* patchData, MLIRCellData* mlir_data) {
    double* qin_storage = nullptr;
    double* qout_storage = nullptr;
    // Calculate dimensions for the data arrays
    int64_t totalCells = patchData->numberOfCells;
    int64_t numUnknowns = static_cast<int64_t>(NumberOfUnknowns + NumberOfAuxiliaryVariables);
    int64_t volumes = static_cast<int64_t>({{PATCH_SIZE}});
    int64_t haloSize = static_cast<int64_t>({{HALO_SIZE}});
    
    int64_t inputSizePerCell = (volumes + 2*haloSize) * (volumes + 2*haloSize) * (volumes + 2*haloSize) * numUnknowns;  // 18*18*18*5
    int64_t outputSizePerCell = volumes * volumes * volumes * numUnknowns;  // 16*16*16*5
    
    // Allocate contiguous memory each time (caller will clean up)
    qin_storage = new double[totalCells * inputSizePerCell];
    qout_storage = new double[totalCells * outputSizePerCell]();
    
    // Copy QIn data to contiguous array
    for (int i = 0; i < totalCells; i++) {
        for (int j = 0; j < inputSizePerCell; j++) {
            qin_storage[i * inputSizePerCell + j] = patchData->QIn[i][j];
        }
    }
    
    // Create memref descriptors for contiguous arrays directly
    mlir_data->QIn.allocated = qin_storage;
    mlir_data->QIn.aligned = qin_storage;
    mlir_data->QIn.offset = 0;
    mlir_data->QIn.sizes[0] = totalCells;
    mlir_data->QIn.sizes[1] = inputSizePerCell;  
    mlir_data->QIn.strides[0] = inputSizePerCell;  // Row-major: stride to next row 
    mlir_data->QIn.strides[1] = 1;                 // Stride to next column
    
    // Create separate memref descriptor for QOut with its own memory
    mlir_data->QOut.allocated = qout_storage;
    mlir_data->QOut.aligned = qout_storage;
    mlir_data->QOut.offset = 0;
    mlir_data->QOut.sizes[0] = totalCells;
    mlir_data->QOut.sizes[1] = outputSizePerCell;  
    mlir_data->QOut.strides[0] = outputSizePerCell;
    mlir_data->QOut.strides[1] = 1;
    
    // Fields 2-3: Create 1D descriptors for cellCentre and cellSize
    double** cellCentrePtr = new double*[patchData->numberOfCells]; 
    for (int i = 0; i < patchData->numberOfCells; i++) {
        cellCentrePtr[i] = (double*)&(patchData->cellCentre[i]);
    }

    mlir_data->cellCentre.allocated = cellCentrePtr;
    mlir_data->cellCentre.aligned = cellCentrePtr;
    mlir_data->cellCentre.offset = 0;
    mlir_data->cellCentre.sizes[0] = patchData->numberOfCells;
    mlir_data->cellCentre.strides[0] = 1;  // One pointer per element
    
    double** cellSizePtr = new double*[patchData->numberOfCells]; 
    for (int i = 0; i < patchData->numberOfCells; i++) {
        cellSizePtr[i] = (double*)&(patchData->cellSize[i]);
    }
    
    mlir_data->cellSize.allocated = cellSizePtr;
    mlir_data->cellSize.aligned = cellSizePtr;
    mlir_data->cellSize.offset = 0;
    mlir_data->cellSize.sizes[0] = patchData->numberOfCells;
    mlir_data->cellSize.strides[0] = 1;  // One pointer per element
    
    // Fields 4-6: Initialize with proper data from patchData
    mlir_data->t.allocated = patchData->t;
    mlir_data->t.aligned = patchData->t;
    mlir_data->t.offset = 0;
    mlir_data->t.sizes[0] = patchData->numberOfCells;
    mlir_data->t.strides[0] = 1;
    
    mlir_data->dt.allocated = patchData->dt;
    mlir_data->dt.aligned = patchData->dt;
    mlir_data->dt.offset = 0;
    mlir_data->dt.sizes[0] = patchData->numberOfCells;
    mlir_data->dt.strides[0] = 1;
    
    mlir_data->id = MemRefDescriptor1D();          // Field 6 (placeholder - unused)
    
    // Fields 10-11: Set original double** pointers for legacy support
    mlir_data->QOut_legacy = patchData->QOut;               // Field 10: Original double** for legacy
    mlir_data->maxEigenvalue = patchData->maxEigenvalue;    // Field 11: Original double* for maxEigenvalue
    
    // Copy scalar fields using direct struct field access
    mlir_data->numberOfCells = static_cast<int64_t>(patchData->numberOfCells);
    mlir_data->memoryLocation = static_cast<int64_t>(patchData->memoryLocation);
    mlir_data->targetDevice = static_cast<int64_t>(patchData->targetDevice);
    
}



// void convertToMLIRStructGPU(exahype2::CopyCellDataGPU<double, double>* copyCellData, MLIRCellData* mlir_data) {
void convertToMLIRStructGPU(exahype2::CellData<double, double>* cellData,
                             MLIRCellData* mlir_data) {
    int64_t totalCells        = cellData->numberOfCells;
    int64_t volumes           = static_cast<int64_t>({{PATCH_SIZE}});
    int64_t haloSize          = static_cast<int64_t>({{HALO_SIZE}});
    int64_t numUnknowns       = static_cast<int64_t>(NumberOfUnknowns + NumberOfAuxiliaryVariables);
    int64_t inputSizePerCell  = (volumes + 2*haloSize) * (volumes + 2*haloSize) 
                                * (volumes + 2*haloSize) * numUnknowns;
    int64_t outputSizePerCell = volumes * volumes * volumes * numUnknowns;

    // Allocate contiguous host memory 
    double* qin_storage  = new double[totalCells * inputSizePerCell];
    double* qout_storage = new double[totalCells * outputSizePerCell]();

    // Copy scattered QIn patches into contiguous buffer
    for (int i = 0; i < totalCells; i++) {
        for (int j = 0; j < inputSizePerCell; j++) {
            qin_storage[i * inputSizePerCell + j] = cellData->QIn[i][j];
        }
    }

    mlir_data->QIn.allocated  = qin_storage;
    mlir_data->QIn.aligned    = qin_storage;
    mlir_data->QIn.offset     = 0;
    mlir_data->QIn.sizes[0]   = totalCells;
    mlir_data->QIn.sizes[1]   = inputSizePerCell;
    mlir_data->QIn.strides[0] = inputSizePerCell;
    mlir_data->QIn.strides[1] = 1;

    mlir_data->QOut.allocated  = qout_storage;
    mlir_data->QOut.aligned    = qout_storage;
    mlir_data->QOut.offset     = 0;
    mlir_data->QOut.sizes[0]   = totalCells;
    mlir_data->QOut.sizes[1]   = outputSizePerCell;
    mlir_data->QOut.strides[0] = outputSizePerCell;
    mlir_data->QOut.strides[1] = 1;

    double** cellCentrePtr = new double*[totalCells];
    for (int i = 0; i < totalCells; i++) {
        cellCentrePtr[i] = (double*)&(cellData->cellCentre[i]);
    }
    mlir_data->cellCentre.allocated  = cellCentrePtr;
    mlir_data->cellCentre.aligned    = cellCentrePtr;
    mlir_data->cellCentre.offset     = 0;
    mlir_data->cellCentre.sizes[0]   = totalCells;
    mlir_data->cellCentre.strides[0] = 1;

    double** cellSizePtr = new double*[totalCells];
    for (int i = 0; i < totalCells; i++) {
        cellSizePtr[i] = (double*)&(cellData->cellSize[i]);
    }
    mlir_data->cellSize.allocated  = cellSizePtr;
    mlir_data->cellSize.aligned    = cellSizePtr;
    mlir_data->cellSize.offset     = 0;
    mlir_data->cellSize.sizes[0]   = totalCells;
    mlir_data->cellSize.strides[0] = 1;

    mlir_data->t.allocated  = cellData->t;
    mlir_data->t.aligned    = cellData->t;
    mlir_data->t.offset     = 0;
    mlir_data->t.sizes[0]   = totalCells;
    mlir_data->t.strides[0] = 1;

    mlir_data->dt.allocated  = cellData->dt;
    mlir_data->dt.aligned    = cellData->dt;
    mlir_data->dt.offset     = 0;
    mlir_data->dt.sizes[0]   = totalCells;
    mlir_data->dt.strides[0] = 1;

    mlir_data->id.allocated  = (double*)cellData->id;
    mlir_data->id.aligned    = (double*)cellData->id;
    mlir_data->id.offset     = 0;
    mlir_data->id.sizes[0]   = totalCells;
    mlir_data->id.strides[0] = 1;

    mlir_data->numberOfCells  = totalCells;
    mlir_data->memoryLocation = static_cast<int64_t>(cellData->memoryLocation);
    mlir_data->targetDevice   = static_cast<int64_t>(cellData->targetDevice);
    mlir_data->maxEigenvalue  = cellData->maxEigenvalue;
    mlir_data->QOut_legacy    = nullptr;
}

void cleanupMLIRStruct(MLIRCellData* mlir_data) {
    if (mlir_data->cellCentre.allocated != nullptr) {
        delete[] mlir_data->cellCentre.allocated;
    }
    if (mlir_data->cellSize.allocated != nullptr) {
        delete[] mlir_data->cellSize.allocated;
    }
}

void cleanupMLIRStructGPU(MLIRCellData* mlir_data) {
    if (mlir_data->QIn.allocated != nullptr) {
        delete[] mlir_data->QIn.allocated;
    }

    if (mlir_data->QOut.allocated != nullptr) {
        delete[] mlir_data->QOut.allocated;
    }

    if (mlir_data->cellCentre.allocated != nullptr) {
        delete[] static_cast<double**>(mlir_data->cellCentre.allocated);
    }

    if (mlir_data->cellSize.allocated != nullptr) {
        delete[] static_cast<double**>(mlir_data->cellSize.allocated);
    }
}

void copybackMLIRresults(exahype2::CellData<double, double>* patchData, MLIRCellData* mlir_data) {
    int64_t totalCells = patchData->numberOfCells;
    int64_t volumes = static_cast<int64_t>({{PATCH_SIZE}});
    int64_t outputSizePerCell = volumes * volumes * volumes * ({{SOLVER_CLASS_NAMESPACE}}::{{SOLVER_CLASS}}::NumberOfUnknowns);
    for (int i = 0; i < totalCells; i++) {
        for (int j = 0; j < outputSizePerCell; j++) {
            patchData->QOut[i][j] = mlir_data->QOut.allocated[i * outputSizePerCell + j];
        }
    }
}

// Main MLIR wrapper function
void {{KERNEL_NAME}}_MLIR_wrapper(exahype2::CellData<double, double>* patchData, bool use_omp = false) {
    // Globals are already initialized from Constants.h at module load time
    // No need to set them here - they're set to the correct values from compile-time constants
    
    // Validate patchData
    if (!patchData) {
        return;
    }
    
    // Variables to track allocated memory for cleanup
    MLIRCellData mlir_data;  
    
    try {
        // Convert to MLIR struct with contiguous memory
        convertToMLIRStruct(patchData, &mlir_data);
        
        int64_t totalCells = patchData->numberOfCells;
        int64_t numUnknowns = {{SOLVER_CLASS_NAMESPACE}}::{{SOLVER_CLASS}}::NumberOfUnknowns;
        
        // Call MLIR-generated kernel
        if (use_omp) {
            {{KERNEL_NAME}}_omp(
                &mlir_data,  
                flux_bridge,
                nonconservativeProduct_bridge,
                sourceTerm_bridge,
                maxEigenvalue_bridge
            );
        } else {
            {{KERNEL_NAME}}(
                &mlir_data,  
                flux_bridge,
                nonconservativeProduct_bridge,
                sourceTerm_bridge,
                maxEigenvalue_bridge
            );
        }

        // Copy results back from contiguous QOut to original double** array
        int64_t volumes = static_cast<int64_t>({{PATCH_SIZE}});
        int64_t outputSizePerCell = volumes * volumes * volumes * numUnknowns;
        for (int i = 0; i < totalCells; i++) {
            for (int j = 0; j < outputSizePerCell; j++) {
                patchData->QOut[i][j] = mlir_data.QOut.allocated[i * outputSizePerCell + j];
            }
        }
        
    } catch (...) {
        std::cout << "WRAPPER: Exception caught in MLIR call" << std::endl;
    }
    
    cleanupMLIRStruct(&mlir_data);
}


// Namespace implementations for the missing template instantiations
namespace {{KERNEL_NAMESPACE}}::mlir {


void {{KERNEL_NAME}}Stateless(exahype2::CellData<double, double>& patchData) {
    {{KERNEL_NAME}}_MLIR_wrapper(&patchData);
}

void {{KERNEL_NAME}}Stateless(exahype2::CellData<double, double>& patchData, tarch::timing::Measurement& measurement) {
    tarch::timing::Watch watch("{{KERNEL_NAMESPACE}}::mlir", "{{KERNEL_NAME}}Stateless", false, true);
    {{KERNEL_NAME}}_MLIR_wrapper(&patchData);
    watch.stop();
    measurement.setValue(watch.getCalendarTime());
}

// Template declaration for mlir namespace
{{TEMPLATE_DEFINITION}}
void {{KERNEL_NAME}}Stateless(exahype2::CellData<double, double>& patchData, tarch::timing::Measurement& measurement);
void {{KERNEL_NAME}}Stateless(MLIRCellData* patchData, tarch::timing::Measurement& measurement);

} // namespace {{KERNEL_NAMESPACE}}::mlir

// Template specialization in mlir namespace
namespace {{KERNEL_NAMESPACE}}::mlir {
template<>
void {{KERNEL_NAME}}Stateless<{{TEMPLATE_CALL}}>(
    exahype2::CellData<double, double>& patchData,
    tarch::timing::Measurement& measurement
) {
    tarch::timing::Watch watch("{{KERNEL_NAMESPACE}}::mlir", "{{KERNEL_NAME}}Stateless", false, true);
    {{KERNEL_NAME}}_MLIR_wrapper(&patchData);
    watch.stop();
    measurement.setValue(watch.getCalendarTime());
}

template<>
void {{KERNEL_NAME}}Stateless<{{TEMPLATE_CALL}}>(
    MLIRCellData* patchData,
    tarch::timing::Measurement& measurement
) {
    tarch::timing::Watch watch("{{KERNEL_NAMESPACE}}::mlir", "{{KERNEL_NAME}}Stateless", false, true);
    {{KERNEL_NAME}}(
        patchData,  
        flux_bridge,
        nonconservativeProduct_bridge,
        sourceTerm_bridge,
        maxEigenvalue_bridge
    );
    watch.stop();
    measurement.setValue(watch.getCalendarTime());
}
} // namespace {{KERNEL_NAMESPACE}}::mlir

// Template specialization in mlir::omp namespace
namespace {{KERNEL_NAMESPACE}}::mlir::omp {
template<>
void {{KERNEL_NAME}}Stateless<{{TEMPLATE_CALL}}>(
    exahype2::CellData<double, double>& patchData,
    tarch::timing::Measurement& measurement
) {
    tarch::timing::Watch watch("{{KERNEL_NAMESPACE}}::mlir::omp", "{{KERNEL_NAME}}Stateless", false, true);
    {{KERNEL_NAME}}_MLIR_wrapper(&patchData, true);
    watch.stop();
    measurement.setValue(watch.getCalendarTime());
}

template<>
void {{KERNEL_NAME}}Stateless<{{TEMPLATE_CALL}}>(
    MLIRCellData* patchData,
    tarch::timing::Measurement& measurement
) {
    tarch::timing::Watch watch("{{KERNEL_NAMESPACE}}::mlir", "{{KERNEL_NAME}}Stateless", false, true);
    {{KERNEL_NAME}}_omp(
        patchData,  
        flux_bridge,
        nonconservativeProduct_bridge,
        sourceTerm_bridge,
        maxEigenvalue_bridge
    );
    watch.stop();
    measurement.setValue(watch.getCalendarTime());
}
} // namespace {{KERNEL_NAMESPACE}}::mlir::omp

#ifdef MLIR_GPU
namespace {{KERNEL_NAMESPACE}}::mlir::gpu {
// GPU template specialization for {{SOLVER_CLASS_NAMESPACE}}::{{SOLVER_CLASS}}
template<>
void {{KERNEL_NAME}}Stateless<{{TEMPLATE_CALL}}>(
    int targetDevice,
    MLIRCellData* patchData,
    tarch::timing::Measurement& measurement
) {
    setupMLIRContextGPU(targetDevice);

    tarch::timing::Watch watch("{{KERNEL_NAMESPACE}}::rusanov::mlir", "{{KERNEL_NAMESPACE}}Stateless", false, true);    

    {{KERNEL_NAME}}_gpu(patchData);
    
    watch.stop();
    measurement.setValue(watch.getCalendarTime());

    releaseMLIRContextGPU(targetDevice);
}


} // namespace exahype2::fv::rusanov::mlir::gpu
#endif

