set(EZMIR_FILES_LIST
        "include/EzMir.h"
        "include/EzMirCommon.h"
        "include/Block/MirBlock.h"
        "include/Block/MirBlockBuilder.h"
        "include/Builder/MirBuilder.h"
        "include/Builder/MirBuilderContext.h"
        "include/Block/MirBlockInstructionQuery.h"
        "include/Instruction/MirInstructionDefs.h"
        "include/Instruction/MirInstruction.h"
        "include/Instruction/MirInstructionBuilder.h"
        "include/Instruction/MirInstructionSet.h"
        "include/MirPass/IMirAnalysisPass.h"
        "include/MirPass/IMirPass.h"
        "include/MirPass/IMirTransformPass.h"
        "include/MirPass/MirPassManager.h"
        "include/Function/MirFunction.h"
        "include/Function/MirFunctionBuilder.h"
        "include/Function/MirFunctionStackFrame.h"
        "include/Operand/MirOperand.h"
        "include/Operand/MirOperandBuilder.h"
        "include/Operand/MirOperands.h"
        "include/Printer/MirPrinter.h"
        "include/Type/MirType.h"
        "include/Type/MirTypeTable.h"
        "include/MirPass/Passes/CodeFlowAnalysis.h"
        "include/MirPass/Passes/LivenessAnalysis.h"

        "src/Block/MirBlock.cpp"
        "src/Block/MirBlockBuilder.cpp"
        "src/Block/MirBlockInstructionQuery.cpp"
        "src/Builder/MirBuilderContext.cpp"
        "src/Instruction/MirInstructionBuilder.cpp"
        "src/Instruction/MirInstruction.cpp"
        "src/Function/MirFunction.cpp"
        "src/Function/MirFunctionBuilder.cpp"
        "src/Function/MirFunctionStackFrame.cpp"
        "src/MirPass/MirPassManager.cpp"
        "src/MirPass/Passes/CodeFlowAnalysis.cpp"
        "src/MirPass/Passes/LivenessAnalysis.cpp"
        "src/Operand/MirOperand.cpp"
        "src/Operand/MirOperandBuilder.cpp"
        "src/Printer/MirPrinter.cpp"
        "src/Type/MirType.cpp"
        "src/Type/MirTypeTable.cpp"
)

EzCMK_ConfigureLib(NAME "EzMir"
        FILE_LIST ${EZMIR_FILES_LIST}
        PRECOMPILED_HEADER "include/EzMirCommon.h"
        LINKED_LIBRARIES "EzCore;libtommath"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzMir")

    EzCMK_ConfigureLib(NAME "EzMirTestSuite"
            FILE_LIST "tests/MirTestSuite.cpp;tests/MirTestSuite.h"
            PRECOMPILED_HEADER "tests/MirTestSuite.h"
            LINKED_LIBRARIES "EzMir"
    )

    EzPacker_AddTest(
            "T_Operand"
            "EzMirTestSuite"
            ""
            "tests/T_Operand.cpp"
    )

    EzPacker_AddTest(
            "T_Instruction"
            "EzMirTestSuite"
            ""
            "tests/T_Instruction.cpp"
    )

    EzPacker_AddTest(
            "T_CodeFlowAnalysis"
            "EzMirTestSuite"
            ""
            "tests/T_CodeFlowAnalysis.cpp"
    )

    EzPacker_AddTest(
            "T_LivenessAnalysis"
            "EzMirTestSuite"
            ""
            "tests/T_LivenessAnalysis.cpp"
    )
endif ()
