get_property(dialect_libs GLOBAL PROPERTY MLIR_DIALECT_LIBS)
get_property(conversion_libs GLOBAL PROPERTY MLIR_CONVERSION_LIBS)
get_property(translation_libs GLOBAL PROPERTY MLIR_TRANSLATION_LIBS)  

set(LIBS
        ${dialect_libs}
        ${conversion_libs}
        ${translation_libs}                       
        MLIRArithDialect
        MLIRGPUDialect
        MLIROptLib
        MLIRExahype
        MLIRMlirOptMain
        # Explicit translation targets for GPU/Grace-Hopper pipeline:
        MLIRGPUToLLVMIRTranslation
        MLIRNVVMToLLVMIRTranslation
        MLIRBuiltinToLLVMIRTranslation
        MLIRLLVMToLLVMIRTranslation
        )

add_llvm_executable(dslhype-opt dslhype-opt.cpp)

add_dependencies(dslhype-opt MLIRExahypePassesIncGen)

llvm_update_compile_flags(dslhype-opt)
target_link_libraries(dslhype-opt PRIVATE ${LIBS})

mlir_check_all_link_libraries(dslhype-opt)

install(TARGETS dslhype-opt DESTINATION bin)

