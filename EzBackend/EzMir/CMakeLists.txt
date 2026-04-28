set(EZMIR_FILES_LIST
        "include/EzMir.h"
        "include/EzMirCommon.h"
        "include/MirBlock.h"
        "include/Emitter/MirEmitter.h"
        "include/Emitter/MirEmitterContext.h"
        "include/Instruction/MirInstructionDefs.h"
        "include/Instruction/MirInstruction.h"
        "include/Instruction/MirInstructionSet.h"
        "include/MirPass/IMirAnalysisPass.h"
        "include/MirPass/IMirPass.h"
        "include/MirPass/IMirTransformPass.h"
        "include/MirPass/MirPassManager.h"
        "include/Function/MirFunction.h"
        "include/Operand/MirOperand.h"
        "include/Type/MirType.h"
        "include/MirPass/Passes/CodeFlowAnalysis.h"
        "include/MirPass/Passes/LivenessAnalysis.h"

        "src/MirBlock.cpp"
        "src/Emitter/MirEmitter.cpp"
        "src/Emitter/MirEmitterContext.cpp"
        "src/Instruction/MirInstruction.cpp"
        "src/Function/MirFunction.cpp"
        "src/Operand/MirOperand.cpp"
        "src/Type/MirType.cpp"
        "src/MirPass/Passes/CodeFlowAnalysis.cpp"
        "src/MirPass/Passes/LivenessAnalysis.cpp"
)

EzCMK_ConfigureLib(NAME "EzMir"
        FILE_LIST ${EZMIR_FILES_LIST}
        PRECOMPILED_HEADER "include/EzMirCommon.h"
        LINKED_LIBRARIES "EzCore;libtommath"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzMir")

    EzPacker_AddTest(
            "T_MirEmitterContext"
            "EzMir"
            ""
            "tests/T_MirEmitterContext.cpp"
    )

    EzPacker_AddTest(
            "T_MirEmitter"
            "EzMir"
            ""
            "tests/T_MirEmitter.cpp"
    )

    EzPacker_AddTest(
            "T_CodeFlowAnalysis"
            "EzMir"
            ""
            "tests/T_CodeFlowAnalysis.cpp"
    )

    EzPacker_AddTest(
            "T_LivenessAnalysis"
            "EzMir"
            ""
            "tests/T_LivenessAnalysis.cpp"
    )
endif ()
