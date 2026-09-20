#include "CodeGenerators/CppTargetDescGenerator.h"
#include "Ast/TargetDescDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetDescSymbols.h"

#include <cctype>
#include <format>
#include <string>

namespace CodeGenerators
{

namespace
{

// Returns the AST node of the (single) target descriptor symbol, or null when none exists.
const DSL::Ast::TargetDesc::TargetDescDecl *findTargetDesc(const SymbolTable *table)
{
    if (!table)
    {
        return nullptr;
    }

    const auto descs = table->collect<Symbols::TargetDescSymbol>(SymbolType::TargetDesc);
    if (descs.empty())
    {
        return nullptr;
    }

    const auto *data = descs.front()->getIf<Symbols::TargetDescSymbol>();
    return data ? data->m_astNode : nullptr;
}

// Escapes characters for the generated C++ string literals (delegates to the shared escaper).
std::string escapeString(std::string_view value) { return EscapeString(value, EscapeMode::CppStringLiteral); }

// Emits an inline constexpr array of escaped strings plus its element-count constant.
template <typename Node>
void emitStringArray(CppSourceEmitter &emitter,
                     std::string_view arrayName,
                     std::string_view comment,
                     const std::pmr::vector<Node> *values)
{
    if (!comment.empty())
    {
        emitter.emitComment(comment);
    }

    emitter.emitLine("inline constexpr const char *{}[] =", arrayName);
    {
        auto s = emitter.enterScope("{", "};");
        if (!values || values->empty())
        {
            emitter.emitLine("\"\",");
        }
        else
        {
            for (const auto &value : *values)
            {
                emitter.emitLine("\"{}\",", escapeString(value.m_node));
            }
        }
    }
    emitter.emitLine("inline constexpr std::size_t {}Count = {};", arrayName, values ? values->size() : 0);
    emitter.emitBlankLine();
}

} // namespace

// Binds the generator to its diagnostics/symbols and normalizes an empty target name to "Target".
CppTargetDescGenerator::CppTargetDescGenerator(DiagnosticCollector *collector,
                                               SymbolTable *table,
                                               std::filesystem::path outPath,
                                               std::string targetName) :
    CodeGenerator("CodeGenerators::TargetDesc", collector, table, std::move(outPath)),
    m_targetName(SanitizeCppIdentifier(targetName, "Target"))
{
}

// Emits the generated TargetDesc subclass declaration plus its static metadata tables.
void CppTargetDescGenerator::emitHeader(CppSourceEmitter &emitter,
                                        const DSL::Ast::TargetDesc::TargetDescDecl *decl) const
{
    const std::string ns = SanitizeCppIdentifier(m_targetName, "Target");
    const std::string className = std::format("{}TargetDesc", ns);

    emitter.emitBanner("CppTargetDescGenerator");
    emitter.emitBlankLine();
    emitter.emitLine("#pragma once");
    emitter.emitBlankLine();

    emitter.emitInclude("cstddef", true);
    emitter.emitInclude("cstdint", true);
    emitter.emitInclude("memory_resource", true);
    emitter.emitInclude("string_view", true);
    emitter.emitBlankLine();

    emitter.emitInclude("Descriptors/TargetDesc.h");
    emitter.emitInclude("Operand/MirRegisterReference.h");
    emitter.emitBlankLine();

    emitter.emitLine("class GenericCodeEmitter;");
    emitter.emitLine("class MirBuilderContext;");
    emitter.emitBlankLine();

    {
        auto nsScope = emitter.enterNamespace(std::format("EzTriple::TableGen::{}", ns));
        emitter.emitBlankLine();

        emitter.emitComment("Declarative metadata extracted from the .tdesc manifest.");
        emitStringArray(emitter,
                        "s_objectFormats",
                        "Object format names declared by the manifest.",
                        decl ? &decl->mObjectFormats : nullptr);
        emitStringArray(emitter,
                        "s_callingConvs",
                        "Calling convention config file paths declared by the manifest.",
                        decl ? &decl->m_callingConvs : nullptr);

        emitter.emitLine("inline constexpr const char *s_componentSlots[] =");
        {
            auto s = emitter.enterScope("{", "};");
            if (!decl || decl->mComponents.empty())
            {
                emitter.emitLine("\"\",");
            }
            else
            {
                for (const auto &component : decl->mComponents)
                {
                    emitter.emitLine("\"{}\",", escapeString(component.m_slot.m_node));
                }
            }
        }
        emitter.emitLine("inline constexpr std::size_t s_componentSlotCount = {};",
                         decl ? decl->mComponents.size() : 0);
        emitter.emitBlankLine();

        emitter.emitLine("inline constexpr const char *s_componentTypes[] =");
        {
            auto s = emitter.enterScope("{", "};");
            if (!decl || decl->mComponents.empty())
            {
                emitter.emitLine("\"\",");
            }
            else
            {
                for (const auto &component : decl->mComponents)
                {
                    emitter.emitLine("\"{}\",", escapeString(component.m_type.m_node));
                }
            }
        }
        emitter.emitLine("inline constexpr std::size_t s_componentTypeCount = {};",
                         decl ? decl->mComponents.size() : 0);
        emitter.emitBlankLine();

        emitter.emitLine("inline constexpr const char *s_libcallSymbols[] =");
        {
            auto s = emitter.enterScope("{", "};");
            if (!decl || decl->mLibcalls.empty())
            {
                emitter.emitLine("\"\",");
            }
            else
            {
                for (const auto &libcall : decl->mLibcalls)
                {
                    emitter.emitLine("\"{}\",", escapeString(libcall.m_symbol.m_node));
                }
            }
        }
        emitter.emitLine("inline constexpr std::size_t s_libcallCount = {};", decl ? decl->mLibcalls.size() : 0);
        emitter.emitBlankLine();

        emitter.emitComment("Concrete target descriptor generated from the .tdesc manifest.");
        {
            auto classScope = emitter.enterClass(className, "public TargetDesc");
            emitter.emitLine("public:");
            emitter.indent();
            emitter.emitLine("explicit {}(MirBuilderContext *ctx);", className);
            emitter.emitLine("~{}() override;", className);
            emitter.emitBlankLine();
            emitter.emitLine("const char *getName() const override {{ return \"{}\"; }}",
                             escapeString(decl ? decl->m_name.m_node : m_targetName));
            emitter.emitLine("MirFrameLowerer *getFrameLowerer() override { return nullptr; }");
            emitter.emitLine("MirInstructionSelector *getInstructionSelector() override { return nullptr; }");
            emitter.emitLine("MirLegalizer *getLegalizer() override { return nullptr; }");
            emitter.emitLine("LegalizerInfo *getLegalizerInfo() override { return nullptr; }");
            emitter.emitLine("MirRegisterAllocator *getRegisterAllocator() override { return nullptr; }");
            emitter.emitLine("MirType *getMemOperandDisplacementType() override;");
            emitter.emitLine("MirRegisterRef getInstructionPtrReg() const override;");
            emitter.emitLine("size_t getStackSlotSize() const override {{ return {}; }}",
                             decl && decl->m_stackSlot.has_value() ? decl->m_stackSlot->m_node : 8);
            emitter.emitLine("void initialize() override;");
            emitter.emitLine("std::string_view getLibcallStr(uint8_t symId) override;");
            emitter.emitLine("std::pmr::vector<TargetBinaryDesc *> getAvailableBinaryDescriptors() override { return "
                             "m_binaries; }");
            emitter.emitLine("std::pmr::vector<CallingConvDesc *> getAvailableCallingConventions() override { return "
                             "m_convs; }");
            emitter.emitLine(
                    "std::pmr::vector<MirRegisterBank *> getAvailableRegisterBanks() override { return m_banks; }");
            emitter.emitLine("MirRegisterBank *createRegisterBank(const char *name) override;");
            emitter.emitLine("GenericCodeEmitter *createCodeEmitter() override;");
            emitter.emitBlankLine();
            emitter.emitLine("private:");
            emitter.emitLine("MirBuilderContext *m_ctx{ nullptr };");
            emitter.emitLine("std::pmr::vector<MirRegisterBank *> m_banks;");
            emitter.emitLine("std::pmr::vector<CallingConvDesc *> m_convs;");
            emitter.emitLine("std::pmr::vector<TargetBinaryDesc *> m_binaries;");
            emitter.dedent();
        }
    }
}

// Emits the out-of-line TargetDesc method definitions and component wiring.
void CppTargetDescGenerator::emitSource(CppSourceEmitter &emitter,
                                        const DSL::Ast::TargetDesc::TargetDescDecl *decl) const
{
    const std::string ns = SanitizeCppIdentifier(m_targetName, "Target");
    const std::string className = std::format("{}TargetDesc", ns);

    emitter.emitBanner("CppTargetDescGenerator");
    emitter.emitBlankLine();
    emitter.emitInclude(std::format("{}TargetDesc.h", ns));
    emitter.emitInclude("Builder/MirBuilderContext.h");
    emitter.emitInclude("Operand/MirRegisterBank.h");
    emitter.emitInclude("Operand/MirRegisterClass.h");
    emitter.emitInclude(std::format("{}RegisterInfo.h", ns));
    emitter.emitBlankLine();

    const std::string registerNs = std::format("EzCodeEmitter::TableGen::{}", ns);

    {
        auto nsScope = emitter.enterNamespace(std::format("EzTriple::TableGen::{}", ns));
        emitter.emitBlankLine();

        emitter.emitLine("{}::{} (MirBuilderContext *ctx) :", className, className);
        emitter.indent();
        emitter.emitLine("m_ctx(ctx),");
        emitter.emitLine("m_banks(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource()),");
        emitter.emitLine("m_convs(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource()),");
        emitter.emitLine("m_binaries(ctx ? ctx->getGlobalAllocator() : std::pmr::get_default_resource())");
        emitter.dedent();
        {
            auto body = emitter.enterScope();
        }
        emitter.emitBlankLine();

        emitter.emitLine("{}::~{}() = default;", className, className);
        emitter.emitBlankLine();

        emitter.emitLine("void {}::initialize()", className);
        {
            auto s = emitter.enterScope();
            emitter.emitLine("if (!m_ctx)");
            {
                auto f = emitter.enterScope();
                emitter.emitLine("return;");
            }
            emitter.emitLine("m_banks = {}::initializeRegisterBanks(m_ctx->getGlobalAllocator());", registerNs);
        }
        emitter.emitBlankLine();

        emitter.emitLine("MirType *{}::getMemOperandDisplacementType()", className);
        {
            auto s = emitter.enterScope();
            emitter.emitLine("return nullptr;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("MirRegisterRef {}::getInstructionPtrReg() const", className);
        {
            auto s = emitter.enterScope();
            if (decl && decl->mInstructionPointer.has_value())
            {
                // Resolve the declared instruction-pointer special register by name.
                emitter.emitLine("const uint32_t id = {}::getSpecialRegId(\"{}\");",
                                 registerNs,
                                 escapeString(decl->mInstructionPointer->m_node));
                emitter.emitLine("if (!m_banks.empty())");
                {
                    // Pick the widest register class in the first bank to hold the pointer.
                    auto bank = emitter.enterScope();
                    emitter.emitLine("MirRegisterClass *bestClass = nullptr;");
                    emitter.emitLine("std::size_t bestBits = 0;");
                    emitter.emitLine("for (const auto &kv : m_banks[0]->getClasses())");
                    {
                        auto loop = emitter.enterScope();
                        emitter.emitLine("for (const auto &reg : kv.second->getRegs())");
                        {
                            auto inner = emitter.enterScope();
                            emitter.emitLine("if (reg.second && reg.second->m_bitSize > bestBits)");
                            {
                                auto ifs = emitter.enterScope();
                                emitter.emitLine("bestBits = reg.second->m_bitSize;");
                                emitter.emitLine("bestClass = kv.second;");
                            }
                        }
                    }
                    emitter.emitLine("if (bestClass)");
                    {
                        auto ifs = emitter.enterScope();
                        emitter.emitLine("return MirRegisterRef(bestClass, id);");
                    }
                }
            }
            emitter.emitLine("return MirRegisterRef();");
        }
        emitter.emitBlankLine();

        emitter.emitLine("std::string_view {}::getLibcallStr(uint8_t symId)", className);
        {
            auto s = emitter.enterScope();
            if (decl && !decl->mLibcalls.empty())
            {
                emitter.emitLine("switch (symId)");
                {
                    auto sw = emitter.enterScope();
                    for (size_t i = 0; i < decl->mLibcalls.size(); ++i)
                    {
                        emitter.emitLine("case {}: return \"{}\";",
                                         i,
                                         escapeString(decl->mLibcalls[i].m_symbol.m_node));
                    }
                    emitter.emitLine("default: return {};");
                }
            }
            emitter.emitLine("return {};");
        }
        emitter.emitBlankLine();

        emitter.emitLine("MirRegisterBank *{}::createRegisterBank(const char *name)", className);
        {
            auto s = emitter.enterScope();
            emitter.emitLine("if (!m_ctx || !name)");
            {
                auto f = emitter.enterScope();
                emitter.emitLine("return nullptr;");
            }
            emitter.emitLine("auto *alloc = m_ctx->getGlobalAllocator();");
            emitter.emitLine("std::pmr::polymorphic_allocator<MirRegisterBank> bankAlloc(alloc);");
            emitter.emitLine("auto *bank = bankAlloc.new_object<MirRegisterBank>(name, alloc);");
            emitter.emitLine("m_banks.push_back(bank);");
            emitter.emitLine("return bank;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("GenericCodeEmitter *{}::createCodeEmitter()", className);
        {
            auto s = emitter.enterScope();
            emitter.emitComment("The target-specific table-driven emitter is wired once the encoding table lands.");
            emitter.emitLine("return nullptr;");
        }
    }
}

// Requires a .tdesc manifest, then emits and writes the descriptor header/source pair.
bool CppTargetDescGenerator::run()
{
    if (!beginGeneration())
    {
        return false;
    }

    // Without a target descriptor symbol there is nothing meaningful to generate.
    const auto *decl = findTargetDesc(getSymbolTable());
    if (!decl)
    {
        error("No target descriptor symbol found in symbol table.");
        return false;
    }

    const std::string ns = SanitizeCppIdentifier(m_targetName, "Target");
    auto [headerPath, sourcePath] = resolveHeaderAndSourcePaths(std::format("{}TargetDesc", ns));

    CppSourceEmitter headerEmitter;
    CppSourceEmitter sourceEmitter;

    emitHeader(headerEmitter, decl);
    emitSource(sourceEmitter, decl);

    if (!writeHeaderAndSource({ headerPath, sourcePath }, headerEmitter.view(), sourceEmitter.view()))
    {
        return false;
    }

    trace("Synthesized target descriptor {} and {}", headerPath.filename().string(), sourcePath.filename().string());
    return true;
}

// Convenience wrapper retained for callers that do not need to configure a generator object.
bool GenerateTargetDescriptor(DiagnosticCollector *collector,
                              SymbolTable *table,
                              std::filesystem::path outPath,
                              std::string targetName)
{
    CppTargetDescGenerator generator(collector, table, std::move(outPath), std::move(targetName));
    return generator.run();
}

} // namespace CodeGenerators
