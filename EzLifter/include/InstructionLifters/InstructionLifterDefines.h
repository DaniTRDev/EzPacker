#ifndef EZPACKER_INSTRUCTIONLIFTERDEFINES_H
#define EZPACKER_INSTRUCTIONLIFTERDEFINES_H

#include "IInstructionLifter.h"

#define MAKE_INSTRUCTION_LIFTER(name, code)                                                                            \
    class Lifter##name : public IInstructionLifter, public LogSink                                                     \
    {                                                                                                                  \
      public:                                                                                                          \
        inline Lifter##name()                                                                                          \
            : IInstructionLifter(), LogSink(g_logger.get(), LogSegment("INSTR_LIFTER").colorize(Colors::magenta))      \
        {                                                                                                              \
        }                                                                                                              \
                                                                                                                       \
        inline bool liftFunction(const InstructionLiftContext &liftContext) override                                   \
        {                                                                                                              \
            const char *str = "Lifter##name";                                                                          \
            LogSink::pushLog(LogMessage("Lifting {}", str));                                                   \
            return code(liftContext);                                                                                  \
        }                                                                                                              \
    };                                                                                                                 \
    static bool _register_lifter_##name = []() {                                                                       \
        g_lifters[DecodedInstructionType::name] = std::make_shared<Lifter##name>();                                   \
        return true;                                                                                                   \
    }();

#endif // EZPACKER_INSTRUCTIONLIFTERDEFINES_H
