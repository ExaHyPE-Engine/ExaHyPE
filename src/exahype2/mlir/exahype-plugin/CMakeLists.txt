add_llvm_library(ExahypePlugin
        # BUILDTREE_ONLY is only for testing purposes
        MODULE BUILDTREE_ONLY
        exahype-plugin.cpp

        DEPENDS
        MLIRExahypePassesIncGen
        MLIRExahype
        PLUGIN_TOOL
        mlir-opt

        LINK_LIBS
        MLIRExahype
        )
target_include_directories(
        ExahypePlugin
        PRIVATE
        "${EXAHYPE_BINARY_DIR}/include"
)
