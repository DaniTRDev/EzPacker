set(EZDSL_LEXER_FILES_LIST
    "include/EzDslLexerCommon.h"
    "include/Ast/CallingConvDefLangAst.h"
    "include/Ast/CommonAstNodes.h"
    "include/Ast/InstructionDefLangAst.h"
    "include/Ast/InstructionSelDefLangAst.h"
    "include/Ast/IrInstructionDefLangAst.h"
    "include/Ast/LegalizeActionDefLangAst.h"
    "include/Ast/LegalizeRuleDefLangAst.h"
    "include/Ast/TargetDefLangAst.h"
    "include/Ast/TypeDefLangAst.h"
    "include/Parser/CommonParsers.h"
    "include/Parser/InstructionDefLang.h"
    "include/Parser/InstructionSelDefLang.h"
    "include/Parser/IrInstructionDefLang.h"
    "include/Parser/LegalizeActionDefLang.h"
    "include/Parser/LegalizeRuleDefLang.h"
    "include/Parser/ParseContext.h"
    "include/Parser/TargetDefLang.h"
    "include/Parser/TypeDefLang.h"

    "src/Parser/ParseContext.cpp"
)

EzCMK_ConfigureLib(NAME "EzDslLexer"
        FILE_LIST ${EZDSL_LEXER_FILES_LIST}
        PRECOMPILED_HEADER "include/EzDslLexerCommon.h"
        LINKED_LIBRARIES "EzCore;foonathan::lexy"
        INCLUDED_DIRS "include"
)