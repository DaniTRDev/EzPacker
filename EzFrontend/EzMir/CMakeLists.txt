set(EZMIR_FILES_LIST
        "include/EzMir.h"
        "include/EzMirCommon.h"
        "include/MirBlock.h"
        "include/Emitter/MirEmitter.h"
        "include/Emitter/MirEmitterContext.h"
        "include/Emitter/MirGlobalDataEmitter.h"
        "include/Instruction/MirInstructionDefs.h"
        "include/Instruction/MirInstruction.h"
        "include/Instruction/MirInstructionSet.h"
        "include/Function/MirFunction.h"
        "include/Operand/MirOperand.h"

        "src/MirBlock.cpp"
        "src/Emitter/MirEmitter.cpp"
        "src/Emitter/MirEmitterContext.cpp"
        "src/Emitter/MirGlobalDataEmitter.cpp"
        "src/Instruction/MirInstruction.cpp"
        "src/Function/MirFunction.cpp"
        "src/Operand/MirOperand.cpp"
)

EzCMK_ConfigureLib(NAME "EzMir"
        FILE_LIST ${EZMIR_FILES_LIST}
        PRECOMPILED_HEADER "include/EzMirCommon.h"
        LINKED_LIBRARIES "EzCore;libtommath"
        INCLUDED_DIRS "include")

EzPacker_AddTest(
        "MirEmitterTest"
        "EzMir;gtest_main"
        ""
        "tests/MirEmitterTest.cpp;tests/MirTestFixture.h;tests/MirTestFixture.cpp"
)

EzPacker_AddTest(
        "MirEmitterContextTest"
        "EzMir;gtest_main"
        ""
        "tests/MirEmitterContextTest.cpp;tests/MirTestFixture.h;tests/MirTestFixture.cpp"
)

EzPacker_AddTest(
        "MirGlobalDataEmitterTest"
        "EzMir;gtest_main"
        ""
        "tests/MirGlobalDataEmitterTest.cpp;tests/MirTestFixture.h;tests/MirTestFixture.cpp"
)
