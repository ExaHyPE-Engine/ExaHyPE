//===- ExahypePasses.cpp - Exahype passes -----------------*- C++ -*-===//
//
//===----------------------------------------------------------------------===//
#ifdef BUILD_MLIR_EXAHYPE

#include "mlir/Dialect/Affine/IR/AffineOps.h"
#include "mlir/Dialect/SCF/IR/SCF.h"
#include "mlir/Dialect/Func/IR/FuncOps.h"
#include "mlir/Dialect/MemRef/IR/MemRef.h" 
#include "mlir/Dialect/Arith/IR/Arith.h"
#include "mlir/Dialect/LLVMIR/LLVMDialect.h"
#include "mlir/Dialect/GPU/IR/GPUDialect.h"
#include "mlir/Dialect/OpenMP/OpenMPDialect.h"
#include "mlir/IR/PatternMatch.h"
#include "mlir/IR/Builders.h"
#include "mlir/Rewrite/FrozenRewritePatternSet.h"
#include "mlir/Support/LogicalResult.h"
#include "mlir/Transforms/GreedyPatternRewriteDriver.h"
#include "mlir/IR/IRMapping.h"
#include "mlir/IR/Dominance.h"
#include "mlir/Interfaces/SideEffectInterfaces.h"
#include "mlir/Analysis/AliasAnalysis.h"
#include "mlir/Pass/Pass.h"
#include "mlir/Support/FileUtilities.h"
#include "mlir/InitAllDialects.h"
#include "mlir/IR/Diagnostics.h"
#include "mlir/Parser/Parser.h"
#include "mlir/IR/BuiltinAttributes.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/FileSystem.h"
#include "llvm/Support/SourceMgr.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/IR/DerivedTypes.h"
#include <map>
#include <list>
#include <iostream>

#include "Exahype/ExahypePasses.h"

namespace mlir::exahype {
#define GEN_PASS_DEF_EXAHYPEMERGEMAXEIGENVALUESARRAY
#define GEN_PASS_DEF_EXAHYPEMERGEARRAYSCONSERVATIVE
#define GEN_PASS_DEF_EXAHYPELINEARIZENESTEDMEMREFS
#define GEN_PASS_DEF_EXAHYPEINCLUDEPHYSICS
#define GEN_PASS_DEF_EXAHYPEADDFASTMATH
#define GEN_PASS_DEF_EXAHYPEADDOPENMPSCHEDULE
#define GEN_PASS_DEF_EXAHYPEHOSTTODEVICEMEMSPACE
#include "Exahype/ExahypePasses.h.inc"


std::unordered_map<std::string, unsigned int> memorySpaceMap = {
    {"host", 0},
    {"global", 1},
    {"shared", 3},
    {"private", 5}
};

struct NestedAllocInfo {
  memref::AllocOp nestedAlloc;
  Value outerDim;
  Value innerDim;
  scf::ForOp allocationLoop;
  SmallVector<memref::LoadOp, 4> loads;
  SmallVector<memref::StoreOp, 4> stores;
};

void collectAllDependencies(Operation *op, SmallVector<Operation*, 32> &deps, 
                            llvm::DenseSet<Operation*> &visited) {
    if (visited.count(op)) return;
    visited.insert(op);
    
    for (auto operand : op->getOperands()) {
        if (auto *defOp = operand.getDefiningOp()) {
            collectAllDependencies(defOp, deps, visited);
        }
    }
    
    deps.push_back(op);
}


struct ExahypeMergeMaxEigenvaluesArray
    : public impl::ExahypeMergeMaxEigenvaluesArrayBase<ExahypeMergeMaxEigenvaluesArray> {
    MLIR_DEFINE_EXPLICIT_INTERNAL_INLINE_TYPE_ID(ExahypeMergeMaxEigenvaluesArray)

private:
    struct ArrayAllocationInfo {
        memref::AllocOp outerAlloc;
        memref::AllocOp innerAlloc;
        memref::DeallocOp outerDealloc;
        memref::DeallocOp innerDealloc;
        scf::ForOp innerAllocationLoop;
        SmallVector<struct ArrayAllocationInfo,32> *mergeArrays = nullptr; 
    };

    void collectArrayInfo(ModuleOp &module, ArrayAllocationInfo &info) {
            memref::AllocOp innerAlloc;
            scf::ForOp innerAllocationLoop;
        
            module.walk([&](memref::StoreOp storeOp) {
                if (storeOp.getMemref() != info.outerAlloc.getResult()) return;
                
                auto storedValue = storeOp.getValueToStore();
                if (auto innerAllocOp = storedValue.getDefiningOp<memref::AllocOp>()) {
                    innerAlloc = innerAllocOp;

                    innerAllocOp->getParentOp()->walk([&](scf::ForOp forOp) {
                        forOp->walk([&](memref::StoreOp storeOp) {
                            auto nameAttr = storeOp->getAttrOfType<StringAttr>("name");
                            // *** NOTE: We could compare the allocation return values 
                            //           but we know the name attributes are correct
                            if (nameAttr.getValue() == innerAllocOp->getAttrOfType<StringAttr>("name").getValue()) {
                                innerAllocationLoop = forOp;
                            }
                        });
                    });
                    }
                });
            
            if (!innerAlloc) return;          

            memref::DeallocOp outerDealloc;
            memref::DeallocOp innerDealloc;

            module.walk([&](memref::DeallocOp deallocOp) {
                if (deallocOp.getMemref() == info.outerAlloc.getResult()) {
                    outerDealloc = deallocOp;
                }
                
                if (deallocOp.getMemref() == innerAlloc.getResult()) {
                    innerDealloc = deallocOp;
                }
            });
            
            info.innerAlloc = innerAlloc;
            info.outerDealloc = outerDealloc;
            info.innerDealloc = innerDealloc;
            info.innerAllocationLoop = innerAllocationLoop;       
    }

    void runOnOperation() override {

        ModuleOp module = getOperation();
        func::FuncOp timeStepFunc;

        module.walk([&](func::FuncOp func) {
            if (func.getName().starts_with("timeStepWith")) {
                timeStepFunc = func;
            }
        });
        
        if (!timeStepFunc) {
            return;
        }

        // A map of the arrays we want to merge: 
        //   key - hoisted array name, 
        //   value - list of array names to merge into the hoisted array 
        std::unordered_map<std::string, std::list<std::string>> arrayMergeMap = {
            {"maxEigenvaluesX", {"maxEigenvaluesX2", "maxEigenvaluesXY"}},
            {"maxEigenvaluesY", {"maxEigenvaluesY2"}},
            {"maxEigenvaluesZ", {"maxEigenvaluesZ2",}}
        };

        SmallVector<ArrayAllocationInfo, 32> allPatterns;
        SmallVector<memref::AllocOp, 32> allOuterAllocs;

        module.walk([&](memref::AllocOp allocOp) {
            auto allocType = llvm::dyn_cast<MemRefType>(allocOp.getMemref().getType());
            if (!allocType || allocType.getRank() != 1) return;
            
            if (!llvm::isa<MemRefType>(allocType.getElementType())) return;

            // Guard for arrays we want to process
            auto nameAttr = allocOp->getAttrOfType<StringAttr>("name");

            if(!llvm::any_of(arrayMergeMap, [&](const auto &pair) {
                return nameAttr.getValue() == pair.first;
            })) {
                return;
            }

            allOuterAllocs.push_back(allocOp);
        });
        
        for (auto outerAlloc : allOuterAllocs) {
            ArrayAllocationInfo info;

            info.outerAlloc = outerAlloc;
            collectArrayInfo(module, info);

            module.walk([&](memref::AllocOp allocOp) {
                auto allocType = llvm::dyn_cast<MemRefType>(allocOp.getMemref().getType());
                if (!allocType || allocType.getRank() != 1) return;
                
                if (!llvm::isa<MemRefType>(allocType.getElementType())) return;

                auto nameAttr = allocOp->getAttrOfType<StringAttr>("name");
                auto mergeNameListIterator = arrayMergeMap.find(std::string(info.outerAlloc->getAttrOfType<StringAttr>("name").getValue()));
                auto iter = std::find (mergeNameListIterator->second.begin(), mergeNameListIterator->second.end(), nameAttr.getValue());

                if (mergeNameListIterator == arrayMergeMap.end() || iter == mergeNameListIterator->second.end()) {
                    return;
                }

                outerAlloc = allocOp;

                ArrayAllocationInfo infoMerge;
                
                infoMerge.outerAlloc = outerAlloc;
                collectArrayInfo(module, infoMerge);    

                if (!info.mergeArrays) {
                    info.mergeArrays = new SmallVector<ArrayAllocationInfo, 32>();
                }

                info.mergeArrays->push_back(infoMerge);
            });

            allPatterns.push_back(info);
        }
        

        hoistMaxEigenvaluesXAllocation(module, allPatterns);
    }

    void hoistMaxEigenvaluesXAllocation(ModuleOp module, 
                                        SmallVector<ArrayAllocationInfo, 32> &patterns) {

        // ***NOTE: We reverse the processing order due to the insertion point logic
        for (const auto &info : llvm::reverse(patterns)) {
            memref::AllocOp outerAlloc = info.outerAlloc; 
            memref::DeallocOp outerDealloc = info.outerDealloc;
            
            auto *funcOp = outerAlloc->getParentOp();

            while (funcOp && !llvm::isa<func::FuncOp>(funcOp)) {
                funcOp = funcOp->getParentOp();
            }
            
            auto timeStepFunc = llvm::cast<func::FuncOp>(funcOp);
            auto *entryBlock = &timeStepFunc.getBody().front();
            
            Operation *insertionPoint = nullptr;

            for (auto &op : entryBlock->getOperations()) {
                if (auto castOp = llvm::dyn_cast<UnrealizedConversionCastOp>(&op)) {
                    if (llvm::isa<MemRefType>(castOp.getResults()[0].getType())) {
                        insertionPoint = &op;
                    }
                } else if (llvm::isa<scf::IfOp>(&op)) {
                    break;
                }
            }
            
            if (!insertionPoint) {
                insertionPoint = &entryBlock->front();
            }
            
            IRMapping mapper;
            OpBuilder builder(entryBlock, Block::iterator(insertionPoint));

            builder.setInsertionPointAfter(insertionPoint);
            
            SmallVector<Operation*, 32> allocDeps;
            llvm::DenseSet<Operation*> visited;

            collectAllDependencies(outerAlloc, allocDeps, visited);
            
            for (auto *op : allocDeps) {
                // ***NOTE: The outer allocation is cloned in collectAllDependencies, so skip here
                if (op == outerAlloc) continue; 
                builder.insert(op->clone(mapper));
            }
            
            auto clonedAlloc = builder.insert(outerAlloc->clone(mapper));
            auto clonedAllocOp = llvm::dyn_cast<memref::AllocOp>(clonedAlloc);
            
            auto nameAttr = outerAlloc->getAttrOfType<StringAttr>("name");

            outerAlloc.getResult().replaceAllUsesWith(clonedAllocOp.getResult());

            if (info.mergeArrays) {
                for (const auto &mergeInfo : *(info.mergeArrays)) {
                    auto mergeNameAttr = mergeInfo.outerAlloc->getAttrOfType<StringAttr>("name");
                    auto mergeOuterAlloc = mergeInfo.outerAlloc;

                    // We want to still be able to trace the original merge array
                    clonedAllocOp->setAttr("name", mergeNameAttr);
                    mergeOuterAlloc.getResult().replaceAllUsesWith(clonedAllocOp.getResult());
                    // We need to reset to the original hoisted array name 
                    clonedAllocOp->setAttr("name", nameAttr);

                    mergeOuterAlloc.erase();
                    mergeInfo.innerAllocationLoop->erase();
                }
            }
            
            // Hoist inner allocation loop and its dependencies 
            SmallVector<Operation*, 32> loopDeps;
            scf::ForOp innerAllocLoop = info.innerAllocationLoop;

            collectAllDependencies(innerAllocLoop, loopDeps, visited);

            for (auto *op: loopDeps) {
                builder.insert(op->clone(mapper));
            }

            // Clean up old ops
            if (outerAlloc) {
                outerAlloc->erase();
            }

            if (innerAllocLoop) {
                innerAllocLoop->erase();
            }
            
            if (outerDealloc) {
                outerDealloc->erase();
            }

            if (info.innerDealloc) {
                info.innerDealloc->erase();
            }
            
            auto *lastOp = entryBlock->getTerminator();

            if (lastOp && llvm::isa<func::ReturnOp>(lastOp)) {
                OpBuilder exitBuilder(lastOp);
                exitBuilder.create<memref::DeallocOp>(lastOp->getLoc(), clonedAllocOp.getResult());
            }
        }

    }
};

struct ExahypeLinearizeNestedMemrefsPass
    : public impl::ExahypeLinearizeNestedMemrefsBase<ExahypeLinearizeNestedMemrefsPass> {

private:
    DenseMap<Value, NestedAllocInfo> analyzeNestedAllocations(func::FuncOp funcOp) {
        DenseMap<Value, NestedAllocInfo> allocations;
        
        funcOp.walk([&](memref::AllocOp allocOp) {

            if (!allocOp.getNumOperands()) return;

            auto memrefType = dyn_cast<MemRefType>(allocOp.getType());

            if (!memrefType) return;
            
            // Check if this is a nested memref (memref<?xmemref<?xf64>>)
            if (!dyn_cast<MemRefType>(memrefType.getElementType())) return;  

            NestedAllocInfo info;
            info.nestedAlloc = allocOp;
            info.outerDim = allocOp.getOperand(0);  
            
            Value allocValue = allocOp.getResult();
            Value innerDimValue = nullptr;
            
            // Let's see if we can get dimensions from the using ops first...
            for (auto* userOp : allocValue.getUsers()) {
                if (auto storeOp = dyn_cast<memref::StoreOp>(userOp)) {
                    Value storedValue = storeOp.getValue();
                    auto storedType = dyn_cast<MemRefType>(storedValue.getType());

                    if (!storedType || storedType.getRank() != 1) continue;
                    
                    if (auto innerAlloc = storedValue.getDefiningOp<memref::AllocOp>()) {
                        if (innerAlloc.getNumOperands() > 0) {
                            innerDimValue = innerAlloc.getOperand(0);
                            break;
                        }
                    }
                }
            }
            
            // No dice - walk the parent
            if (!innerDimValue) {
                SmallVector<Value> candidateInnerDims;
                
                // Look through all stores to the outer nested array
                for (auto* userOp : allocValue.getUsers()) {
                    if (auto storeOp = dyn_cast<memref::StoreOp>(userOp)) {
                        Value storedValue = storeOp.getValueToStore();
                        
                        if (auto innerAlloc = storedValue.getDefiningOp<memref::AllocOp>()) {
                            auto allocType = dyn_cast<MemRefType>(innerAlloc.getType());
                            if (allocType && allocType.getRank() == 1) {
                                // Verify this is truly a 1D allocation (not nested)
                                if (!dyn_cast<MemRefType>(allocType.getElementType())) {
                                    if (innerAlloc.getNumOperands() > 0) {
                                        candidateInnerDims.push_back(innerAlloc.getOperand(0));
                                    }
                                }
                            }
                        }
                    }
                }
                
                // Use the collected dimension if available
                if (!candidateInnerDims.empty()) {
                    innerDimValue = candidateInnerDims[0];  // All should be equivalent
                }
            }
            
            info.innerDim = innerDimValue;  
            
            for (auto* userOp : allocValue.getUsers()) {
                if (auto loadOp = dyn_cast<memref::LoadOp>(userOp)) {
                    info.loads.push_back(loadOp);
                } else if (auto storeOp = dyn_cast<memref::StoreOp>(userOp)) {
                    info.stores.push_back(storeOp);
                }
            }
            
            for (auto* userOp : allocValue.getUsers()) {
                if (auto storeOp = dyn_cast<memref::StoreOp>(userOp)) {
                    auto parentLoop = storeOp->getParentOfType<scf::ForOp>();

                    if (parentLoop) {
                        info.allocationLoop = parentLoop;
                        break;
                    }
                }
            }
            
            allocations[allocValue] = info;
        });

        return allocations;
    }


    Value cloneWithDeps(OpBuilder &builder, Value val) {
        IRMapping irMap;

        if (!val.getDefiningOp()) {
            return val;
        }
        
        Operation *defOp = val.getDefiningOp();
        
        // Recursively clone operands
        SmallVector<Value> newOperands;
        for (auto operand : defOp->getOperands()) {
            newOperands.push_back(cloneWithDeps(builder, operand));
        }
        
        Operation *clonedOp = defOp->cloneWithoutRegions();
        for (size_t i = 0; i < newOperands.size(); ++i) {
            clonedOp->setOperand(i, newOperands[i]);
        }
        
        builder.insert(clonedOp);
        
        Value clonedVal = clonedOp->getResult(0);

        irMap.map(val, clonedVal);
        return clonedVal;
    };
    
public:
    // *** NOTE: This pass *only* linearizes a single-nested memref i.e. 'memref<?xmemref<?xT>>'
    //           to a linear 2D memref i.e. 'memref<?x?xT>'
    //
    // *** TODO: This is way too big and unweildy - refactor
    //
    void runOnOperation() override {
        // Transform nested memref<?xmemref<?xT>> to flat memref<?x?xT>
        ModuleOp module = getOperation();
        
        module.walk([&](func::FuncOp funcOp) {
            auto allocations = analyzeNestedAllocations(funcOp);

            if (allocations.empty()) return;

            OpBuilder builder(funcOp.getContext());
            SmallVector<Operation*, 16> opsToErase;
            DenseMap<Value, Value> nestedTo2D;  
            
            for (auto &[allocValue, info] : allocations) {
                auto oldAlloc = info.nestedAlloc;
                auto oldType = dyn_cast<MemRefType>(oldAlloc.getType());

                if (!oldType) continue;
                
                // Check inner type is a memref i.e. nested
                if (!dyn_cast<MemRefType>(oldType.getElementType())) continue;
                
                // Extract or hoist inner dimension
                Value hoistedInnerDim = nullptr;
                
                if (info.innerDim && info.innerDim.getDefiningOp()) {
                    Operation *dimDefOp = info.innerDim.getDefiningOp();
                    
                    // *** NOTE: The operations we clone are restricted to the list implemented here
                    std::function<bool(Operation*)> isCloneable = [&](Operation *op) -> bool {
                        auto dialect = op->getDialect();
                        
                        if (!dialect) return false;
                        
                        auto dialectName = dialect->getNamespace();
                        
                        if (dialectName == "arith" || dialectName == "index") {
                            return true;
                        }

                        if (dialectName == "llvm") {
                            SmallVector<StringRef> llvmOps = {
                                "llvm.load", "llvm.extractvalue", "llvm.bitcast",
                                "llvm.inttoptr", "llvm.ptrtoint", "llvm.mlir.addressof"
                            };
                            auto opName = op->getName().getStringRef();

                            if (llvm::is_contained(llvmOps, opName)) {
                                return true;
                            }
                        }
                        return false;
                    };
                    
                    // Check dependencies
                    bool allCloneable = true;
                    SmallVector<Operation*> toVisit;
                    std::set<Operation*> visited;
                    toVisit.push_back(dimDefOp);
                    
                    while (!toVisit.empty() && allCloneable) {
                        Operation *op = toVisit.pop_back_val();
                        
                        if (visited.count(op)) continue;
                        visited.insert(op);
                        
                        if (!isCloneable(op)) {
                            allCloneable = false;
                            break;
                        }
                        
                        for (auto operand : op->getOperands()) {
                            if (auto defOp = operand.getDefiningOp()) {
                                if (!visited.count(defOp)) {
                                    toVisit.push_back(defOp);
                                }
                            }
                        }
                    }
                    
                    if (allCloneable) {
                        builder.setInsertionPoint(oldAlloc);
                        hoistedInnerDim = cloneWithDeps(builder, info.innerDim);
                    }
                } else if (info.innerDim) {
                    hoistedInnerDim = info.innerDim;
                }
                
                if (!hoistedInnerDim) continue;
                
                auto newAllocType = MemRefType::get(
                    {ShapedType::kDynamic, ShapedType::kDynamic},
                    // Get the inner element type
                    dyn_cast<MemRefType>(oldType.getElementType()).getElementType(),
                    MemRefLayoutAttrInterface{}, // *** NOTE: default - should fix assertion error
                    oldType.getMemorySpace()); 
                
                builder.setInsertionPoint(oldAlloc);

                SmallVector<Value> dims = {info.outerDim, hoistedInnerDim};
                Operation *newAllocOp = builder.create<memref::AllocOp>(
                    oldAlloc.getLoc(), 
                    newAllocType, 
                    ValueRange(dims)
                );
                
                // Preserve name attribute from original allocation
                if (auto nameAttr = oldAlloc->getAttrOfType<StringAttr>("name")) {
                    newAllocOp->setAttr("name", nameAttr);
                }
                
                Value newAllocResult = newAllocOp->getResult(0);
                nestedTo2D[oldAlloc.getResult()] = newAllocResult;
                
                SmallVector<memref::LoadOp> nestedLoads;
                SmallVector<memref::StoreOp> nestedStores;
                SmallVector<memref::DeallocOp> outerDeallocOps;
                
                funcOp.walk([&](Operation *op) {
                    if (auto load = dyn_cast<memref::LoadOp>(op)) {
                        if (load.getMemref() == oldAlloc.getResult()) {
                            nestedLoads.push_back(load);
                        }
                    }
                    if (auto storeOp = dyn_cast<memref::StoreOp>(op)) {
                        if (storeOp.getMemref() == oldAlloc.getResult()) {
                            nestedStores.push_back(storeOp);
                        }
                    }
                    if (auto deallocOp = dyn_cast<memref::DeallocOp>(op)) {
                        if (deallocOp.getMemref() == oldAlloc.getResult()) {
                            outerDeallocOps.push_back(deallocOp);
                        }
                    }
                });
                
                for (auto storeOp : nestedStores) {
                    opsToErase.push_back(storeOp.getOperation());
                }
                
                // Collect inner allocations that feed into stores to the outer nested memref
                // These must be erased before the loop can be erased
                SmallVector<memref::AllocOp> innerAllocs;
                for (auto storeOp : nestedStores) {
                    Value storedValue = storeOp.getValueToStore();
                    if (auto allocOp = storedValue.getDefiningOp<memref::AllocOp>()) {
                        innerAllocs.push_back(allocOp);
                    }
                }
                
                for (auto innerAlloc : innerAllocs) {
                    opsToErase.push_back(innerAlloc);
                }
                
                for (auto loadOp : nestedLoads) {
                    SmallVector<Operation*> usesOps;
                    for (auto &use : loadOp.getResult().getUses()) {
                        usesOps.push_back(use.getOwner());
                    }
                    
                    for (auto usesOp : usesOps) {
                        if (auto innerLoadOp = dyn_cast<memref::LoadOp>(usesOp)) {
                            builder.setInsertionPoint(innerLoadOp);
                            
                            SmallVector<Value> newIndices;

                            for (auto idx : loadOp.getIndices()) {
                                newIndices.push_back(idx);
                            }

                            for (auto idx : innerLoadOp.getIndices()) {
                                newIndices.push_back(idx);
                            }
                            
                            auto newLoadOp = builder.create<memref::LoadOp>(
                                innerLoadOp.getLoc(),
                                newAllocResult,
                                newIndices);
                            
                            // *** NOTE: Preserve name string attribute from inner load
                            if (auto nameAttr = innerLoadOp->getAttrOfType<StringAttr>("name")) {
                                newLoadOp->setAttr("name", nameAttr);
                            }
                            
                            innerLoadOp.replaceAllUsesWith(newLoadOp.getResult());
                            opsToErase.push_back(innerLoadOp.getOperation());

                            continue;
                        }  
                        
                        if (auto storeOp = dyn_cast<memref::StoreOp>(usesOp)) {
                            builder.setInsertionPoint(storeOp);
                            
                            SmallVector<Value> newIndices;

                            for (auto idx : loadOp.getIndices()) {
                                newIndices.push_back(idx);
                            }

                            for (auto idx : storeOp.getIndices()) {
                                newIndices.push_back(idx);
                            }
                            
                            auto newStoreOp = builder.create<memref::StoreOp>(
                                storeOp.getLoc(),
                                storeOp.getValueToStore(),
                                newAllocResult,
                                newIndices);
                            
                            // *** NOTE: Preserve name string attribute from inner store
                            if (auto nameAttr = storeOp->getAttrOfType<StringAttr>("name")) {
                                newStoreOp->setAttr("name", nameAttr);
                            }
                            
                            opsToErase.push_back(storeOp.getOperation());

                            continue;
                        } 
                        
                        if (auto dimOp = dyn_cast<memref::DimOp>(usesOp)) {
                            dimOp.replaceAllUsesWith(hoistedInnerDim);
                            opsToErase.push_back(dimOp.getOperation());

                            continue;
                        }  
                        
                        if (auto deallocOp = dyn_cast<memref::DeallocOp>(usesOp)) {
                            opsToErase.push_back(deallocOp.getOperation());

                            continue;
                        } 
                        
                        // *** NOTE: We need additional handling when we extract the pointer for a function call
                        //           Here we use pointer arithmetic on the linearized array
                        if (auto extractPtr = dyn_cast<memref::ExtractAlignedPointerAsIndexOp>(usesOp)) {
                            builder.setInsertionPoint(extractPtr);

                            auto basePtr = builder.create<memref::ExtractAlignedPointerAsIndexOp>(
                                extractPtr.getLoc(), newAllocResult);

                            auto rowElemOffset = builder.create<arith::MulIOp>(
                                extractPtr.getLoc(), loadOp.getIndices()[0], hoistedInnerDim);

                            // We need to calculate the element size in bytes
                            auto dl = DataLayout::closest(extractPtr.getOperation());
                            auto elemType = cast<MemRefType>(newAllocResult.getType()).getElementType();
                            auto elemSizeBytes = builder.create<arith::ConstantIndexOp>(
                                extractPtr.getLoc(), dl.getTypeSize(elemType));

                            auto rowByteOffset = builder.create<arith::MulIOp>(
                                extractPtr.getLoc(), rowElemOffset, elemSizeBytes);
                            auto rowPtr = builder.create<arith::AddIOp>(
                                extractPtr.getLoc(), basePtr, rowByteOffset);

                            extractPtr.replaceAllUsesWith(rowPtr.getResult());
                            opsToErase.push_back(extractPtr.getOperation());

                            // *** NOTE: This skips the SubViewOp handling below
                            continue;
                        }

                        builder.setInsertionPoint(loadOp);
                        
                        auto constIndexOp0 = builder.create<arith::ConstantIndexOp>(loadOp.getLoc(), 0);
                        auto constIndexOp1 = builder.create<arith::ConstantIndexOp>(loadOp.getLoc(), 1);
                        
                        auto subviewOp = builder.create<memref::SubViewOp>(
                            loadOp.getLoc(),
                            newAllocResult,
                            SmallVector<Value>{loadOp.getIndices()[0], constIndexOp0},
                            SmallVector<Value>{constIndexOp1, hoistedInnerDim},
                            SmallVector<Value>{constIndexOp1, constIndexOp1}
                        );
                        
                        // *** NOTE: Preserve name string attribute from load
                        if (auto nameAttr = loadOp->getAttrOfType<StringAttr>("name")) {
                            subviewOp->setAttr("name", nameAttr);
                        }
                        
                        loadOp.getResult().replaceAllUsesWith(subviewOp.getResult());
                    
                    }
                    
                    opsToErase.push_back(loadOp.getOperation());
                }
                
                for (auto deallocOp : outerDeallocOps) {
                    builder.setInsertionPoint(deallocOp);

                    auto newDealloc = builder.create<memref::DeallocOp>(
                        deallocOp.getLoc(),
                        newAllocResult
                    );
                    
                    // *** NOTE: Preserve name string attribute from original deallocation
                    if (auto nameAttr = deallocOp->getAttrOfType<StringAttr>("name")) {
                        newDealloc->setAttr("name", nameAttr);
                    }
                    
                    opsToErase.push_back(deallocOp.getOperation());
                }
                
                opsToErase.push_back(oldAlloc.getOperation());
                
                // Erase the inner allocation loop if it exists
                if (info.allocationLoop) {
                    opsToErase.push_back(info.allocationLoop);
                }
            }
            
            for (auto *op : opsToErase) {
                op->erase();
            }
        });

    }
};


struct ExahypeIncludePhysics : public impl::ExahypeIncludePhysicsBase<ExahypeIncludePhysics> {
    using Base = impl::ExahypeIncludePhysicsBase<ExahypeIncludePhysics>;

private:
    int memorySpace = 0;
    
public:
    ExahypeIncludePhysics() = default;
    explicit ExahypeIncludePhysics(const ExahypeIncludePhysicsOptions &options)
        : Base(options) {}
    
    void runOnOperation() override {
        ModuleOp module = getOperation();
        
        if (physicsFile.empty()) {
            emitError(module.getLoc(), "physics-file option is required");
            return;
        }

        if (!memSpace.empty()) {
            memorySpace = memorySpaceMap[memSpace];
        }

        // Read physics file as string
        auto fileRef = llvm::MemoryBuffer::getFile(physicsFile);
        if (!fileRef) {
            emitError(module.getLoc()) << "Failed to read physics file: " << physicsFile;
            return;
        }

        auto source = fileRef.get()->getBuffer().str();
        auto buffer = llvm::MemoryBuffer::getMemBuffer(source, "physics.mlir");
        
        llvm::SourceMgr sourceMgr;
        sourceMgr.AddNewSourceBuffer(std::move(buffer), llvm::SMLoc());
        
        auto parseResult = parseSourceFile<ModuleOp>(sourceMgr, module.getContext());
        if (!parseResult) {
            emitError(module.getLoc()) << "Failed to parse physics file: " << physicsFile;
            return;
        }

        ModuleOp physicsModule = llvm::dyn_cast_or_null<ModuleOp>((Operation*)(parseResult.release()));
        if (!physicsModule) {
            emitError(module.getLoc(), "Physics file did not parse to a valid module");
            physicsModule->erase();
            return;
        }
        
        mergePhysicsFunctions(module, physicsModule);

        // *** NOTE: Only update the kernel function signature if we are passed 
        //           'simplify-kernel-signature' option
        if (simplifyKernelSig) {
            simplifyKernelSignature(module);
        }

        physicsModule.erase();
    }

private:
    void mergePhysicsFunctions(ModuleOp kernelModule, ModuleOp physicsModule) {
        // *** TODO: This uses multiple walks for simplicty of development
        //           it might be better to use a RewriterPattern
        SmallVector<Operation*> physicsFuncsOps;  // Generic operations
        SmallVector<StringRef> physicsFuncNames;
        
        physicsModule.walk([&](func::FuncOp funcOp) {
            // Only include functions with actual implementations 
            // Skip extern declarations 
            if (!funcOp.getBody().empty()) {
                physicsFuncsOps.push_back(funcOp.getOperation());
                physicsFuncNames.push_back(funcOp.getName());
            }
        });
        
        physicsModule.walk([&](LLVM::LLVMFuncOp llvmFuncOp) {
            physicsFuncsOps.push_back(llvmFuncOp.getOperation());
            physicsFuncNames.push_back(llvmFuncOp.getName());
        });
        
        if (physicsFuncsOps.empty()) {
            emitError(kernelModule.getLoc(), "No physics function implementations found in physics module");
            return;
        }
        
        SmallVector<Operation*> funcsToRemove;
        
        kernelModule.walk([&](func::FuncOp funcOp) {
            if (llvm::is_contained(physicsFuncNames, funcOp.getName())) {
                funcsToRemove.push_back(funcOp.getOperation());
            }
        });
        
        kernelModule.walk([&](LLVM::LLVMFuncOp llvmFuncOp) {
            if (llvm::is_contained(physicsFuncNames, llvmFuncOp.getName())) {
                funcsToRemove.push_back(llvmFuncOp.getOperation());
            }
        });
        
        for (auto op : funcsToRemove) {
            op->erase();
        }
        
        // Clone physics function implementations into kernel module
        OpBuilder builder(kernelModule.getContext());
        builder.setInsertionPointToEnd(kernelModule.getBody());
        
        IRMapping mapper;
        int funcCount = 0;
        
        for (auto physicsOp : physicsFuncsOps) {
            StringRef opName;

            if (auto funcOp = llvm::dyn_cast<func::FuncOp>(physicsOp)) {
                opName = funcOp.getName();
            } else if (auto llvmFuncOp = llvm::dyn_cast<LLVM::LLVMFuncOp>(physicsOp)) {
                opName = llvmFuncOp.getName();
            }
            
            Operation* clonedOp = builder.clone(*physicsOp, mapper);
            
            if (auto clonedFunc = llvm::dyn_cast<func::FuncOp>(clonedOp)) {
                transformPhysicsFunctionSignature(clonedFunc);
            }
            
            funcCount++;
        }
    }
    
    void transformPhysicsFunctionSignature(func::FuncOp func) {
        // *** NOTE: Even if we don't need the transformation of 'memref<?xi8>' to '!llvm.ptr'
        //           we still need to transform the memspace of pointers and memrefs if it is
        //           passed as an argument to the pass.
        //
        FunctionType origType = func.getFunctionType();
        SmallVector<Type> newArgTypes;
        auto newPtrType = LLVM::LLVMPointerType::get(func.getContext(), memorySpace);
        
        // Convert all argument types: memref<?xi8> to !llvm.ptr
        for (unsigned i = 0; i < origType.getNumInputs(); ++i) {
            Type argType = origType.getInput(i);
            
            // Check if this is a 'memref<?xi8>' - default generated by Polygeist
            // *** TODO: If we use 'polygeist-opt --convert-polygeist-to-llvm'
            //           we don't need to have the following transform
            if (auto memrefType = dyn_cast<MemRefType>(argType)) {
                if (memrefType.getRank() == 1) {
                    auto elementType = memrefType.getElementType();

                    if (elementType.isInteger(8)) {
                        newArgTypes.push_back(newPtrType);

                        continue;
                    }
                }

                // *** NOTE: Convert to GPU memory space if memorySpace is set
                if (memorySpace) {
                    auto newMemSpaceAttr = IntegerAttr::get(
                        IntegerType::get(func.getContext(), 64),
                        memorySpace);

                    auto newMemrefType = MemRefType::get(
                        memrefType.getShape(),
                        memrefType.getElementType(),
                        memrefType.getLayout(),
                        newMemSpaceAttr);

                    newArgTypes.push_back(newMemrefType);

                    continue;
                }
            }

            if (auto pointerType = dyn_cast<LLVM::LLVMPointerType>(argType)) {
                if (memorySpace) {
                    newArgTypes.push_back(newPtrType);

                    continue;
                }
            }

            // Leave other types alone
            newArgTypes.push_back(argType);
        }
        
        // *** TODO: We're assuming the physics functions are `void` here
        SmallVector<Type> newReturnTypes; 
        
        auto newFuncType = FunctionType::get(func.getContext(), newArgTypes, newReturnTypes);
        func.setFunctionType(newFuncType);
        
        if (!func.getBody().empty()) {
            Block &entryBlock = func.getBody().front();

            for (unsigned i = 0; i < entryBlock.getArguments().size() && i < newArgTypes.size(); ++i) {
                if (i < newArgTypes.size()) {
                    entryBlock.getArgument(i).setType(newArgTypes[i]);
                }
            }
        }
        
        func.setPrivate();
    }

    // Remove the function pointer parameters for GPU
    // *** NOTE: This transformation is guarded by 'timeStepWith' in name
    void simplifyKernelSignature(ModuleOp module) {

        func::FuncOp kernelFunc;
        module.walk([&](func::FuncOp func) {
            std::string name = func.getName().str();
            if (name.find("timeStepWith") != std::string::npos) {
                kernelFunc = func;
            }
        });
        
        if (!kernelFunc) {
            emitError(module.getLoc(), "No kernel function named timeStepWith* found");
            return;
        }
        
        
        auto funcType = kernelFunc.getFunctionType();
        SmallVector<unsigned> closureParamIndices;
        
        for (unsigned i = 0; i < funcType.getNumInputs(); i++) {
            Type argType = funcType.getInput(i);

            // Closure parameters are function types
            if (isa<FunctionType>(argType)) {
                closureParamIndices.push_back(i);
            }
        }
        
        if (closureParamIndices.empty()) {
            emitError(module.getLoc(), "No closure parameters found");
            return;
        }
        
        Block &funcBlock = kernelFunc.getBody().front();
        
        std::sort(closureParamIndices.rbegin(), closureParamIndices.rend());
        for (unsigned idx : closureParamIndices) {
            funcBlock.eraseArgument(idx);
        }
        
        SmallVector<Type> newInputTypes;
        for (unsigned i = 0; i < funcType.getNumInputs(); i++) {
            if (!llvm::is_contained(closureParamIndices, i)) {
                newInputTypes.push_back(funcType.getInput(i));
            }
        }
        
        auto newFuncType = FunctionType::get(
            kernelFunc.getContext(),
            newInputTypes,
            funcType.getResults()
        );
        
        kernelFunc.setType(newFuncType);
    }
};

struct ExahypeAddFastmathPass
    : public impl::ExahypeAddFastmathBase<ExahypeAddFastmathPass> {
    
    // Add fastmath<fast> attribute to all arith floating-point operations
    void runOnOperation() override {

        ModuleOp module = getOperation();

        module.walk([](Operation *op) {
            SmallVector<StringRef> arithOps = {
                "arith.addf", "arith.subf", "arith.mulf", "arith.divf",
                "arith.remf", "arith.negf", "arith.truncf", 
                "arith.convertf", "arith.extf", "arith.flush_denormals",
                "arith.maximumf", "arith.maxnumf", "arith.minimumf", "arith.minnumf",
                "arith.scaling_extf", "arith.scaling_truncf"
            };

            auto opName = op->getName().getStringRef();

            if (llvm::is_contained(arithOps, opName)) {
                op->setAttr("fastmath", 
                    arith::FastMathFlagsAttr::get(
                        op->getContext(),
                        arith::FastMathFlags::fast
                    )
                );
            }
        });
    }
};

struct ExahypeAddOpenMPSchedule
    : public impl::ExahypeAddOpenMPScheduleBase<ExahypeAddOpenMPSchedule> {
    
    // Add a schedule to `omp.wsloop` ops
    void runOnOperation() override {
        ModuleOp module = getOperation();

        module.walk([&](mlir::omp::WsloopOp loopOp) {
            auto *ctx = loopOp.getContext();

            if (loopOp.getScheduleKind())
                return;

            loopOp.setScheduleKindAttr(
                mlir::omp::ClauseScheduleKindAttr::get(
                    ctx, 
                    mlir::omp::ClauseScheduleKind::Static
                )
            );

            loopOp.setScheduleSimdAttr(mlir::UnitAttr::get(ctx));
        });
    }
};

struct ExahypeHostToDeviceMemspace
    : public impl::ExahypeHostToDeviceMemspaceBase<ExahypeHostToDeviceMemspace> {

public:
    ExahypeHostToDeviceMemspace() = default;
    explicit ExahypeHostToDeviceMemspace(const ExahypeHostToDeviceMemspaceOptions &options)
        : Base(options) {}

    void getDependentDialects(DialectRegistry &registry) const override {
        registry.insert<gpu::GPUDialect,
                        memref::MemRefDialect,
                        LLVM::LLVMDialect>();
    }

    // Convert host space memrefs and pointers to device space
    // *** TODO: We could take two parameters in the pass
    //           and map from one to the other e.g. global to private,
    //           global to host etc. as well as host to global (device)
    void runOnOperation() override {

        ModuleOp module = getOperation();

        if (memSpace.empty()) return;

        // Target memory space pointer used in walks below
        auto newPtrType = LLVM::LLVMPointerType::get(
            module.getContext(),
            memorySpaceMap[memSpace]
        );

        // Target memory space attribute used in walks below
        auto newMemSpaceAttr = IntegerAttr::get(
            IntegerType::get(module.getContext(), 64),
            memorySpaceMap[memSpace]);


        module.walk([&](LLVM::GlobalOp globalOp) {
            OpBuilder builder(globalOp);
            globalOp->setAttr("addr_space", builder.getI32IntegerAttr(memorySpaceMap[memSpace]));
        });

        module.walk([&](LLVM::AddressOfOp addressOfOp) {
            addressOfOp.getResult().setType(
                LLVM::LLVMPointerType::get(addressOfOp.getContext(), memorySpaceMap[memSpace])
            );
        });

        module.walk([&](UnrealizedConversionCastOp castOp) {
            auto srcStructType = dyn_cast<LLVM::LLVMStructType>(castOp.getOperand(0).getType());
            auto resultMemrefType = dyn_cast<MemRefType>(castOp.getResult(0).getType());

            if (!srcStructType || !resultMemrefType) return;
            
            if (resultMemrefType.getMemorySpaceAsInt() == memorySpaceMap[memSpace]) return;

            OpBuilder builder(castOp);
            SmallVector<Type> newFields;
            Value sourceVal = castOp.getOperand(0);
            SmallVector<LLVM::InsertValueOp> insertChain;
            Value cur = sourceVal;

            for (Type bodyType : srcStructType.getBody()) {
                if (auto ptrType = dyn_cast<LLVM::LLVMPointerType>(bodyType)) {
                    if (ptrType.getAddressSpace() != memorySpaceMap[memSpace]) {
                        newFields.push_back(newPtrType);

                        continue;
                    }
                }
                // Leave the type alone
                newFields.push_back(bodyType);
            }

            while (auto insertOp = cur.getDefiningOp<LLVM::InsertValueOp>()) {
                insertChain.push_back(insertOp);
                cur = insertOp.getContainer();
            }
            
            // *** NOTE: Reverse so we process from undef outward
            std::reverse(insertChain.begin(), insertChain.end());
            
            if (!insertChain.empty()) {
                auto newStructType = LLVM::LLVMStructType::getLiteral(
                    castOp.getContext(), 
                    newFields, 
                    srcStructType.isPacked()
                );
                
                builder.setInsertionPoint(insertChain.front());

                Value newStruct = builder.create<LLVM::UndefOp>(castOp.getLoc(), newStructType);

                for (auto insertOp : insertChain) {
                    Value val = insertOp.getValue();

                    if (auto ptrType = dyn_cast<LLVM::LLVMPointerType>(val.getType())) {
                        if (ptrType.getAddressSpace() != memorySpaceMap[memSpace]) {
                            val = builder.create<LLVM::AddrSpaceCastOp>(
                                insertOp.getLoc(), 
                                newPtrType, 
                                val
                            );
                        }
                    }

                    newStruct = builder.create<LLVM::InsertValueOp>(
                        insertOp.getLoc(), newStructType,
                        newStruct, val,
                        insertOp.getPosition());
                }
                
                sourceVal = newStruct;
            }

            auto newMemrefType = MemRefType::get(
                resultMemrefType.getShape(),
                resultMemrefType.getElementType(),
                resultMemrefType.getLayout(),
                newMemSpaceAttr);

            builder.setInsertionPoint(castOp);

            auto newCast = builder.create<UnrealizedConversionCastOp>(
                castOp.getLoc(),
                TypeRange{newMemrefType},
                ValueRange{sourceVal});

            castOp.replaceAllUsesWith(newCast->getResults());
            castOp.erase();
        });

        module.walk([&](memref::AllocOp allocOp) {
            OpBuilder builder(allocOp);
            auto oldType = dyn_cast<MemRefType>(allocOp.getType());

            if (!oldType) return;

            if (oldType.getMemorySpaceAsInt() == memorySpaceMap[memSpace]) return;

            Type elementType = oldType.getElementType();
            if (auto nestedMemref = dyn_cast<MemRefType>(elementType)) {
                elementType = MemRefType::get(
                    nestedMemref.getShape(),
                    nestedMemref.getElementType(),
                    nestedMemref.getLayout(),
                    newMemSpaceAttr);  
            }

            auto newType = MemRefType::get(
                oldType.getShape(),
                elementType,  
                oldType.getLayout(),
                newMemSpaceAttr);

            auto newAlloc = builder.create<gpu::AllocOp>(
                allocOp.getLoc(),
                newType,                           
                Type{},             
                ValueRange{}, 
                allocOp.getDynamicSizes(),         
                ValueRange{} 
            );

            allocOp.replaceAllUsesWith(newAlloc.getMemref());
            allocOp.erase();
        });

        module.walk([&](LLVM::LoadOp loadOp) {
            auto operand = loadOp.getOperand();

            if (isa<LLVM::LLVMPointerType>(operand.getType())) {
                operand.setType(newPtrType);
            }

            if( auto memRefType = dyn_cast<MemRefType>(operand.getType())) {
                    auto newMemrefType = MemRefType::get( 
                    memRefType.getShape(),
                    memRefType.getElementType(),
                    memRefType.getLayout(),
                    newMemSpaceAttr
                );
                operand.setType(newMemrefType);
            }
        });

        module.walk([&](LLVM::StoreOp storeOp) {
            for (auto operand : storeOp.getOperands()) {
                if (isa<LLVM::LLVMPointerType>(operand.getType())) {
                    operand.setType(newPtrType);
                }

                if( auto memRefType = dyn_cast<MemRefType>(operand.getType())) {
                        auto newMemrefType = MemRefType::get( 
                        memRefType.getShape(),
                        memRefType.getElementType(),
                        memRefType.getLayout(),
                        newMemSpaceAttr
                    );
                    operand.setType(newMemrefType);
                }
            }
         });

         module.walk([&](memref::SubViewOp subviewOp) {
            auto sourceType = cast<MemRefType>(subviewOp.getSource().getType());
            auto resultType = cast<MemRefType>(subviewOp.getResult().getType());

            if (sourceType.getMemorySpaceAsInt() != memorySpaceMap[memSpace]) {
                auto newSourceViewType = MemRefType::get(
                    sourceType.getShape(),
                    sourceType.getElementType(),
                    sourceType.getLayout(),
                    newMemSpaceAttr
                );

                subviewOp.getSource().setType(newSourceViewType);
            }

            if (resultType.getMemorySpaceAsInt() != memorySpaceMap[memSpace]) {
                auto newResultType = MemRefType::get(
                    resultType.getShape(),
                    resultType.getElementType(),
                    resultType.getLayout(),   
                    newMemSpaceAttr);

                subviewOp.getResult().setType(newResultType);
            }
        });

        module.walk([&](memref::CollapseShapeOp op) {
            auto srcType = dyn_cast<MemRefType>(op.getSrc().getType());
            auto dstType = dyn_cast<MemRefType>(op.getType());

            if (!srcType || !dstType) return;

            if (srcType.getMemorySpace() == dstType.getMemorySpace()) return;

            op.getResult().setType(MemRefType::get(
                dstType.getShape(),
                dstType.getElementType(),
                dstType.getLayout(),
                srcType.getMemorySpace()));
        });

        module.walk([&](memref::CastOp castOp) {
            auto srcType = dyn_cast<MemRefType>(castOp.getSource().getType());
            auto dstType = dyn_cast<MemRefType>(castOp.getType());

            if (!srcType || !dstType) return;

            if (srcType.getMemorySpace() == dstType.getMemorySpace()) return;

            castOp.getResult().setType(MemRefType::get(
                dstType.getShape(),
                dstType.getElementType(),
                dstType.getLayout(),
                srcType.getMemorySpace()));
        });

        module.walk([&](func::FuncOp funcOp) {
            auto funcType = funcOp.getFunctionType();
            SmallVector<Type> newInputs(funcType.getInputs().begin(), funcType.getInputs().end());
            SmallVector<Type> newOutputs(funcType.getResults().begin(), funcType.getResults().end());

            // *** NOTE: If we find a 'timeStepWith*` function name, we have a kernel
            //           and we need to rename it to prevent a clash with the serial kernel name
            //           The check should make sure we only append '_gpu' once ;)
            if (funcOp.getName().starts_with("timeStepWith") && !funcOp.getName().ends_with("_gpu")) {
                std::string funcName = funcOp.getName().str();
                funcName += "_gpu";

                funcOp.setName(StringRef(funcName));
            }

            for (unsigned i = 0; i < newInputs.size(); ++i) {
                if (auto ptrType = dyn_cast<LLVM::LLVMPointerType>(newInputs[i])) {
                    if (ptrType.getAddressSpace() != memorySpaceMap[memSpace]) {
                        newInputs[i] = newPtrType;
                    }
                }

                if (auto memrefType = dyn_cast<MemRefType>(newInputs[i])) {
                    if (memrefType.getMemorySpaceAsInt() != memorySpaceMap[memSpace]) {
                        auto newMemrefType = MemRefType::get(
                            memrefType.getShape(),
                            memrefType.getElementType(),
                            memrefType.getLayout(),
                            newMemSpaceAttr);  

                        newInputs[i] = newMemrefType;
                    }
                }
            }

            for (unsigned i = 0; i < newOutputs.size(); ++i) {
                if (auto ptrType = dyn_cast<LLVM::LLVMPointerType>(newOutputs[i])) {
                    if (ptrType.getAddressSpace() != memorySpaceMap[memSpace]) {
                        newOutputs[i] = newPtrType;
                    }
                }
                if (auto memrefType = dyn_cast<MemRefType>(newOutputs[i])) {
                    if (memrefType.getMemorySpaceAsInt() != memorySpaceMap[memSpace]) {
                        auto newMemSpaceAttr = IntegerAttr::get(
                            IntegerType::get(funcOp.getContext(), 64),
                            memorySpaceMap[memSpace]);

                        auto newMemrefType = MemRefType::get(
                            memrefType.getShape(),
                            memrefType.getElementType(),
                            memrefType.getLayout(),
                            newMemSpaceAttr);  
                        newOutputs[i] = newMemrefType;
                    }
                }
            }            

            auto newFuncType = FunctionType::get(funcOp.getContext(), newInputs, newOutputs);

            funcOp.setFunctionType(newFuncType);

            if (!funcOp.getBody().empty()) {
                Block &entry = funcOp.getBody().front();
                for (auto [argument, newType] : 
                        llvm::zip(entry.getArguments(), newInputs)) {
                    argument.setType(newType);
                }

                // *** NOTE: We need to add these ops if we have a GPU memspace
                // llvm.func @mgpuMemAlloc(%size: i64, %stream: !llvm.ptr, %flags: i8) -> !llvm.ptr<1>
                // llvm.func @mgpuMemFree(%p: !llvm.ptr<1>, %stream: !llvm.ptr) -> i32
                // The memory space should match the kernel's e.g. use newPtrType
                if(!memSpace.empty() && memSpace != "host") {
                    OpBuilder builder(funcOp);
                    builder.setInsertionPointToStart(module.getBody());
                    auto moduleTopLoc = module.getLoc();

                    auto hostPtrType = LLVM::LLVMPointerType::get(
                        module.getContext(),
                        memorySpaceMap["host"]
                    );
                    
                    auto mgpuAllocfuncType = LLVM::LLVMFunctionType::get(
                        newPtrType,  // Target memspace
                        {builder.getI64Type(), hostPtrType, builder.getI8Type()},
                        false
                    );

                    auto mgpuFreefuncType = LLVM::LLVMFunctionType::get(
                        builder.getI32Type(),
                        {newPtrType, hostPtrType},
                        false
                    );

                    // Check we haven't already declared the functions as we can run this pass multiple times
                    if (!module.lookupSymbol<LLVM::LLVMFuncOp>("mgpuMemAlloc")) {
                        builder.create<LLVM::LLVMFuncOp>(
                            moduleTopLoc,
                            "mgpuMemAlloc",
                            mgpuAllocfuncType,
                            LLVM::Linkage::External
                        );
                    }

                    if (!module.lookupSymbol<LLVM::LLVMFuncOp>("mgpuMemFree")) {
                        builder.create<LLVM::LLVMFuncOp>(
                            moduleTopLoc,
                            "mgpuMemFree",
                            mgpuFreefuncType,
                            LLVM::Linkage::External
                        );
                    }
                }

            }
        });
        
        module.walk([&](LLVM::GEPOp gepOp) {
            auto resultPtrType = dyn_cast<LLVM::LLVMPointerType>(
                gepOp.getResult().getType());
                
            if (!resultPtrType) return;
            
            if (resultPtrType.getAddressSpace() == memorySpaceMap[memSpace]) return;
            
            gepOp.getResult().setType(
                LLVM::LLVMPointerType::get(gepOp.getContext(), memorySpaceMap[memSpace]));
        });

        module.walk([&](func::CallOp callOp) {
            for (auto operand : callOp.getArgOperands()) {
                if (isa<LLVM::LLVMPointerType>(operand.getType())) {
                    operand.setType(newPtrType);
                }

                if (isa<MemRefType>(operand.getType())) {
                    auto memrefType = cast<MemRefType>(operand.getType());
                    auto newMemrefType = MemRefType::get(
                        memrefType.getShape(),
                        memrefType.getElementType(),
                        memrefType.getLayout(),
                        newMemSpaceAttr);
                    operand.setType(newMemrefType);
                }
            }

            for (auto result : callOp.getResults()) {
                if (isa<LLVM::LLVMPointerType>(result.getType())) {
                    result.setType(newPtrType);
                }

                if (isa<MemRefType>(result.getType())) {
                    auto memrefType = cast<MemRefType>(result.getType());
                    auto newMemrefType = MemRefType::get(
                        memrefType.getShape(),
                        memrefType.getElementType(),
                        memrefType.getLayout(),
                        newMemSpaceAttr);
                    result.setType(newMemrefType);
                }
            }
        });

        module.walk([&](memref::DeallocOp deallocOp) {
            OpBuilder builder(deallocOp);
            auto memrefOperand = deallocOp.getMemref();
            builder.create<gpu::DeallocOp>(deallocOp.getLoc(), Type(), ValueRange(), memrefOperand);

            deallocOp.erase();
        });
    }
};

} // namespace mlir::exahype

#endif
