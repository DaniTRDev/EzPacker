#include "CodeGenerators/CppMirTypeTableGenerator.h"
#include "Diagnostics/DiagnosticCollector.h"
#include "Sema/Symbol.h"
#include "Sema/SymbolTable.h"

namespace CodeGenerators
{

namespace
{

// Maps a DSL type kind to the generated MirTypeKind enumerator.
std::string KindToEnumString(DSL::Ast::TypeDef::TypeKind kind)
{
    switch (kind)
    {
        case DSL::Ast::TypeDef::TypeKind::Void:
            return "MirTypeKind::Void";
        case DSL::Ast::TypeDef::TypeKind::Integer:
            return "MirTypeKind::Integer";
        case DSL::Ast::TypeDef::TypeKind::FloatingPoint:
            return "MirTypeKind::FloatingPoint";
        case DSL::Ast::TypeDef::TypeKind::BindingToken:
            return "MirTypeKind::BindingToken";
        case DSL::Ast::TypeDef::TypeKind::Pointer:
            return "MirTypeKind::Pointer";
    }
    return "MirTypeKind::Integer";
}

} // namespace

// Binds the generator to its diagnostics/symbols and records which artifacts may be emitted.
CppMirTypeTableGenerator::CppMirTypeTableGenerator(DiagnosticCollector *collector,
                                                   SymbolTable *table,
                                                   std::filesystem::path outPath,
                                                   MirTypeTableGenWorkingMode mode) :
    CodeGenerator("CodeGenerators::MirTypeTable", collector, table, std::move(outPath)), m_mode(mode)
{
}

// Flattens every type symbol into a TypeEntry with derived field/getter names and compact id.
std::vector<CppMirTypeTableGenerator::TypeEntry> CppMirTypeTableGenerator::collectTypes() const
{
    std::vector<TypeEntry> collectedTypes;
    if (!m_table)
    {
        return collectedTypes;
    }

    for (const Symbol *sym : m_table->collect<Symbols::TypeSymbol>(SymbolType::Type))
    {
        const auto *data = sym->getIf<Symbols::TypeSymbol>();
        if (!data)
        {
            continue;
        }

        std::string typeName(sym->getName());

        collectedTypes.push_back(TypeEntry{ .m_name = typeName,
                                            .m_bitWidth = data->m_bitWidth,
                                            .m_alignment = data->m_alignment,
                                            .m_kind = data->m_kind,
                                            .m_fieldName = std::format("m_{}Type", typeName),
                                            .m_getterName = typeName,
                                            .m_compactId = data->m_compactId });
    }

    return collectedTypes;
}

// Emits the generated MirTypeTable header: compact ids, accessors, fields and interning caches.
void CppMirTypeTableGenerator::emitHeader(CppSourceEmitter &emitter, const std::vector<TypeEntry> &types) const
{
    emitter.emitIncludeGuardStart("EZMIR_MIR_TYPE_TABLE_H");
    emitter.emitBlankLine();
    emitter.emitBanner("CppMirTypeTableGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude("EzCoreCommon.h");
    emitter.emitInclude("Type/MirType.h");
    emitter.emitInclude("array", true);
    emitter.emitInclude("list", true);
    emitter.emitInclude("memory_resource", true);
    emitter.emitInclude("string", true);
    emitter.emitInclude("string_view", true);
    emitter.emitInclude("unordered_map", true);
    emitter.emitInclude("vector", true);
    emitter.emitBlankLine();

    // Pointer types are interned dynamically, so only non-pointer primitives get compact ids.
    size_t primitiveCount = 0;
    for (const auto &type : types)
    {
        if (type.m_kind != DSL::Ast::TypeDef::TypeKind::Pointer)
        {
            ++primitiveCount;
        }
    }

    emitter.emitDocComment("Compact machine type identifiers for dense LUT indexing during legalization / ISel.");
    {
        auto enumScope = emitter.enterEnum("MirTypeCompactId", "uint8_t", true);
        emitter.emitLine("Invalid = 0,");

        for (const auto &type : types)
        {
            if (type.m_kind == DSL::Ast::TypeDef::TypeKind::Pointer)
            {
                continue;
            }
            emitter.emitLine("{} = {},", type.m_name, type.m_compactId);
        }

        emitter.emitLine("Custom = 255");
    }

    emitter.emitBlankLine();
    emitter.emitDocComment("Total number of machine-defined primitive types.");
    emitter.emitLine("constexpr size_t MachineTypeCount = {};", primitiveCount);
    emitter.emitBlankLine();

    emitter.emitDocComment("Class used to store the least minimum required types so everything else works.");
    {
        auto classScope = emitter.enterClass("MirTypeTable");
        emitter.emitLine("public:");
        emitter.emitLine("static constexpr uint8_t InvalidCompactId = 0;");
        emitter.emitLine("static constexpr uint8_t CustomCompactId = 255;");
        emitter.emitLine("static constexpr size_t MachineDefinedTypeCount = {};", primitiveCount);
        emitter.emitBlankLine();

        emitter.emitDocComment("Creates the type table with the given arena allocator.");
        emitter.emitLine("explicit MirTypeTable(std::pmr::memory_resource *globalArena, size_t pointerBitWidth = 64);");
        emitter.emitBlankLine();

        emitter.emitComment("Disable copies to safeguard our arena resource mappings");
        emitter.emitLine("MirTypeTable(const MirTypeTable &) = delete;");
        emitter.emitLine("MirTypeTable &operator=(const MirTypeTable &) = delete;");
        emitter.emitBlankLine();

        emitter.emitDocComment("Creates a unique base or compound type.");
        emitter.emitLine("MirType *create(MirTypeKind kind,");
        emitter.emitLine("                size_t totalSizeInBits,");
        emitter.emitLine("                size_t totalAlignmentInBits,");
        emitter.emitLine("                std::pmr::vector<MirType *> subTypes,");
        emitter.emitLine("                const std::string_view &name,");
        emitter.emitLine("                uint8_t compactId = CustomCompactId);");
        emitter.emitBlankLine();

        emitter.emitDocComment(
                "Creates a function type with the given parameters. Returns existing if previously created.");
        emitter.emitLine("MirType *getFuncType(MirType *returnType,");
        emitter.emitLine("                     const std::pmr::list<class MirRegister *> &parameters,");
        emitter.emitLine("                     const std::string_view &funcName);");
        emitter.emitBlankLine();

        emitter.emitDocComment("Interns pointer types. Guarantees that getPtr(T) always returns the exact same type "
                               "instance pointer.");
        emitter.emitLine("MirType *getPtr(MirType *srcType);");
        emitter.emitBlankLine();

        emitter.emitDocComment("Interns array types structurally based on size and base element composition.");
        emitter.emitLine("MirType *getArray(MirType *elementType, size_t elementCount);");
        emitter.emitBlankLine();

        emitter.emitDocComment("Returns the first floating-point type that can hold the given bit size.");
        emitter.emitLine("MirType *getFloatingTypeBySize(size_t sizeInBits) const;");
        emitter.emitBlankLine();

        emitter.emitDocComment("Returns the first integer type that can hold the given bit size.");
        emitter.emitLine("MirType *getIntegerTypeBySize(size_t sizeInBits) const;");
        emitter.emitBlankLine();

        emitter.emitDocComment(
                "Searches the table for the given ID and returns its type, if any. Returns nullptr if not created.");
        emitter.emitLine("MirType *getMirTypeById(size_t id) const;");
        emitter.emitBlankLine();

        emitter.emitDocComment(
                "Fast O(1) array lookup using dense compact machine type IDs for legalization and ISel tables.");
        emitter.emitLine("MirType *getTypeByCompactId(uint8_t compactId) const;");
        emitter.emitBlankLine();

        emitter.emitDocComment("Returns the target pointer width in bits.");
        emitter.emitLine("size_t getPointerBitWidth() const { return m_pointerBitWidth; }");
        emitter.emitBlankLine();

        emitter.emitDocComment("Returns the total number of unique types instantiated in this table.");
        emitter.emitLine("size_t getTypeCount() const;");
        emitter.emitBlankLine();

        emitter.emitComment("--- Built-in & Generated Type Accessors ---");
        for (const auto &type : types)
        {
            if (type.m_kind == DSL::Ast::TypeDef::TypeKind::Pointer)
            {
                continue;
            }
            emitter.emitLine("MirType *{}();", type.m_getterName);
        }
        emitter.emitBlankLine();

        emitter.emitDocComment(
                "Initializes the type table with the basic primitive types using the given pointer size.");
        emitter.emitLine("void initialize(size_t pointerBitWidth);");
        emitter.emitBlankLine();

        emitter.emitLine("private:");
        emitter.emitLine("size_t m_currentId{ 0 };");
        emitter.emitLine("size_t m_pointerBitWidth{ 64 };");
        emitter.emitBlankLine();

        emitter.emitComment("Built-in & Generated Primitives");
        for (const auto &type : types)
        {
            if (type.m_kind == DSL::Ast::TypeDef::TypeKind::Pointer)
            {
                continue;
            }
            emitter.emitLine("MirType *{}{{ nullptr }};", type.m_fieldName);
        }
        emitter.emitBlankLine();

        emitter.emitLine("std::pmr::memory_resource *m_arena;");
        emitter.emitBlankLine();

        emitter.emitComment("Dense lookup cache mapping compactId to primitive MirType instance");
        emitter.emitLine("std::array<MirType *, 256> m_compactIdToType{};");
        emitter.emitBlankLine();

        emitter.emitComment("High performance tracking hashes using PMR mapping blocks");
        emitter.emitLine("std::pmr::unordered_map<std::pmr::string, MirType *> m_typeNames;");
        emitter.emitLine("std::pmr::unordered_map<size_t, MirType *> m_idToType;");
        emitter.emitBlankLine();

        emitter.emitComment("Interning caches: maps base type to its unique pointer type representation");
        emitter.emitLine("std::pmr::unordered_map<MirType *, MirType *> m_pointerCache;");
    }

    emitter.emitBlankLine();
    emitter.emitIncludeGuardEnd("EZMIR_MIR_TYPE_TABLE_H");
}

// Emits the generated MirTypeTable method definitions and per-type accessor bodies.
void CppMirTypeTableGenerator::emitSource(CppSourceEmitter &emitter, const std::vector<TypeEntry> &types) const
{
    emitter.emitBanner("CppMirTypeTableGenerator");
    emitter.emitBlankLine();

    emitter.emitInclude("Type/MirTypeTable.h");
    emitter.emitInclude("Operand/MirOperands.h");
    emitter.emitBlankLine();

    emitter.emitLine("MirTypeTable::MirTypeTable(std::pmr::memory_resource *globalArena, size_t pointerBitWidth) :");
    emitter.indent();
    emitter.emitLine("m_pointerBitWidth(pointerBitWidth),");
    emitter.emitLine("m_arena(globalArena),");
    emitter.emitLine("m_typeNames(globalArena),");
    emitter.emitLine("m_idToType(globalArena),");
    emitter.emitLine("m_pointerCache(globalArena)");
    emitter.dedent();
    {
        auto ctorScope = emitter.enterBlock();
        emitter.emitLine("m_compactIdToType.fill(nullptr);");
    }
    emitter.emitBlankLine();

    // Interns a type by name, reusing any previously created instance with the same name.
    emitter.emitLine("MirType *MirTypeTable::create(MirTypeKind kind,");
    emitter.emitLine("                              size_t totalSizeInBits,");
    emitter.emitLine("                              size_t totalAlignmentInBits,");
    emitter.emitLine("                              std::pmr::vector<MirType *> subTypes,");
    emitter.emitLine("                              const std::string_view &name,");
    emitter.emitLine("                              uint8_t compactId)");
    {
        auto fnScope = emitter.enterBlock();
        emitter.emitLine("if (name.empty())");
        {
            auto ifScope = emitter.enterBlock();
            emitter.emitLine("return nullptr;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("std::pmr::string lookupName(name, m_arena);");
        emitter.emitLine("auto it = m_typeNames.find(lookupName);");
        emitter.emitLine("if (it != m_typeNames.end())");
        {
            auto ifScope = emitter.enterBlock();
            emitter.emitLine("return it->second;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("size_t assignedId = ++m_currentId;");
        emitter.emitBlankLine();
        emitter.emitLine("std::pmr::polymorphic_allocator<MirType> alloc(m_arena);");
        emitter.emitLine("MirType *uniqueType = alloc.new_object<MirType>(kind,");
        emitter.emitLine("                                                this,");
        emitter.emitLine("                                                assignedId,");
        emitter.emitLine("                                                totalAlignmentInBits,");
        emitter.emitLine("                                                totalSizeInBits,");
        emitter.emitLine("                                                lookupName,");
        emitter.emitLine("                                                std::move(subTypes),");
        emitter.emitLine("                                                compactId);");
        emitter.emitBlankLine();

        emitter.emitLine("m_typeNames[lookupName] = uniqueType;");
        emitter.emitLine("m_idToType[assignedId] = uniqueType;");
        emitter.emitBlankLine();

        emitter.emitLine("if (compactId != CustomCompactId && compactId != InvalidCompactId)");
        {
            auto ifScope = emitter.enterBlock();
            emitter.emitLine("m_compactIdToType[compactId] = uniqueType;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("return uniqueType;");
    }
    emitter.emitBlankLine();

    // Function types are interned on a structural signature built from return and parameter names.
    emitter.emitLine("MirType *MirTypeTable::getFuncType(MirType *returnType,");
    emitter.emitLine("                                   const std::pmr::list<MirRegister *> &parameters,");
    emitter.emitLine("                                   const std::string_view &funcName)");
    {
        auto fnScope = emitter.enterBlock();
        emitter.emitLine("if (!returnType)");
        {
            auto ifScope = emitter.enterBlock();
            emitter.emitLine("return nullptr;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("auto structuralSignature = returnType->getName();");
        emitter.emitLine("structuralSignature += \"(\";");
        emitter.emitBlankLine();

        emitter.emitLine("std::pmr::vector<MirType *> subTypes(m_arena);");
        emitter.emitLine("subTypes.reserve(parameters.size() + 1);");
        emitter.emitLine("subTypes.push_back(returnType);");
        emitter.emitBlankLine();

        emitter.emitLine("for (auto &param : parameters)");
        {
            auto forScope = emitter.enterBlock();
            emitter.emitLine("if (!param || !param->getMirType())");
            {
                auto ifScope = emitter.enterBlock();
                emitter.emitLine("continue;");
            }
            emitter.emitLine("MirType *paramType = param->getMirType();");
            emitter.emitLine("subTypes.push_back(paramType);");
            emitter.emitBlankLine();
            emitter.emitLine("if (subTypes.size() > 2)");
            {
                auto ifScope = emitter.enterBlock();
                emitter.emitLine("structuralSignature += \",\";");
            }
            emitter.emitLine("structuralSignature += paramType->getName();");
        }
        emitter.emitLine("structuralSignature += \")\";");
        emitter.emitBlankLine();

        emitter.emitLine("std::pmr::string lookupKey(structuralSignature, m_arena);");
        emitter.emitLine("auto it = m_typeNames.find(lookupKey);");
        emitter.emitLine("if (it != m_typeNames.end())");
        {
            auto ifScope = emitter.enterBlock();
            emitter.emitLine("return it->second;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("return create(MirTypeKind::Function, m_pointerBitWidth, m_pointerBitWidth, "
                         "std::move(subTypes), lookupKey, CustomCompactId);");
    }
    emitter.emitBlankLine();

    // Pointer types are cached one-to-one per source type so the returned pointer is stable.
    emitter.emitLine("MirType *MirTypeTable::getPtr(MirType *srcType)");
    {
        auto fnScope = emitter.enterBlock();
        emitter.emitLine("if (!srcType)");
        {
            auto ifScope = emitter.enterBlock();
            emitter.emitLine("return nullptr;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("auto it = m_pointerCache.find(srcType);");
        emitter.emitLine("if (it != m_pointerCache.end())");
        {
            auto ifScope = emitter.enterBlock();
            emitter.emitLine("return it->second;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("std::pmr::vector<MirType *> childTarget({ srcType }, m_arena);");
        emitter.emitLine("std::string formattedName = std::format(\"{}*\", srcType->getName());");
        emitter.emitBlankLine();

        emitter.emitLine("MirType *newPointerType =");
        emitter.indent();
        emitter.emitLine("create(MirTypeKind::Pointer, m_pointerBitWidth, m_pointerBitWidth, std::move(childTarget), "
                         "formattedName, CustomCompactId);");
        emitter.dedent();
        emitter.emitBlankLine();

        emitter.emitLine("m_pointerCache[srcType] = newPointerType;");
        emitter.emitLine("return newPointerType;");
    }
    emitter.emitBlankLine();

    // Arrays are interned by a name of the form Element[N].
    emitter.emitLine("MirType *MirTypeTable::getArray(MirType *elementType, size_t elementCount)");
    {
        auto fnScope = emitter.enterBlock();
        emitter.emitLine("if (!elementType)");
        {
            auto ifScope = emitter.enterBlock();
            emitter.emitLine("return nullptr;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("std::string arraySignature = std::format(\"{}[{}]\", elementType->getName(), elementCount);");
        emitter.emitLine("std::pmr::string lookupName(arraySignature, m_arena);");
        emitter.emitBlankLine();

        emitter.emitLine("auto it = m_typeNames.find(lookupName);");
        emitter.emitLine("if (it != m_typeNames.end())");
        {
            auto ifScope = emitter.enterBlock();
            emitter.emitLine("return it->second;");
        }
        emitter.emitBlankLine();

        emitter.emitLine("size_t totalSizeInBits = elementType->getTotalSizeInBits() * elementCount;");
        emitter.emitLine("size_t alignmentInBits = elementType->getMaxAlignmentInBits();");
        emitter.emitLine("std::pmr::vector<MirType *> childType({ elementType }, m_arena);");
        emitter.emitBlankLine();

        emitter.emitLine("return create(MirTypeKind::Array, totalSizeInBits, alignmentInBits, std::move(childType), "
                         "lookupName, CustomCompactId);");
    }
    emitter.emitBlankLine();

    // Scans all types for the smallest floating type whose width is at least sizeInBits.
    emitter.emitLine("MirType *MirTypeTable::getFloatingTypeBySize(size_t sizeInBits) const");
    {
        auto fnScope = emitter.enterBlock();
        emitter.emitLine("MirType *result = nullptr;");
        emitter.emitLine("for (auto &[id, type] : m_idToType)");
        {
            auto forScope = emitter.enterBlock();
            emitter.emitLine("if (type->getKind() != MirTypeKind::FloatingPoint)");
            emitter.indent();
            emitter.emitLine("continue;");
            emitter.dedent();
            emitter.emitBlankLine();

            emitter.emitLine("size_t typeSize = type->getTotalSizeInBits();");
            emitter.emitLine("if (typeSize >= sizeInBits)");
            {
                auto ifScope = emitter.enterBlock();
                emitter.emitLine("if (!result || typeSize < result->getTotalSizeInBits())");
                {
                    auto innerScope = emitter.enterBlock();
                    emitter.emitLine("result = type;");
                }
            }
        }
        emitter.emitLine("return result;");
    }
    emitter.emitBlankLine();

    // Scans all types for the smallest integer type whose width is at least sizeInBits.
    emitter.emitLine("MirType *MirTypeTable::getIntegerTypeBySize(size_t sizeInBits) const");
    {
        auto fnScope = emitter.enterBlock();
        emitter.emitLine("MirType *result = nullptr;");
        emitter.emitLine("for (auto &[id, type] : m_idToType)");
        {
            auto forScope = emitter.enterBlock();
            emitter.emitLine("if (type->getKind() != MirTypeKind::Integer)");
            emitter.indent();
            emitter.emitLine("continue;");
            emitter.dedent();
            emitter.emitBlankLine();

            emitter.emitLine("size_t typeSize = type->getTotalSizeInBits();");
            emitter.emitLine("if (typeSize >= sizeInBits)");
            {
                auto ifScope = emitter.enterBlock();
                emitter.emitLine("if (!result || typeSize < result->getTotalSizeInBits())");
                {
                    auto innerScope = emitter.enterBlock();
                    emitter.emitLine("result = type;");
                }
            }
        }
        emitter.emitLine("return result;");
    }
    emitter.emitBlankLine();

    emitter.emitLine("MirType *MirTypeTable::getMirTypeById(size_t id) const");
    {
        auto fnScope = emitter.enterBlock();
        emitter.emitLine("auto it = m_idToType.find(id);");
        emitter.emitLine("return (it != m_idToType.end()) ? it->second : nullptr;");
    }
    emitter.emitBlankLine();

    emitter.emitLine("MirType *MirTypeTable::getTypeByCompactId(uint8_t compactId) const");
    {
        auto fnScope = emitter.enterBlock();
        emitter.emitLine("return m_compactIdToType[compactId];");
    }
    emitter.emitBlankLine();

    emitter.emitLine("size_t MirTypeTable::getTypeCount() const");
    {
        auto fnScope = emitter.enterBlock();
        emitter.emitLine("return m_idToType.size();");
    }
    emitter.emitBlankLine();

    for (const auto &type : types)
    {
        if (type.m_kind == DSL::Ast::TypeDef::TypeKind::Pointer)
        {
            continue;
        }
        emitter.emitLine("MirType *MirTypeTable::{}() {{ return {}; }}", type.m_getterName, type.m_fieldName);
    }
    emitter.emitBlankLine();

    // Materializes the declared primitive types into the table's fields.
    emitter.emitLine("void MirTypeTable::initialize(size_t pointerBitWidth)");
    {
        auto fnScope = emitter.enterBlock();
        emitter.emitLine("m_pointerBitWidth = pointerBitWidth;");
        for (const auto &type : types)
        {
            if (type.m_kind == DSL::Ast::TypeDef::TypeKind::Pointer)
            {
                continue;
            }
            emitter.emitLine("{} = create({}, {}, {}, {{}}, \"{}\", {});",
                             type.m_fieldName,
                             KindToEnumString(type.m_kind),
                             type.m_bitWidth,
                             type.m_alignment,
                             type.m_name,
                             static_cast<uint32_t>(type.m_compactId));
        }
    }
}

// Collects the type definitions and emits only the artifacts enabled by the working mode.
bool CppMirTypeTableGenerator::run()
{
    if (!validate())
    {
        return false;
    }

    trace("Generating MirTypeTable in {}", m_outputPath.string());

    auto collectedTypes = collectTypes();
    auto paths = resolveHeaderAndSourcePaths("MirTypeTable");

    // The mode bitmask decides independently whether the header and/or source are written.
    if (m_mode & MirTypeTableGenWorkingMode::Header)
    {
        CppSourceEmitter headerEmitter;
        emitHeader(headerEmitter, collectedTypes);
        if (!writeOutput(paths.m_headerPath, headerEmitter.view()))
        {
            return false;
        }
    }

    if (m_mode & MirTypeTableGenWorkingMode::Source)
    {
        CppSourceEmitter sourceEmitter;
        emitSource(sourceEmitter, collectedTypes);
        if (!writeOutput(paths.m_sourcePath, sourceEmitter.view()))
        {
            return false;
        }
    }

    trace("Successfully generated {} type definitions", collectedTypes.size());
    return true;
}

// Convenience wrapper retained for callers that do not need to configure a generator object.
bool GenerateMirTypeTable(DiagnosticCollector *collector,
                          SymbolTable *table,
                          std::filesystem::path outPath,
                          MirTypeTableGenWorkingMode mode)
{
    CppMirTypeTableGenerator generator(collector, table, std::move(outPath), mode);
    return generator.run();
}

} // namespace CodeGenerators