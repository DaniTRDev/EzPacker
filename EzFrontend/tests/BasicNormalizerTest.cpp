#include "BasicNormalizerTest.h"

bool BasicNormalizerTest::test()
{
    std::string input = ".module myModule(.i64 %myVar, .i64 %myVar2)\n"
                        ".add .i64 %rcx, .i64 %rbx\n"
                        ".add .i64 (, %rax, 4), .i64 %rbx\n"
                        ".end\n";
    std::shared_ptr<SourceManager> sourceManager = std::make_shared<SourceManager>();
    std::shared_ptr<FrontendLogger> logger = std::make_shared<FrontendLogger>(sourceManager);
    std::shared_ptr<BasicTokenizer> tokenizer = std::make_shared<BasicTokenizer>(sourceManager, logger, "TEST");

    sourceManager->addSourceContent("TEST", input);
    bool tokenizeResult = tokenizer->tokenize((char *)input.data(), 0, input.size());

    if (!tokenizeResult)
        return false;

    std::shared_ptr<Ast> result = std::make_shared<TokenTypeNode>();
    std::shared_ptr<BasicParser> parser = std::make_shared<BasicParser>(logger, sourceManager, tokenizer->getTokens());

    std::shared_ptr<Ast> out = std::make_shared<TokenTypeNode>();
    grammar::module()->matchRet(*parser, out);

    std::shared_ptr<SymbolTable> symbolTable = std::make_shared<SymbolTable>();
    std::shared_ptr<TypeTable> typeTable = std::make_shared<TypeTable>();
    std::shared_ptr<InstructionTable> instrTable = std::make_shared<InstructionTable>();

    typeTable->beginScope();
    instrTable->beginScope();
    symbolTable->beginScope();

    typeTable->addElement(std::make_shared<TypeEntry>(TypeEntry{.m_size = 1, .m_name = "i64"}), "i64");
    instrTable->addElement(std::make_shared<InstructionEntry>(InstructionEntry{.m_id = 1}), "add");

    std::shared_ptr<NormalizerContext> context = std::make_shared<NormalizerContext>();
    context->m_logger = logger;
    context->m_instructionTable = instrTable;
    context->m_symbolTable = symbolTable;
    context->m_typeTable = typeTable;

    auto normalized = ModuleNormalizer().normalizeNode(out->getChildren()[0], context);

    typeTable->endScope();
    instrTable->endScope();
    symbolTable->endScope();
    return normalized != nullptr;
}

void BasicNormalizerTest::SetUp()
{
    Test::SetUp();
}

void BasicNormalizerTest::TearDown()
{
    Test::TearDown();
}
