#include "RealCompilerSession.h"
#include "EzFrontendCompiler.h"
#include "ErrorCollector/ErrorCollector.h"
#include "ErrorCollector/ErrorEmitter.h"
#include "SourceManager/SourceManager.h"
#include "Tokenizer/BasicTokenizer.h"
#include "AstNode/AstNode.h"
#include "Emitter/MirEmitter.h"
#include "Emitter/MirEmitterContext.h"
#include "Function/MirFunction.h"
#include "Instruction/MirInstruction.h"
#include "Analyzers/CfgBuilder.h"
#include "Analyzers/LivenessAnalyzer.h"
#include "Architectures/x64/x64Lowerer.h"
#include "LoweringPasses/RegisterAllocator.h"
#include "ABIDesc.h"
#include "SystemV64.h"
#include "StackFrameBuilder.h"
#include <sstream>

// Dummy implementation of AST node extraction since it depends on Visitors
// We just create a dummy AST info for now. In real logic, we'd traverse the TypedPoolLinkedList.

namespace EzGui
{

RealCompilerSession::RealCompilerSession()
{
    m_errorCollector = std::make_shared<ErrorCollector>();
    m_sourceManager = std::make_shared<SourceManager>(std::filesystem::current_path());
    m_errorCollector->addSubscriber(OnCompilerError, this);
}

RealCompilerSession::~RealCompilerSession() = default;

std::vector<TokenInfo> RealCompilerSession::GetTokens() const
{
    if (!m_mainUnit)
        return {};
    auto tokenizer = m_mainUnit->getTokenizer();
    if (!tokenizer)
        return {};

    std::vector<TokenInfo> tokens;
    for (const auto &t : tokenizer->getTokens())
    {
        tokens.push_back({ "Token", // We can enhance this later
                           t.m_str,
                           (int)t.m_sourceReference.m_line,
                           (int)t.m_sourceReference.m_col });
    }
    return tokens;
}

AstNodeInfo RealCompilerSession::GetAstRoot() const
{
    if (!m_mainUnit)
        return { "No AST", "Run compilation", {} };

    auto globalNodes = m_mainUnit->getGlobalScopeAstNodes();
    if (!globalNodes)
        return { "Empty AST", "", {} };

    AstNodeInfo root{ "Module", "Parsed source", {} };
    for (auto *node : *globalNodes)
    {
        AstNodeInfo child{ node->getAstNodeName(), "", {} };
        root.children.push_back(std::move(child));
    }
    return root;
}

std::vector<MirBlockInfo> RealCompilerSession::GetMirBlocks() const
{
    if (!m_mainUnit)
        return {};

    auto ctx = m_mainUnit->getMirEmitterContext();
    if (!ctx)
        return {};

    auto functionList = ctx->getFunctionList();
    if (!functionList)
        return {};

    std::vector<MirBlockInfo> blocks;
    for (auto *function : *functionList)
    {
        auto funcBlocks = function->getBlocks();
        if (!funcBlocks)
            continue;
        for (auto *block : *funcBlocks)
        {
            MirBlockInfo info;
            info.label = "bb" + std::to_string(block->getId());
            auto instructions = block->getInstructions();
            if (instructions)
            {
                for (auto *inst : *instructions)
                {
                    info.instructions.push_back(inst->toString());
                }
            }
            blocks.push_back(std::move(info));
        }
    }
    return blocks;
}

std::string RealCompilerSession::GetMirAnalysisText() const
{
    if (!m_mainUnit)
        return "";
    auto ctx = m_mainUnit->getMirEmitterContext();
    if (!ctx)
        return "";

    auto functionList = ctx->getFunctionList();
    if (!functionList || functionList->m_numElems == 0)
        return "No functions generated.";

    // Basic CFG Analysis on first function
    m_errorCollector->beginScope();
    auto *firstFunc = *functionList->begin();
    auto blocks = firstFunc->getBlocks();

    ABIDesc abi = createSystemV64ABI();
    CfgBuilder cfgBuilder;
    LivenessAnalyzer liveness(&abi, cfgBuilder);

    std::stringstream ss;
    if (cfgBuilder.analyze(blocks, ctx.get(), m_mainUnit.get()))
    {
        ss << "CFG Analysis:\n";
        for (const auto &[block, node] : cfgBuilder.getGraph())
        {
            ss << "  [Block bb" << block->getId() << "]\n";
            ss << "    Predecessors: ";
            if (node.m_predecessors)
            {
                for (auto *pred : *node.m_predecessors)
                    ss << "bb" << pred->getId() << " ";
            }
            ss << "\n    Successors: ";
            if (node.m_successors)
            {
                for (auto *suc : *node.m_successors)
                    ss << "bb" << suc->getId() << " ";
            }
            ss << "\n";
        }
    }

    if (liveness.analyze(blocks, ctx.get(), m_mainUnit.get()))
    {
        ss << "\nLiveness Analysis:\n";
        for (auto *block : *blocks)
        {
            const auto &liveInfo = liveness.getLiveness(block);
            ss << "  [Block bb" << block->getId() << "]\n";
            ss << "    LiveIn: " << liveInfo.m_liveIn.size() << " regs\n";
            ss << "    LiveOut: " << liveInfo.m_liveOut.size() << " regs\n";
        }
    }

    m_errorCollector->endScope(ErrorAction::Propagate);
    return ss.str();
}

std::vector<std::string> RealCompilerSession::GetLoweredInstructions() const
{
    if (!m_mainUnit)
        return { "Assembly Output Pending Lowerer Intg..." };
    auto ctx = m_mainUnit->getMirEmitterContext();
    if (!ctx)
        return { "No context." };

    auto functionList = ctx->getFunctionList();
    if (!functionList || functionList->m_numElems == 0)
        return { "No functions." };

    auto *firstFunc = *functionList->begin();
    auto blocks = firstFunc->getBlocks();

    m_errorCollector->beginScope();
    ABIDesc abi = createSystemV64ABI();
    CfgBuilder cfgBuilder;
    LivenessAnalyzer liveness(&abi, cfgBuilder);
    cfgBuilder.analyze(blocks, ctx.get(), m_mainUnit.get());
    liveness.analyze(blocks, ctx.get(), m_mainUnit.get());

    StackFrameBuilder stackFrame(&abi, 0);
    RegisterAllocator regAlloc(&liveness, &stackFrame);
    auto emitter = m_mainUnit->getMirEmitter();

    if (!regAlloc.run(blocks, emitter.get(), m_mainUnit.get()))
    {
        m_errorCollector->endScope(ErrorAction::Propagate);
        return { "Register Allocation failed." };
    }

    asmjit::StringLogger logger;
    EzMirx64Lowerer lowerer(&abi, ctx.get(), &stackFrame);
    lowerer.setLogger(&logger);
    
    auto bytes = lowerer.lower(blocks, m_mainUnit.get());
    if (bytes.empty())
    {
        return { "Lowering returned no bytes (or failed)." };
    }

    std::vector<std::string> out;
    out.push_back("--- Generated AsmJit Assembly ---");
    out.push_back(logger.data());
    
    out.push_back("--- Lowered X64 Machine Code (" + std::to_string(bytes.size()) + " bytes) ---");
    std::stringstream hexStr;
    for (auto b : bytes)
    {
        char buf[4];
        snprintf(buf, sizeof(buf), "%02X ", b);
        hexStr << buf;
    }
    out.push_back(hexStr.str());

    m_errorCollector->endScope(ErrorAction::Propagate);
    return out;
}

std::vector<LogMessage> RealCompilerSession::GetLogs() const { return m_logs; }

void RealCompilerSession::CompileSource(const std::string &source)
{
    m_logs.clear();
    m_lastSource = source;
    m_errorCollector->beginScope();
    m_driver = std::make_unique<FrontendCompilerDriver>(m_errorCollector, m_sourceManager);
    m_driver->addSource(source, "editor.ez", &m_mainUnit);
    m_driver->compile();
    m_errorCollector->endScope(ErrorAction::Propagate);
}

void RealCompilerSession::OnCompilerError(void *userParam, const std::shared_ptr<Error> &error)
{
    auto *self = static_cast<RealCompilerSession *>(userParam);
    if (!self || !error)
        return;

    LogMessage log;
    log.severity = static_cast<int>(error->m_severity);
    log.module = error->m_sender;
    log.text = error->m_message;
    log.location =
            "line:" + std::to_string(error->m_sourceRef.m_line) + " col:" + std::to_string(error->m_sourceRef.m_col);
    self->m_logs.push_back(std::move(log));
}

} // namespace EzGui
