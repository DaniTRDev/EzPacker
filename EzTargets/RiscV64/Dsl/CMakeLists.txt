# RISC-V 64 EzDSL plugin.
#
# Build-time-only target support: the RISC-V encoding dialect (semantic validation of
# .idf ENCODING directives) and the encoding code generation backend that emits the
# runtime encoding table. EzDslCli links this library so it can generate the RISC-V
# tables; the runtime library (EzTargetsRiscV64) does not depend on it.

set(EZTARGETS_RISCV64_DSL_FILES_LIST
        "include/EzTargetsRiscV64DslCommon.h"
        "include/EzTargetsRiscV64Dsl.h"
        "include/Sema/Encoding/RiscVEncodingDialect.h"
        "include/Sema/Encoding/RiscVEncodingVocabulary.h"
        "include/CodeGenerators/RiscVEncodingCodegenBackend.h"

        "src/EzTargetsRiscV64Dsl.cpp"
        "src/Sema/Encoding/RiscVEncodingDialect.cpp"
        "src/CodeGenerators/RiscVEncodingCodegenBackend.cpp"
)

EzCMK_ConfigureLib(NAME "EzTargetsRiscV64Dsl"
        FILE_LIST ${EZTARGETS_RISCV64_DSL_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTargetsRiscV64DslCommon.h"
        LINKED_LIBRARIES "EzDslSema;EzDslCodeGenerators;EzDslLexer;EzCore"
        INCLUDED_DIRS "include;${CMAKE_SOURCE_DIR}/EzDsl/Sema/include;${CMAKE_SOURCE_DIR}/EzDsl/CodeGenerators/include;${CMAKE_SOURCE_DIR}/EzDsl/Lexer/include;${CMAKE_SOURCE_DIR}/EzCore/include")
