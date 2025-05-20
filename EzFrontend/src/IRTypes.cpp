#include "IRTypes.h"

const std::map<std::string_view, IRInstructionType> g_IRInstructionStr2Type{
    {"invalid", IRInstructionType::Invalid},
    {"unsupported", IRInstructionType::Unsupported},
    {"add", IRInstructionType::Add},
    {"and", IRInstructionType::And},
    {"branch", IRInstructionType::Branch},
    {"call", IRInstructionType::Call},
    {"compare", IRInstructionType::Compare},
    {"divide", IRInstructionType::Divide},
    {"exchange", IRInstructionType::Exchange},
    {"freeStack", IRInstructionType::FreeStack},
    {"jump", IRInstructionType::Jump},
    {"load", IRInstructionType::Load},
    {"lea", IRInstructionType::LoadEffectiveAddress},
    {"mul", IRInstructionType::Multiply},
    {"nop", IRInstructionType::Nop},
    {"not", IRInstructionType::Not},
    {"or", IRInstructionType::Or},
    {"pop", IRInstructionType::Pop},
    {"push", IRInstructionType::Push},
    {"reserve", IRInstructionType::ReserveStack},
    {"ret", IRInstructionType::Return},
    {"rotl", IRInstructionType::RotateLeft},
    {"rotr", IRInstructionType::RotateRight},
    {"setflags", IRInstructionType::SetFlags},
    {"shl", IRInstructionType::ShiftLeft},
    {"shr", IRInstructionType::ShiftRight},
    {"signextend", IRInstructionType::SignExtend},
    {"store", IRInstructionType::Store},
    {"sub", IRInstructionType::Subtract},
    {"test", IRInstructionType::Test},
    {"xor", IRInstructionType::Xor},
    {"prefetch", IRInstructionType::Prefetch},
    {"sysCall", IRInstructionType::SysCall},
    {"zeroextend", IRInstructionType::ZeroExtend}};

const std::map<IRInstructionType, std::string_view> g_IRInstruction2Str = {
    {IRInstructionType::Invalid, "invalid"},
    {IRInstructionType::Unsupported, "unsupported"},
    {IRInstructionType::Add, "add"},
    {IRInstructionType::And, "and"},
    {IRInstructionType::Branch, "branch"},
    {IRInstructionType::Call, "call"},
    {IRInstructionType::Compare, "compare"},
    {IRInstructionType::Divide, "divide"},
    {IRInstructionType::Exchange, "exchange"},
    {IRInstructionType::FreeStack, "freeStack"},
    {IRInstructionType::Jump, "jump"},
    {IRInstructionType::Load, "load"},
    {IRInstructionType::LoadEffectiveAddress, "lea"},
    {IRInstructionType::Multiply, "mul"},
    {IRInstructionType::Nop, "nop"},
    {IRInstructionType::Not, "not"},
    {IRInstructionType::Or, "or"},
    {IRInstructionType::Pop, "pop"},
    {IRInstructionType::Push, "push"},
    {IRInstructionType::ReserveStack, "reserve"},
    {IRInstructionType::Return, "ret"},
    {IRInstructionType::RotateLeft, "rotl"},
    {IRInstructionType::RotateRight, "rotr"},
    {IRInstructionType::SetFlags, "setflags"},
    {IRInstructionType::ShiftLeft, "shl"},
    {IRInstructionType::ShiftRight, "shr"},
    {IRInstructionType::SignExtend, "signextend"},
    {IRInstructionType::Store, "store"},
    {IRInstructionType::Subtract, "sub"},
    {IRInstructionType::Test, "test"},
    {IRInstructionType::Xor, "xor"},
    {IRInstructionType::Prefetch, "prefetch"},
    {IRInstructionType::SysCall, "sysCall"},
    {IRInstructionType::ZeroExtend, "zeroextend"}};

const std::set<std::string_view> g_IRInstructionStrSet = {
    "invalid",   "unsupported", "add",  "and",  "branch",   "call",     "compare",   "divide", "exchange",
    "freeStack", "jump",        "load", "lea",  "mul",      "nop",      "not",       "or",     "pop",
    "push",      "reserve",     "ret",  "rotl", "rotr",     "setflags", "shl",       "shr",    "signextend",
    "store",     "sub",         "test", "xor",  "prefetch", "sysCall",  "zeroextend"};

const std::map<std::string_view, IRKeywordType> g_IRKeywordTypeStr2Type = {
    {"invalid", IRKeywordType::Invalid}, {"module", IRKeywordType::Module}, {"variable", IRKeywordType::Variable}};

const std::map<IRKeywordType, std::string_view> g_IRKeywordType2Str = {
    {IRKeywordType::Invalid, "invalid"}, {IRKeywordType::Module, "module"}, {IRKeywordType::Variable, "variable"}

};
const std::set<std::string_view> g_IRKeywordTypeStrSet = {"invalid", "module", "variable"};

uint64_t getMaxIRIntValue(IRType type)
{
    switch (type)
    {
    case IRType::i8: {
        return UINT8_MAX;
    }
    case IRType::i16: {
        return UINT16_MAX;
    }
    case IRType::i32: {
        return UINT32_MAX;
    }
    case IRType::i64: {
        return UINT64_MAX;
    }
    default:
        return 0;
    }
    return 0;
}

const std::map<IRType, std::string_view> g_IRTypes2Str = {
    {IRType::Invalid, "Invalid"}, {IRType::_double, "double"}, {IRType::_float, "float"},
    {IRType::i8, "i8"},           {IRType::i16, "i16"},        {IRType::i32, "i32"},
    {IRType::i64, "i64"},         {IRType::ptr, "ptr"},        {IRType::string, "string"}};

const std::map<std::string_view, IRType> g_IRStr2Types = {
    {"Invalid", IRType::Invalid}, {"double", IRType::_double}, {"float", IRType::_float},
    {"i8", IRType::i8},           {"i16", IRType::i16},        {"i32", IRType::i32},
    {"i64", IRType::i64},         {"ptr", IRType::ptr},        {"string", IRType::string}};

const std::set<std::string_view> g_IRTypesStrSet = {"Invalid", "double", "float", "i8",    "i16",
                                                    "i32",     "i64",    "ptr",   "string"};