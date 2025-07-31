#include "EzLifter.h"

EzLifter::EzLifter(std::unique_ptr<IArchitecture> arch, std::unique_ptr<IDecoder> decoder)
    : m_architecture(std::move(arch)), m_decoder(std::move(decoder)), m_context(std::make_unique<llvm::LLVMContext>()),
      LogSink(g_logger.get(), LogSegment("EzLifter").colorize(Colors::green))
{
    m_module = std::make_unique<llvm::Module>("EzLifter", *m_context);
}

EzLifter::~EzLifter()
{
    m_module.reset();
}

bool EzLifter::initialize()
{
    using namespace llvm;
    FunctionType *funcType = FunctionType::get(Type::getVoidTy(*m_context), false);
    Function *llvmFunc = Function::Create(funcType, Function::ExternalLinkage, "lifted", m_module.get());
    m_function = llvmFunc;

    if (!llvmFunc || !m_decoder->initialize(m_architecture))
    {
        LogSink::pushLog(
            LogMessage("")
                .add("Could not initialize Lifter because LLVM func was not created or decoder could not initialize")
                .colorize(Colors::red));
        return false;
    }

    LogSink::pushLog(LogMessage("Lifter intialized"));
    return true;
}

llvm::Function *EzLifter::lift(char *buffer, size_t baseAddress, size_t bufferSize, size_t startAddress)
{
    if (!m_function)
    {
        LogSink::pushLog(LogMessage("Can't lift because lifter was not initialized").colorize(Colors::red));
        return nullptr;
    }

    using namespace llvm;

    BasicBlock *entry = BasicBlock::Create(*m_context, "entry", m_function);
    IRBuilder<> builder(entry);

    size_t address = startAddress;
    while (address < bufferSize)
    {
        std::vector<std::shared_ptr<IDecodedOperand>> operands;
        auto instr = m_decoder->decodeInstruction(buffer, address, bufferSize, operands);

        if (instr == nullptr)
        {
            LogSink::pushLog(LogMessage("Could not lift instruction, aborting").colorize(Colors::red));
            return nullptr;
        }

        const std::shared_ptr<ParsedInstructionData> &instrData = instr->getData();

        if (instrData->m_instrType == DecodedInstructionType::Unsupported)
        {
            LogSink::pushLog(LogMessage("Detected an unsupported instruction").colorize(Colors::red));
            return nullptr;
        }

        bool liftResult = false;
        InstructionLiftContext context = {
            .m_builder = builder, .m_data = instrData, .m_context = m_context, .m_operands = operands};

        auto it = g_lifters.find(instrData->m_instrType);
        if (it == g_lifters.end())
        {
            LogSink::pushLog(LogMessage("No lifter found for instruction").colorize(Colors::red));
            return nullptr;
        }
        
        liftResult = it->second->liftFunction(context);
        if (!liftResult)
        {
            LogSink::pushLog(LogMessage("Could not lift instruction.").colorize(Colors::red));
            return nullptr;
        }
    }

    builder.CreateRetVoid();
    return m_function;
}
