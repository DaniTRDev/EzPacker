set(EZMIR_FILES_LIST
        "include/EzMir.h"
        "include/EzMirCommon.h"
        "include/Block/MirBlock.h"
        "include/Block/MirBlockBuilder.h"
        "include/Block/MirBlockInstructionQuery.h"
        "include/Builder/MirBuilder.h"
        "include/Builder/MirBuilderContext.h"
        "include/Class/MirClass.h"
        "include/Class/MirClassBuilder.h"
        "include/Instruction/MirInstructionDefs.h"
        "include/Instruction/MirInstruction.h"
        "include/Instruction/MirInstructionBuilder.h"
        "include/Instruction/MirInstructionSet.h"
        "include/MirPasses/IMirAnalysisPass.h"
        "include/MirPasses/IMirTransformPass.h"
        "include/MirPasses/MirPass.h"
        "include/MirPasses/MirPassManager.h"
        "include/Function/MirFunction.h"
        "include/Function/MirFunctionBuilder.h"
        "include/Function/MirFunctionStackFrame.h"
        "include/Operand/MirOperand.h"
        "include/Operand/MirOperandBuilder.h"
        "include/Operand/MirOperands.h"
        "include/Printer/MirPrinter.h"
        "include/Type/MirType.h"
        "include/Type/MirTypeTable.h"
        "include/MirPasses/Passes/CodeFlowAnalysis.h"
        "include/MirPasses/Passes/LivenessAnalysis.h"

        "src/Block/MirBlock.cpp"
        "src/Block/MirBlockBuilder.cpp"
        "src/Block/MirBlockInstructionQuery.cpp"
        "src/Builder/MirBuilderContext.cpp"
        "src/Class/MirClass.cpp"
        "src/Class/MirClassBuilder.cpp"
        "src/Instruction/MirInstructionBuilder.cpp"
        "src/Instruction/MirInstruction.cpp"
        "src/Function/MirFunction.cpp"
        "src/Function/MirFunctionBuilder.cpp"
        "src/Function/MirFunctionStackFrame.cpp"
        "src/MirPasses/MirPass.cpp"
        "src/MirPasses/MirPassManager.cpp"
        "src/MirPasses/Passes/CodeFlowAnalysis.cpp"
        "src/MirPasses/Passes/LivenessAnalysis.cpp"
        "src/Operand/MirOperand.cpp"
        "src/Operand/MirOperandBuilder.cpp"
        "src/Printer/MirPrinter.cpp"
        "src/Type/MirType.cpp"
        "src/Type/MirTypeTable.cpp"
)

set(MIRTESTSUITE_FILES_LIST
        "tests/EzMirTestSuite/EzMirTestSuite.h"
        "tests/EzMirTestSuite/Verifiers/CodeFlowPassVerifier.h"
        "tests/EzMirTestSuite/Verifiers/LivenessPassVerifier.h"
        "tests/EzMirTestSuite/Verifiers/MirCoreVerifiers.h"

        "tests/EzMirTestSuite/EzMirTestSuite.cpp"
        "tests/EzMirTestSuite/Verifiers/CodeFlowPassVerifier.cpp"
        "tests/EzMirTestSuite/Verifiers/LivenessPassVerifier.cpp"
        "tests/EzMirTestSuite/Verifiers/MirCoreVerifiers.cpp"
)

EzCMK_ConfigureLib(NAME "EzMir"
        FILE_LIST ${EZMIR_FILES_LIST}
        PRECOMPILED_HEADER "include/EzMirCommon.h"
        LINKED_LIBRARIES "EzCore"
        INCLUDED_DIRS "include")

if (EZPACKER_BUILD_TESTS)
    message(STATUS "Building tests for EzMir")

    EzCMK_ConfigureLib(NAME "EzMirTestSuite"
            FILE_LIST ${MIRTESTSUITE_FILES_LIST}
            PRECOMPILED_HEADER "tests/EzMirTestSuite/Verifiers/MirCoreVerifiers.h"
            LINKED_LIBRARIES "EzMir"
            INCLUDED_DIRS "tests/EzMirTestSuite"
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
            "T_Block"
            "EzMirTestSuite"
            ""
            "tests/T_Block.cpp"
    )

    EzPacker_AddTest(
            "T_Class"
            "EzMirTestSuite"
            ""
            "tests/T_Class.cpp"
    )

    EzPacker_AddTest(
            "T_Function"
            "EzMirTestSuite"
            ""
            "tests/T_Function.cpp"
    )

    EzPacker_AddTest(
            "T_CodeFlowPass"
            "EzMirTestSuite"
            ""
            "tests/T_CodeFlowPass.cpp"
    )

    EzPacker_AddTest(
            "T_LivenessAnalysis"
            "EzMirTestSuite"
            ""
            "tests/T_LivenessAnalysis.cpp"
    )
endif ()
