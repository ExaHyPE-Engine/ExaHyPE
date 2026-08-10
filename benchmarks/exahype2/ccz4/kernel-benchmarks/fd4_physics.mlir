module {
  func.func @maxEigenvalue_bridge(%arg0: !llvm.ptr, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: f64, %arg4: f64, %arg5: i32, %arg6: !llvm.ptr) attributes {llvm.linkage = #llvm.linkage<external>} {
    %c1_i32 = arith.constant 1 : i32
    %cst = arith.constant 5.000000e-01 : f64
    %cst_0 = arith.constant 0.000000e+00 : f64
    %cst_1 = arith.constant 1.000000e+00 : f64
    %0 = llvm.load %arg0 : !llvm.ptr -> f64
    %1 = arith.divf %cst_1, %0 : f64
    %2 = llvm.getelementptr %arg0[4] : (!llvm.ptr) -> !llvm.ptr, f64
    %3 = llvm.load %2 : !llvm.ptr -> f64
    %4 = arith.mulf %1, %cst : f64
    %5 = llvm.getelementptr %arg0[1] : (!llvm.ptr) -> !llvm.ptr, f64
    %6 = llvm.load %5 : !llvm.ptr -> f64
    %7 = arith.mulf %6, %6 : f64
    %8 = llvm.getelementptr %arg0[2] : (!llvm.ptr) -> !llvm.ptr, f64
    %9 = llvm.load %8 : !llvm.ptr -> f64
    %10 = arith.mulf %9, %9 : f64
    %11 = arith.addf %7, %10 : f64
    %12 = llvm.getelementptr %arg0[3] : (!llvm.ptr) -> !llvm.ptr, f64
    %13 = llvm.load %12 : !llvm.ptr -> f64
    %14 = arith.mulf %13, %13 : f64
    %15 = arith.addf %11, %14 : f64
    %16 = arith.mulf %4, %15 : f64
    %17 = arith.subf %3, %16 : f64
    %18 = arith.mulf %17, %cst_0 : f64
    %19 = arith.mulf %18, %1 : f64
    %20 = math.sqrt %19 : f64
    %21 = arith.addi %arg5, %c1_i32 : i32
    %22 = llvm.getelementptr %arg0[%21] : (!llvm.ptr, i32) -> !llvm.ptr, f64
    %23 = llvm.load %22 : !llvm.ptr -> f64
    %24 = arith.mulf %23, %1 : f64
    %25 = arith.subf %24, %20 : f64
    %26 = arith.addf %24, %20 : f64
    %27 = arith.cmpf ogt, %25, %cst_0 : f64
    %28 = scf.if %27 -> (f64) {
      scf.yield %25 : f64
    } else {
      %34 = arith.negf %25 : f64
      scf.yield %34 : f64
    }
    llvm.store %28, %arg6 : f64, !llvm.ptr
    %29 = llvm.load %arg6 : !llvm.ptr -> f64
    %30 = arith.cmpf ogt, %26, %29 : f64
    scf.if %30 {
      llvm.store %26, %arg6 : f64, !llvm.ptr
    }
    %31 = arith.negf %26 : f64
    %32 = llvm.load %arg6 : !llvm.ptr -> f64
    %33 = arith.cmpf ogt, %31, %32 : f64
    scf.if %33 {
      llvm.store %31, %arg6 : f64, !llvm.ptr
    }
    return
  }
  func.func @flux_bridge(%arg0: !llvm.ptr, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: f64, %arg4: f64, %arg5: i32, %arg6: !llvm.ptr) attributes {llvm.linkage = #llvm.linkage<external>} {
    %c1_i32 = arith.constant 1 : i32
    %cst = arith.constant 5.000000e-01 : f64
    %cst_0 = arith.constant 0.000000e+00 : f64
    %cst_1 = arith.constant 1.000000e+00 : f64
    %0 = llvm.load %arg0 : !llvm.ptr -> f64
    %1 = arith.divf %cst_1, %0 : f64
    %2 = llvm.getelementptr %arg0[4] : (!llvm.ptr) -> !llvm.ptr, f64
    %3 = llvm.load %2 : !llvm.ptr -> f64
    %4 = arith.mulf %1, %cst : f64
    %5 = llvm.getelementptr %arg0[1] : (!llvm.ptr) -> !llvm.ptr, f64
    %6 = llvm.load %5 : !llvm.ptr -> f64
    %7 = arith.mulf %6, %6 : f64
    %8 = llvm.getelementptr %arg0[2] : (!llvm.ptr) -> !llvm.ptr, f64
    %9 = llvm.load %8 : !llvm.ptr -> f64
    %10 = arith.mulf %9, %9 : f64
    %11 = arith.addf %7, %10 : f64
    %12 = llvm.getelementptr %arg0[3] : (!llvm.ptr) -> !llvm.ptr, f64
    %13 = llvm.load %12 : !llvm.ptr -> f64
    %14 = arith.mulf %13, %13 : f64
    %15 = arith.addf %11, %14 : f64
    %16 = arith.mulf %4, %15 : f64
    %17 = arith.subf %3, %16 : f64
    %18 = arith.mulf %17, %cst_0 : f64
    %19 = arith.addi %arg5, %c1_i32 : i32
    %20 = llvm.getelementptr %arg0[%19] : (!llvm.ptr, i32) -> !llvm.ptr, f64
    %21 = llvm.load %20 : !llvm.ptr -> f64
    llvm.store %21, %arg6 : f64, !llvm.ptr
    %22 = llvm.load %20 : !llvm.ptr -> f64
    %23 = llvm.load %5 : !llvm.ptr -> f64
    %24 = arith.mulf %22, %23 : f64
    %25 = arith.mulf %24, %1 : f64
    %26 = llvm.getelementptr %arg6[1] : (!llvm.ptr) -> !llvm.ptr, f64
    llvm.store %25, %26 : f64, !llvm.ptr
    %27 = llvm.load %20 : !llvm.ptr -> f64
    %28 = llvm.load %8 : !llvm.ptr -> f64
    %29 = arith.mulf %27, %28 : f64
    %30 = arith.mulf %29, %1 : f64
    %31 = llvm.getelementptr %arg6[2] : (!llvm.ptr) -> !llvm.ptr, f64
    llvm.store %30, %31 : f64, !llvm.ptr
    %32 = llvm.load %20 : !llvm.ptr -> f64
    %33 = llvm.load %12 : !llvm.ptr -> f64
    %34 = arith.mulf %32, %33 : f64
    %35 = arith.mulf %34, %1 : f64
    %36 = llvm.getelementptr %arg6[3] : (!llvm.ptr) -> !llvm.ptr, f64
    llvm.store %35, %36 : f64, !llvm.ptr
    %37 = llvm.load %20 : !llvm.ptr -> f64
    %38 = llvm.load %2 : !llvm.ptr -> f64
    %39 = arith.addf %38, %18 : f64
    %40 = arith.mulf %37, %39 : f64
    %41 = arith.mulf %40, %1 : f64
    %42 = llvm.getelementptr %arg6[4] : (!llvm.ptr) -> !llvm.ptr, f64
    llvm.store %41, %42 : f64, !llvm.ptr
    return
  }
  func.func @nonconservativeProduct_bridge(%arg0: memref<?xi8>, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: memref<?xi8>, %arg4: f64, %arg5: f64, %arg6: i32, %arg7: memref<?xi8>) attributes {llvm.linkage = #llvm.linkage<external>} {
    return
  }
  func.func @sourceTerm_bridge(%arg0: memref<?xi8>, %arg1: memref<?xi8>, %arg2: memref<?xi8>, %arg3: f64, %arg4: f64, %arg5: memref<?xi8>) attributes {llvm.linkage = #llvm.linkage<external>} {
    return
  }
}

