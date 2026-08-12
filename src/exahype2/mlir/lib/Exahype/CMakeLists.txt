add_mlir_dialect_library(MLIRExahype
        ExahypePasses.cpp

        ADDITIONAL_HEADER_DIRS
        ${PROJECT_SOURCE_DIR}/include/Exahype

        DEPENDS
        MLIRExahypePassesIncGen

        LINK_LIBS PUBLIC
        MLIRIR
        MLIRInferTypeOpInterface
        MLIRFuncDialect
        MLIRMemRefDialect
        MLIRSCFDialect
        MLIRArithDialect
        )
