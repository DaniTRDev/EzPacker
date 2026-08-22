set(EZDSL_FILES_LIST
        "include/EzDslCommon.h"
        "include/Ast/CommonAstNodes.h"
        "include/Ast/InstructionDefLangAst.h"
        "include/Ast/InstructionSelDefLang.h"
        "include/Ast/LegalizeActionDefLangAst.h"
        "include/Ast/LegalizeRuleDefLangAst.h"
        "include/Ast/TargetDefLangAst.h"
        "include/Parser/CommonParsers.h"
        "include/Parser/InstructionDefLang.h"
        "include/Parser/LegalizeActionDefLang.h"
        "include/Parser/ParseContext.h"
        "include/Parser/TargetDefLang.h"

        "src/Parser/ParseContext.cpp"
)

EzCMK_ConfigureLib(NAME "EzDsl"
        FILE_LIST ${EZDSL_FILES_LIST}
        PRECOMPILED_HEADER "include/EzDslCommon.h"
        LINKED_LIBRARIES "EzCore;foonathan::lexy"
        INCLUDED_DIRS "include"
)