# x86-64 EzDSL plugin.
#
# Build-time-only target support: the x86-64 encoding dialect (semantic validation of
# .idf ENCODING directives) and the encoding code generation backend that emits the
# runtime encoding table. EzDslCli links this library so it can generate the x86-64
# tables; the runtime library (EzTargetsX86_64) does not depend on it.

set(EZTARGETS_X86_64_DSL_FILES_LIST
        "include/EzTargetsX86_64DslCommon.h"
        "include/EzTargetsX86_64Dsl.h"
        "include/Sema/Encoding/X86_64EncodingDialect.h"
        "include/Sema/Encoding/X86_64EncodingVocabulary.h"
        "include/CodeGenerators/X86_64EncodingCodegenBackend.h"

        "src/EzTargetsX86_64Dsl.cpp"
        "src/Sema/Encoding/X86_64EncodingDialect.cpp"
        "src/CodeGenerators/X86_64EncodingCodegenBackend.cpp"
)

EzCMK_ConfigureLib(NAME "EzTargetsX86_64Dsl"
        FILE_LIST ${EZTARGETS_X86_64_DSL_FILES_LIST}
        PRECOMPILED_HEADER "include/EzTargetsX86_64DslCommon.h"
        LINKED_LIBRARIES "EzDslSema;EzDslCodeGenerators;EzDslLexer;EzCore"
        INCLUDED_DIRS "include;${CMAKE_SOURCE_DIR}/EzDsl/Sema/include;${CMAKE_SOURCE_DIR}/EzDsl/CodeGenerators/include;${CMAKE_SOURCE_DIR}/EzDsl/Lexer/include;${CMAKE_SOURCE_DIR}/EzCore/include")
