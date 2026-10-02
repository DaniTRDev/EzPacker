#include "CodeGenerators/CppTargetDescGenerator.h"
#include "Ast/TargetDescDefLangAst.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"
#include "Sema/Symbols/TargetDescSymbols.h"
#include "StringUtils.h"

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
                                               std::string targetName,
                                               std::string namespaceRoot) :
    CodeGenerator("CodeGenerators::TargetDesc", collector, table, std::move(outPath)),
    m_targetName(SanitizeCppIdentifier(targetName, "Target")), m_namespaceRoot(std::move(namespaceRoot))
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
    emitter.emitInclude("memory", true);
    emitter.emitInclude("memory_resource", true);
    emitter.emitInclude("string_view", true);
    emitter.emitInclude("vector", true);
    emitter.emitBlankLine();

    emitter.emitInclude("Descriptors/TargetDesc.h");
    emitter.emitInclude("Libcall/TargetLibcallRegistry.h");
    emitter.emitInclude("Operand/MirRegisterReference.h");
    emitter.emitBlankLine();

    emitter.emitLine("class GenericCodeEmitter;");
    emitter.emitLine("class MirBuilderContext;");
    emitter.emitLine("class MirFrameLowerer;");
    emitter.emitLine("class MirInstructionSelector;");
    emitter.emitLine("class MirRegisterClass;");
    emitter.emitLine("class MirRegisterBank;");
    emitter.emitLine("class MirLegalizer;");
    emitter.emitLine("class LegalizerInfo;");
    emitter.emitLine("class MirRegisterAllocator;");
    emitter.emitLine("class MirType;");
    emitter.emitLine("class CallingConvDesc;");
    emitter.emitLine("class TargetBinaryDesc;");
    emitter.emitLine("class TargetRelocationResolver;");
    emitter.emitBlankLine();

    {
        auto nsScope = emitter.enterNamespace(std::format("{}::TableGen::{}", m_namespaceRoot, ns));
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

        emitter.emitLine("inline constexpr const char *s_targetExtensions[] =");
        {
            auto s = emitter.enterScope("{", "};");
            if (!decl || decl->m_extensions.empty())
            {
                emitter.emitLine("\"\",");
            }
            else
            {
                for (const auto &ext : decl->m_extensions)
                {
                    emitter.emitLine("\"{}\",", escapeString(ext.m_name.m_node));
                }
            }
        }
        emitter.emitLine("inline constexpr std::size_t s_targetExtensionCount = {};",
                         decl ? decl->m_extensions.size() : 0);
        emitter.emitBlankLine();

        emitter.emitComment("Concrete target descriptor generated from the .tdesc manifest.");
        {
            auto classScope = emitter.enterClass(className, "public TargetDesc");
            emitter.emitLine("public:");
            emitter.indent();
            emitter.emitLine("explicit {}(MirBuilderContext *ctx);", className);
            emitter.emitLine("~{}() override;", className);
            emitter.emitBlankLine();
            std::string targetNameStr = StrToLower(decl ? std::string(decl->m_name.m_node) : m_targetName);
            emitter.emitLine("const char *getName() const override {{ return \"{}\"; }}",
                             escapeString(targetNameStr));
            emitter.emitLine("MirFrameLowerer *getFrameLowerer() override;");
            emitter.emitLine("MirInstructionSelector *getInstructionSelector() override;");
            emitter.emitLine("MirRegisterClass *getGprClass() override;");
            emitter.emitLine("MirRegisterClass *getVr128Class() const { return m_vr128; }");
            emitter.emitLine("MirRegisterClass *getVr256Class() const { return m_vr256; }");
            emitter.emitLine("MirLegalizer *getLegalizer() override;");
            emitter.emitLine("LegalizerInfo *getLegalizerInfo() override;");
            emitter.emitLine("MirRegisterAllocator *getRegisterAllocator() override;");
            emitter.emitLine("MirType *getMemOperandDisplacementType() override;");
            emitter.emitLine("MirRegisterRef getInstructionPtrReg() const override;");
            emitter.emitLine("size_t getStackSlotSize() const override {{ return {}; }}",
                             decl && decl->m_stackSlot.has_value() ? decl->m_stackSlot->m_node : 8);
            emitter.emitLine("void initialize() override;");
            emitter.emitLine("std::string_view getLibcallStr(uint8_t symId) override;");
            emitter.emitLine("TargetLibcallRegistry *getLibcallRegistry() override {{ return &m_libcallRegistry; }}");
            emitter.emitLine("const TargetLibcallRegistry *getLibcallRegistry() const override {{ return &m_libcallRegistry; }}");
            emitter.emitLine("const std::pmr::vector<TargetBinaryDesc *> &getAvailableBinaryDescriptors() override;");
            emitter.emitLine("const std::pmr::vector<CallingConvDesc *> &getAvailableCallingConventions() override;");
            emitter.emitLine("const std::pmr::vector<MirRegisterBank *> &getAvailableRegisterBanks() override;");
            emitter.emitLine("MirRegisterBank *createRegisterBank(const char *name) override;");
            emitter.emitLine("std::unique_ptr<GenericCodeEmitter> createCodeEmitter() override;");
            emitter.emitLine("TargetRelocationResolver *getRelocationResolver() override;");
            emitter.emitBlankLine();
            emitter.emitComment("Convenience accessors for target-specific conventions, formats and settings.");
            emitter.emitLine("void setPositionIndependent(bool isPositionIndependent) { m_isPic = isPositionIndependent; }");
            emitter.emitLine("CallingConvDesc *getSysVCallingConv() const { return m_sysVConv.get(); }");
            emitter.emitLine("CallingConvDesc *getWin64CallingConv() const { return m_win64Conv.get(); }");
            emitter.emitLine("TargetBinaryDesc *getElfBinaryDesc() const { return m_elfBinary.get(); }");
            emitter.emitLine("TargetBinaryDesc *getCoffBinaryDesc() const { return m_coffBinary.get(); }");
            emitter.emitBlankLine();
            emitter.emitLine("private:");
            emitter.emitLine("MirBuilderContext *m_ctx{ nullptr };");
            emitter.emitLine("bool m_initialized{ false };");
            emitter.emitLine("bool m_isPic{ false };");
            emitter.emitBlankLine();
            emitter.emitLine("MirRegisterBank *m_gprBank{ nullptr };");
            emitter.emitLine("MirRegisterBank *m_fprBank{ nullptr };");
            emitter.emitLine("MirRegisterClass *m_gpr64{ nullptr };");
            emitter.emitLine("MirRegisterClass *m_gpr32{ nullptr };");
            emitter.emitLine("MirRegisterClass *m_gpr16{ nullptr };");
            emitter.emitLine("MirRegisterClass *m_gpr8{ nullptr };");
            emitter.emitLine("MirRegisterClass *m_fpr64{ nullptr };");
            emitter.emitLine("MirRegisterClass *m_fpr32{ nullptr };");
            emitter.emitLine("MirRegisterClass *m_vr128{ nullptr };");
            emitter.emitLine("MirRegisterClass *m_vr256{ nullptr };");
            emitter.emitBlankLine();
            emitter.emitLine("std::unique_ptr<MirFrameLowerer> m_frameLowerer;");
            emitter.emitLine("std::unique_ptr<MirInstructionSelector> m_isel;");
            emitter.emitLine("std::unique_ptr<MirLegalizer> m_legalizer;");
            emitter.emitLine("std::unique_ptr<LegalizerInfo> m_legalizerInfo;");
            emitter.emitLine("std::unique_ptr<MirRegisterAllocator> m_regAlloc;");
            emitter.emitLine("std::unique_ptr<CallingConvDesc> m_sysVConv;");
            emitter.emitLine("std::unique_ptr<CallingConvDesc> m_win64Conv;");
            emitter.emitLine("std::unique_ptr<TargetBinaryDesc> m_elfBinary;");
            emitter.emitLine("std::unique_ptr<TargetBinaryDesc> m_coffBinary;");
            emitter.emitLine("std::unique_ptr<TargetRelocationResolver> m_relocResolver;");
            emitter.emitLine("TargetLibcallRegistry m_libcallRegistry;");
            emitter.emitBlankLine();
            emitter.emitLine("std::pmr::vector<MirRegisterBank *> m_banks;");
            emitter.emitLine("std::pmr::vector<CallingConvDesc *> m_convs;");
            emitter.emitLine("std::pmr::vector<TargetBinaryDesc *> m_binaries;");
            emitter.dedent();
        }
    }

    std::string primaryNs = m_namespaceRoot;
    if (!primaryNs.ends_with(ns))
    {
        primaryNs = std::format("{}::{}", m_namespaceRoot, ns);
    }
    emitter.emitBlankLine();
    emitter.emitComment("Compatibility alias in primary target namespace.");
    {
        auto nsScope = emitter.enterNamespace(primaryNs);
        emitter.emitLine("using {} = {}::TableGen::{}::{};", className, m_namespaceRoot, ns, className);
    }
}

// Emits the out-of-line TargetDesc method definitions and component wiring.
void CppTargetDescGenerator::emitSource(CppSourceEmitter &emitter,
                                        const DSL::Ast::TargetDesc::TargetDescDecl *decl) const
{
    const std::string ns = SanitizeCppIdentifier(m_targetName, "Target");
    const std::string className = std::format("{}TargetDesc", ns);
    std::string targetLower = ns;
    for (char &c : targetLower)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    emitter.emitBanner("CppTargetDescGenerator");
    emitter.emitBlankLine();
    emitter.emitInclude(std::format("{}TargetDesc.h", ns));
    emitter.emitInclude("Builder/MirBuilderContext.h");
    emitter.emitInclude("Type/MirTypeTable.h");
    emitter.emitInclude("Operand/MirRegisterBank.h");
    emitter.emitInclude("Operand/MirRegisterClass.h");
    emitter.emitInclude(std::format("{}RegisterInfo.h", ns));
    emitter.emitInclude("Descriptors/TargetRelocationResolver.h");
    emitter.emitInclude("Descriptors/TargetBinaryDesc.h");
    emitter.emitInclude("Function/CallingConvDesc.h");
    emitter.emitInclude("FrameLowerer/MirFrameLowerer.h");
    emitter.emitInclude("InstructionSelector/MirInstructionSelector.h");
    emitter.emitInclude("Legalizer/MirLegalizer.h");
    emitter.emitInclude("Legalizer/LegalizerInfo.h");
    emitter.emitInclude("RegisterAllocator/MirRegisterAllocator.h");
    emitter.emitInclude("GenericCodeEmitter.h");
    emitter.emitInclude("Instruction/MirTargetInstructionDesc.h");
    emitter.emitInclude(std::format("{}FrameLowerer.h", ns));
    emitter.emitInclude(std::format("{}RegisterAllocator.h", ns));
    emitter.emitInclude(std::format("{}ElfBinaryDesc.h", ns));
    emitter.emitInclude(std::format("{}CoffBinaryDesc.h", ns));
    emitter.emitInclude(std::format("{}CallingConvDesc.h", targetLower));
    emitter.emitInclude(std::format("{}TargetInstructionTable.h", targetLower));
    emitter.emitInclude(std::format("Encoding/{}EncodingDesc.h", ns));
    emitter.emitInclude(std::format("{}EncodingTable.h", targetLower));
    emitter.emitInclude(std::format("{}LegalizerActionTable.h", targetLower));
    emitter.emitInclude(std::format("{}TargetInstructionSelector.h", ns));
    emitter.emitInclude(std::format("{}RelocationResolver.h", ns));
    emitter.emitInclude(std::format("{}CodeEmitter.h", ns));
    emitter.emitBlankLine();

    const std::string registerNs = std::format("{}::TableGen::{}", m_namespaceRoot, ns);

    {
        auto nsScope = emitter.enterNamespace(std::format("{}::TableGen::{}", m_namespaceRoot, ns));
        emitter.emitBlankLine();
        emitter.emitLine("using namespace {}::{};", m_namespaceRoot, ns);
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
            if (decl && !decl->m_extensions.empty())
            {
                emitter.emitComment("Register target extensions defined in the .tdesc manifest.");
                for (const auto &ext : decl->m_extensions)
                {
                    const bool isDefault = ext.m_default.has_value() && ext.m_default->m_node;
                    std::string desc = ext.m_description.has_value() ? escapeString(ext.m_description->m_node) : "";

                    std::string impliesStr = "{";
                    for (size_t i = 0; i < ext.m_implies.size(); ++i)
                    {
                        if (i > 0)
                        {
                            impliesStr += ", ";
                        }
                        impliesStr += std::format("\"{}\"", escapeString(ext.m_implies[i].m_node));
                    }
                    impliesStr += "}";

                    emitter.emitLine("m_extensions.registerExtension(\"{}\", \"{}\", {}, {});",
                                     escapeString(ext.m_name.m_node),
                                     desc,
                                     isDefault ? "true" : "false",
                                     impliesStr);
                }
            }
        }
        emitter.emitBlankLine();

        emitter.emitLine("{}::~{}() = default;", className, className);
        emitter.emitBlankLine();

        emitter.emitLine("void {}::initialize()", className);
        {
            auto s = emitter.enterScope();
            emitter.emitLine("if (!m_ctx || m_initialized)");
            {
                auto f = emitter.enterScope();
                emitter.emitLine("return;");
            }
            emitter.emitLine("m_initialized = true;");
            emitter.emitBlankLine();
            emitter.emitLine("auto *alloc = m_ctx->getGlobalAllocator();");
            emitter.emitBlankLine();
            emitter.emitComment("1-4. Register Banks and Classes");
            emitter.emitLine("m_banks = {}::initializeRegisterBanks(m_ctx->getGlobalAllocator());", registerNs);
            emitter.emitLine("for (auto *b : m_banks)");
            {
                auto loop = emitter.enterScope();
                emitter.emitLine("if (std::string_view(b->getName()) == \"GPR\")");
                {
                    auto gprScope = emitter.enterScope();
                    emitter.emitLine("m_gprBank = b;");
                    emitter.emitLine("m_gpr64 = b->getClass(\"GPR64\");");
                    emitter.emitLine("m_gpr32 = b->getClass(\"GPR32\");");
                    emitter.emitLine("m_gpr16 = b->getClass(\"GPR16\");");
                    emitter.emitLine("m_gpr8 = b->getClass(\"GPR8\");");
                }
                emitter.emitLine("else if (std::string_view(b->getName()) == \"FPR\")");
                {
                    auto fprScope = emitter.enterScope();
                    emitter.emitLine("m_fprBank = b;");
                    emitter.emitLine("m_fpr64 = b->getClass(\"FPR64\");");
                    emitter.emitLine("m_fpr32 = b->getClass(\"FPR32\");");
                    emitter.emitLine("m_vr128 = b->getClass(\"VR128\");");
                    emitter.emitLine("m_vr256 = b->getClass(\"VR256\");");
                }
            }
            emitter.emitBlankLine();
            emitter.emitComment("5. Calling Conventions");
            emitter.emitLine("m_sysVConv = std::make_unique<SysV_AMD64CallingConvDesc>(m_ctx, m_gpr64, m_fpr64);");
            emitter.emitLine("m_win64Conv = std::make_unique<Win64CallingConvDesc>(m_ctx, m_gpr64, m_fpr64);");
            emitter.emitLine("m_convs.clear();");
            emitter.emitLine("m_convs.push_back(m_sysVConv.get());");
            emitter.emitLine("m_convs.push_back(m_win64Conv.get());");
            emitter.emitBlankLine();
            emitter.emitComment("6. Target Instruction Descriptors Table");
            emitter.emitLine("EzTargets::{}::{}TargetInst::initializeTargetInstructionTable(this);", ns, targetLower);
            emitter.emitBlankLine();
            emitter.emitComment("7. Legalizer Info & Legalizer");
            emitter.emitLine("m_legalizerInfo = std::make_unique<{}LegalizerInfo>();", targetLower);
            emitter.emitLine("m_legalizer = std::make_unique<MirLegalizer>(m_ctx, this);");
            emitter.emitBlankLine();
            emitter.emitComment("8. Instruction Selector");
            emitter.emitLine("m_isel = std::make_unique<{}TargetInstructionSelector>(this);", ns);
            emitter.emitBlankLine();
            emitter.emitComment("9. Register Allocator & Frame Lowerer");
            emitter.emitLine("m_regAlloc = std::make_unique<{}RegisterAllocator>();", ns);
            emitter.emitLine("m_frameLowerer = std::make_unique<{}FrameLowerer>();", ns);
            emitter.emitBlankLine();
            emitter.emitComment("10. Binary Descriptors");
            emitter.emitLine("m_elfBinary = std::make_unique<{}ElfBinaryDesc>(alloc, m_isPic);", ns);
            emitter.emitLine("m_elfBinary->initialize();");
            emitter.emitLine("m_coffBinary = std::make_unique<{}CoffBinaryDesc>(alloc);", ns);
            emitter.emitLine("m_coffBinary->initialize();");
            emitter.emitLine("m_binaries.clear();");
            emitter.emitLine("m_binaries.push_back(m_elfBinary.get());");
            emitter.emitLine("m_binaries.push_back(m_coffBinary.get());");
            emitter.emitBlankLine();
            emitter.emitComment("11. Libcall Registry Defaults");
            emitter.emitLine("m_libcallRegistry.initDefaults(\"{}\", \"linux\", CrtFlavor::Gnu);", targetLower);
            if (decl && !decl->mLibcalls.empty())
            {
                for (const auto &libcall : decl->mLibcalls)
                {
                    emitter.emitLine("if (auto kind = m_libcallRegistry.findKindByName(\"{}\"))", escapeString(libcall.m_name.m_node));
                    emitter.emitLine("    m_libcallRegistry.setLibcallName(*kind, \"{}\");", escapeString(libcall.m_symbol.m_node));
                }
            }
        }
        emitter.emitBlankLine();

        emitter.emitLine("MirFrameLowerer *{}::getFrameLowerer() {{ return m_frameLowerer.get(); }}", className);
        emitter.emitLine("MirInstructionSelector *{}::getInstructionSelector() {{ return m_isel.get(); }}", className);
        emitter.emitLine("MirRegisterClass *{}::getGprClass() {{ return m_gpr64; }}", className);
        emitter.emitLine("MirLegalizer *{}::getLegalizer() {{ return m_legalizer.get(); }}", className);
        emitter.emitLine("LegalizerInfo *{}::getLegalizerInfo() {{ return m_legalizerInfo.get(); }}", className);
        emitter.emitLine("MirRegisterAllocator *{}::getRegisterAllocator() {{ return m_regAlloc.get(); }}", className);
        emitter.emitBlankLine();

        emitter.emitLine("MirType *{}::getMemOperandDisplacementType()", className);
        {
            auto s = emitter.enterScope();
            if (decl && decl->mMemDispType.has_value() && decl->mMemDispType->m_node == "i64")
            {
                emitter.emitLine("return m_ctx ? m_ctx->getTypeTable()->i64() : nullptr;");
            }
            else
            {
                emitter.emitLine("return nullptr;");
            }
        }
        emitter.emitBlankLine();

        emitter.emitLine("MirRegisterRef {}::getInstructionPtrReg() const", className);
        {
            auto s = emitter.enterScope();
            if (decl && decl->mInstructionPointer.has_value())
            {
                emitter.emitLine("const uint32_t id = {}::getSpecialRegId(\"{}\");",
                                 registerNs,
                                 escapeString(decl->mInstructionPointer->m_node));
                emitter.emitLine("if (m_gpr64)");
                {
                    auto gprScope = emitter.enterScope();
                    emitter.emitLine("return MirRegisterRef(m_gpr64, id);");
                }
                emitter.emitLine("if (!m_banks.empty())");
                {
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
            emitter.emitLine("if (m_legalizerInfo)");
            {
                auto ifScope = emitter.enterScope();
                emitter.emitLine("return m_legalizerInfo->getLibcallSymbol(symId);");
            }
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

        emitter.emitLine("const std::pmr::vector<TargetBinaryDesc *> &{}::getAvailableBinaryDescriptors() {{ return m_binaries; }}", className);
        emitter.emitLine("const std::pmr::vector<CallingConvDesc *> &{}::getAvailableCallingConventions() {{ return m_convs; }}", className);
        emitter.emitLine("const std::pmr::vector<MirRegisterBank *> &{}::getAvailableRegisterBanks() {{ return m_banks; }}", className);
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

        emitter.emitLine("std::unique_ptr<GenericCodeEmitter> {}::createCodeEmitter()", className);
        {
            auto s = emitter.enterScope();
            emitter.emitLine("auto emitter = std::make_unique<EzTargets::{}::{}CodeEmitter>();", ns, ns);
            emitter.emitLine("emitter->setEncodingResolver(");
            emitter.indent();
            emitter.emitLine("[](const MirTargetInstructionDesc *desc) -> const EzTargets::{}::EncodingDesc *", ns);
            {
                auto resScope = emitter.enterScope();
                emitter.emitLine("if (!desc)");
                {
                    auto ifNull = emitter.enterScope();
                    emitter.emitLine("return nullptr;");
                }
                emitter.emitLine("if (const auto *enc = EzTargets::{}::getEncodingDesc(desc->getEncodingId()))", ns);
                {
                    auto ifEnc = emitter.enterScope();
                    emitter.emitLine("return enc;");
                }
                emitter.emitLine("return EzTargets::{}::findEncodingDesc(desc->getName());", ns);
            }
            emitter.emitLine(");");
            emitter.dedent();
            emitter.emitLine("return emitter;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("TargetRelocationResolver *{}::getRelocationResolver()", className);
        {
            auto s = emitter.enterScope();
            emitter.emitLine("if (!m_relocResolver)");
            {
                auto ifNull = emitter.enterScope();
                emitter.emitLine("m_relocResolver = std::make_unique<{}RelocationResolver>();", ns);
            }
            emitter.emitLine("return m_relocResolver.get();");
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
                              std::string targetName,
                              std::string namespaceRoot)
{
    CppTargetDescGenerator generator(
            collector, table, std::move(outPath), std::move(targetName), std::move(namespaceRoot));
    return generator.run();
}

} // namespace CodeGenerators
