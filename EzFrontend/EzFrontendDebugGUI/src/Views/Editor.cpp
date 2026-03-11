#include "Views/Editor.h"

Editor::Editor(std::shared_ptr<EzFrontendWrapper> frontend) : m_frontend(std::move(frontend))
{
    syncEditorBufferFromSource("void main() {\n    nop;\n}\n");
    m_statusMessage = "Ready. Open an .ez file, edit the buffer and compile to inspect the full frontend pipeline.";
}

const char *Editor::getName() { return "Editor"; }

void Editor::render()
{
    renderToolbar();
    ImGui::Separator();
    renderWorkspace();
}

bool Editor::promptOpenFile()
{
    OPENFILENAMEA ofn{};
    char fileName[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFilter = "EZ source (*.ez)\0*.ez\0All files (*.*)\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (!GetOpenFileNameA(&ofn))
    {
        return false;
    }

    if (!m_frontend->loadSourceFromFile(fileName))
    {
        m_statusMessage = std::format("Could not open '{}'", fileName);
        return false;
    }

    syncEditorBufferFromSource(m_frontend->getLoadedSourceText());
    m_dirty = false;
    m_statusMessage = std::format("Opened '{}'", m_frontend->getLoadedFilePath().string());
    compileCurrentBuffer();
    return true;
}

bool Editor::promptSaveFileAs()
{
    OPENFILENAMEA ofn{};
    char fileName[MAX_PATH] = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile = fileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "ez";
    ofn.lpstrFilter = "EZ source (*.ez)\0*.ez\0All files (*.*)\0*.*\0\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

    if (!GetSaveFileNameA(&ofn))
    {
        return false;
    }

    if (!m_frontend->saveSourceToFile(fileName, getEditorText()))
    {
        m_statusMessage = std::format("Could not save '{}'", fileName);
        return false;
    }

    m_frontend->loadSourceFromFile(fileName);
    syncEditorBufferFromSource(getEditorText());
    m_dirty = false;
    m_statusMessage = std::format("Saved '{}'", fileName);
    return true;
}

bool Editor::saveCurrentFile()
{
    if (m_frontend->getLoadedFilePath().empty())
    {
        return promptSaveFileAs();
    }

    if (!m_frontend->saveSourceToFile(m_frontend->getLoadedFilePath(), getEditorText()))
    {
        m_statusMessage = std::format("Could not save '{}'", m_frontend->getLoadedFilePath().string());
        return false;
    }

    m_frontend->loadSourceFromFile(m_frontend->getLoadedFilePath());
    syncEditorBufferFromSource(getEditorText());
    m_dirty = false;
    m_statusMessage = std::format("Saved '{}'", m_frontend->getLoadedFilePath().string());
    return true;
}

bool Editor::compileCurrentBuffer()
{
    const std::string sourceText = getEditorText();
    const std::filesystem::path workingDirectory = m_frontend->getLoadedFilePath().empty()
            ? std::filesystem::current_path()
            : m_frontend->getLoadedFilePath().parent_path();
    const std::string sourceName = m_frontend->getSourceName().empty()
            ? (m_frontend->getLoadedFilePath().empty() ? "scratch.ez"
                                                       : m_frontend->getLoadedFilePath().filename().string())
            : m_frontend->getSourceName();

    const bool ok = m_frontend->compileSource(sourceText, sourceName, workingDirectory);
    m_statusMessage = ok ? std::format("Compilation succeeded for '{}'", sourceName)
                         : std::format("Compilation failed for '{}'", sourceName);
    return ok;
}

void Editor::syncEditorBufferFromSource(const std::string &source)
{
    std::fill(m_editorBuffer.begin(), m_editorBuffer.end(), '\0');
    const size_t copySize = std::min(source.size(), m_editorBuffer.size() - 1);
    std::copy_n(source.data(), copySize, m_editorBuffer.data());
}

std::string Editor::getEditorText() const { return std::string(m_editorBuffer.data()); }

void Editor::renderToolbar()
{
    if (ImGui::Button("Open"))
    {
        promptOpenFile();
    }

    ImGui::SameLine();
    if (ImGui::Button("Save"))
    {
        saveCurrentFile();
    }

    ImGui::SameLine();
    if (ImGui::Button("Save As"))
    {
        promptSaveFileAs();
    }

    ImGui::SameLine();
    if (ImGui::Button("Compile"))
    {
        compileCurrentBuffer();
    }

    ImGui::SameLine();
    if (ImGui::Button("Reload") && !m_frontend->getLoadedFilePath().empty())
    {
        if (m_frontend->loadSourceFromFile(m_frontend->getLoadedFilePath()))
        {
            syncEditorBufferFromSource(m_frontend->getLoadedSourceText());
            m_dirty = false;
            m_statusMessage = std::format("Reloaded '{}'", m_frontend->getLoadedFilePath().string());
            compileCurrentBuffer();
        }
    }

    ImGui::SameLine();
    ImGui::TextDisabled("|");
    ImGui::SameLine();

    const bool buildOk = m_frontend->lastCompilationSucceeded();
    const std::string activeFile = m_frontend->getLoadedFilePath().empty()
                                           ? std::string("scratch.ez")
                                           : m_frontend->getLoadedFilePath().filename().string();
    ImGui::TextColored(buildOk ? ImVec4(0.35f, 0.85f, 0.45f, 1.0f) : ImVec4(0.95f, 0.45f, 0.35f, 1.0f),
                       "%s",
                       buildOk ? "BUILD OK" : "BUILD IDLE/FAILED");
    ImGui::SameLine();
    ImGui::TextDisabled("%s", m_statusMessage.c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("| file: %s | buffer: %zu bytes%s",
                        activeFile.c_str(),
                        getEditorText().size(),
                        m_dirty ? " | modified" : "");
}

void Editor::renderWorkspace()
{
    constexpr float diagnosticsHeight = 235.0f;

    if (ImGui::BeginChild("##workspace-top", ImVec2(0, -diagnosticsHeight), false))
    {
        if (ImGui::BeginTable("##ide-layout", 3, ImGuiTableFlags_Resizable | ImGuiTableFlags_BordersInnerV))
        {
            ImGui::TableSetupColumn("Project", ImGuiTableColumnFlags_WidthFixed, 275.0f);
            ImGui::TableSetupColumn("Editor", ImGuiTableColumnFlags_WidthStretch, 0.0f);
            ImGui::TableSetupColumn("Inspectors", ImGuiTableColumnFlags_WidthFixed, 430.0f);

            ImGui::TableNextColumn();
            renderProjectPanel();

            ImGui::TableNextColumn();
            renderEditorPanel();

            ImGui::TableNextColumn();
            renderInspectorPanel();

            ImGui::EndTable();
        }
    }
    ImGui::EndChild();

    ImGui::Separator();
    if (ImGui::BeginChild("##workspace-bottom", ImVec2(0, 0), true))
    {
        renderDiagnosticsPanel();
    }
    ImGui::EndChild();
}

void Editor::renderProjectPanel()
{
    ImGui::TextUnformatted("Project");
    ImGui::Separator();

    const std::string loadedFile =
            m_frontend->getLoadedFilePath().empty() ? "<unsaved>" : m_frontend->getLoadedFilePath().string();
    const std::string workingDir = m_frontend->getWorkingDirectory().empty()
            ? std::filesystem::current_path().string()
            : m_frontend->getWorkingDirectory().string();

    ImGui::TextWrapped("File: %s", loadedFile.c_str());
    ImGui::TextWrapped("Working Dir: %s", workingDir.c_str());
    ImGui::Text("Buffer: %zu bytes%s", getEditorText().size(), m_dirty ? " (modified)" : "");
    ImGui::Spacing();

    if (ImGui::CollapsingHeader("Compilation Pipeline", ImGuiTreeNodeFlags_DefaultOpen))
    {
        renderPipelineStages();
    }

    if (ImGui::CollapsingHeader("Included Files", ImGuiTreeNodeFlags_DefaultOpen))
    {
        renderIncludedFiles();
    }
}

void Editor::renderEditorPanel()
{
    ImGui::TextUnformatted("Source Editor");
    ImGui::Separator();

    const ImVec2 size = ImGui::GetContentRegionAvail();
    if (ImGui::InputTextMultiline("##ez-source",
                                  m_editorBuffer.data(),
                                  m_editorBuffer.size(),
                                  size,
                                  ImGuiInputTextFlags_AllowTabInput))
    {
        m_dirty = true;
    }
}

void Editor::renderInspectorPanel()
{
    ImGui::TextUnformatted("Inspectors");
    ImGui::Separator();

    if (ImGui::BeginTabBar("##inspectors"))
    {
        renderTokenizer();
        renderParser();
        renderSemantics();
        renderMir();
        ImGui::EndTabBar();
    }
}

void Editor::renderDiagnosticsPanel()
{
    ImGui::TextUnformatted("Diagnostics & Output");
    ImGui::Separator();

    const auto &diagnostics = m_frontend->getDiagnostics();
    if (diagnostics.empty())
    {
        ImGui::TextDisabled(
                "No diagnostics yet. Compile the current buffer to inspect lexer/parser/semantic/MIR output.");
        return;
    }

    if (ImGui::BeginTable(
                "##diagnostics-table",
                5,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("Severity", ImGuiTableColumnFlags_WidthFixed, 90.0f);
        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Sender", ImGuiTableColumnFlags_WidthFixed, 170.0f);
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Time", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        for (const FrontendDiagnosticEntry &diagnostic : diagnostics)
        {
            ImGui::TableNextRow();

            ImGui::TableNextColumn();
            const ImVec4 color = diagnostic.m_severity == ErrorSeverity::Fatal
                    ? ImVec4(0.95f, 0.35f, 0.35f, 1.0f)
                    : (diagnostic.m_severity == ErrorSeverity::Warning ? ImVec4(0.95f, 0.8f, 0.3f, 1.0f)
                                                                       : ImVec4(0.6f, 0.75f, 1.0f, 1.0f));
            ImGui::TextColored(color, "%s", formatSeverity(diagnostic.m_severity).c_str());

            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", formatSourceReference(diagnostic.m_sourceRef).c_str());

            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", diagnostic.m_sender.c_str());

            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", diagnostic.m_message.c_str());

            ImGui::TableNextColumn();
            ImGui::TextUnformatted(diagnostic.m_timeStamp.c_str());
        }

        ImGui::EndTable();
    }
}

void Editor::renderPipelineStages()
{
    for (const FrontendPipelineStage &stage : m_frontend->getPipelineStages())
    {
        ImGui::Bullet();
        ImGui::SameLine();
        ImGui::TextColored(stage.m_available ? ImVec4(0.35f, 0.85f, 0.45f, 1.0f) : ImVec4(0.85f, 0.5f, 0.45f, 1.0f),
                           "%s",
                           stage.m_name.c_str());
        ImGui::SameLine();
        ImGui::TextDisabled("- %s", stage.m_detail.c_str());
    }
}

void Editor::renderIncludedFiles()
{
    const auto &includedFiles = m_frontend->getIncludedFiles();
    if (includedFiles.empty())
    {
        ImGui::TextDisabled("No include directives resolved.");
        return;
    }

    for (const std::string &file : includedFiles)
    {
        ImGui::BulletText("%s", file.c_str());
    }
}

void Editor::renderTokenizer()
{
    if (!ImGui::BeginTabItem("Tokens"))
    {
        return;
    }

    const auto &tokens = m_frontend->getTokens();
    if (tokens.empty())
    {
        ImGui::TextDisabled("No token stream available.");
        ImGui::EndTabItem();
        return;
    }

    if (ImGui::BeginTable(
                "##tokens",
                5,
                ImGuiTableFlags_RowBg | ImGuiTableFlags_Borders | ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable))
    {
        ImGui::TableSetupColumn("#", ImGuiTableColumnFlags_WidthFixed, 40.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Text", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Line", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Col", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < tokens.size(); ++i)
        {
            const TokenInformation &token = tokens[i];
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            ImGui::Text("%zu", i);
            ImGui::TableNextColumn();
            ImGui::TextUnformatted(TokenType2StrMap[token.m_type]);
            ImGui::TableNextColumn();
            ImGui::TextWrapped("%s", token.m_str.c_str());
            ImGui::TableNextColumn();
            ImGui::Text("%zu", token.m_sourceReference.m_line);
            ImGui::TableNextColumn();
            ImGui::Text("%zu", token.m_sourceReference.m_col);
        }

        ImGui::EndTable();
    }

    ImGui::EndTabItem();
}

void Editor::renderParser()
{
    if (!ImGui::BeginTabItem("AST"))
    {
        return;
    }

    const auto &nodes = m_frontend->getParseResult();
    if (nodes.empty())
    {
        ImGui::TextDisabled("No AST available.");
        ImGui::EndTabItem();
        return;
    }

    for (size_t i = 0; i < nodes.size(); ++i)
    {
        renderAstNodeTree(nodes[i], std::format("{}: {}", i, nodes[i]->getAstNodeName()));
    }

    ImGui::EndTabItem();
}

void Editor::renderSemantics()
{
    if (!ImGui::BeginTabItem("Semantics"))
    {
        return;
    }

    const auto &unit = m_frontend->getCompilationUnit();
    if (!unit || !unit->getSemanticContext())
    {
        ImGui::TextDisabled("No semantic context available.");
        ImGui::EndTabItem();
        return;
    }

    auto semanticContext = unit->getSemanticContext();
    ImGui::Text("Global scope available: %s", semanticContext->getGlobalScope() ? "yes" : "no");
    ImGui::Text("Inside loop: %s", semanticContext->isContextInsideLoop() ? "yes" : "no");
    ImGui::Text("Inside switch: %s", semanticContext->isContextInsideSwitch() ? "yes" : "no");
    ImGui::Separator();

    renderScopeTree(semanticContext->getGlobalScope().get(), "Global Scope", false);

    ImGui::EndTabItem();
}

void Editor::renderMir()
{
    if (!ImGui::BeginTabItem("MIR"))
    {
        return;
    }

    const auto &unit = m_frontend->getCompilationUnit();
    if (!unit || !unit->getMirEmitterContext())
    {
        ImGui::TextDisabled("No MIR emitted yet.");
        ImGui::EndTabItem();
        return;
    }

    MirEmitterContext *mirContext = unit->getMirEmitterContext().get();
    auto *functionList = mirContext->getFunctionList();
    if (!functionList || functionList->m_numElems == 0)
    {
        ImGui::TextDisabled("No MIR functions available.");
        ImGui::EndTabItem();
        return;
    }

    int functionIndex = 0;
    for (MirFunction *function : *functionList)
    {
        MirType *returnType = mirContext->getMirTypeById(function->getReturnTypeId());
        const std::string label =
                std::format("fn#{} (id={}, return={})",
                            functionIndex++,
                            function->getId(),
                            returnType ? std::string(returnType->getName()) : std::string("<invalid>"));

        if (ImGui::TreeNode(label.c_str()))
        {
            ImGui::Text("Entry block: %zu", function->getEntryPoint() ? function->getEntryPoint()->getId() : 0ull);
            ImGui::Text("Parameters: %zu", function->getParameters() ? function->getParameters()->m_numElems : 0ull);

            if (function->getBlocks())
            {
                for (MirBlock *block : *function->getBlocks())
                {
                    const std::string blockLabel = std::format("block {}", block->getId());
                    if (ImGui::TreeNode(blockLabel.c_str()))
                    {
                        if (block->getInstructions())
                        {
                            int instructionIndex = 0;
                            for (MirInstruction *instruction : *block->getInstructions())
                            {
                                const MirInstructionMetadata &metadata = instruction->getMetadata();
                                const std::string instructionLabel =
                                        std::format("{}: {}", instructionIndex++, metadata.m_name);
                                if (ImGui::TreeNode(instructionLabel.c_str()))
                                {
                                    ImGui::Text("Opcode: %d", static_cast<int>(instruction->getOpCode()));
                                    ImGui::Text("Flags: 0x%X", instruction->getFlags());
                                    if (instruction->getOperands() && instruction->getOperands()->m_numElems > 0)
                                    {
                                        int operandIndex = 0;
                                        for (MirOperand *operand : *instruction->getOperands())
                                        {
                                            if (operand)
                                            {
                                                ImGui::BulletText("op%d = %s",
                                                                  operandIndex++,
                                                                  formatOperand(*operand).c_str());
                                            }
                                        }
                                    }
                                    else
                                    {
                                        ImGui::TextDisabled("No operands");
                                    }
                                    ImGui::TreePop();
                                }
                            }
                        }
                        ImGui::TreePop();
                    }
                }
            }
            ImGui::TreePop();
        }
    }

    ImGui::EndTabItem();
}

void Editor::renderAstNodeTree(AstNode *node, const std::string &label)
{
    if (!node)
    {
        return;
    }

    if (ImGui::TreeNode(label.c_str()))
    {
        ImGui::Text("Node: %s", node->getAstNodeName());
        ImGui::Text("Type: %d", static_cast<int>(node->getType()));
        ImGui::TextWrapped("Source: %s", formatSourceReference(node->getSourceRef()).c_str());
        if (node->hasAnnotations())
        {
            ImGui::Text("Annotations: %zu", node->getAnnotations() ? node->getAnnotations()->m_numElems : 0ull);
        }
        else
        {
            ImGui::TextDisabled("No annotations attached.");
        }
        ImGui::TreePop();
    }
}

void Editor::renderScopeTree(const Scope *scope, const char *label, bool includeParents)
{
    if (!scope)
    {
        ImGui::TextDisabled("No scope available.");
        return;
    }

    if (ImGui::TreeNode(label))
    {
        if (scope->getSymbols().empty())
        {
            ImGui::TextDisabled("No symbols in this scope.");
        }
        else
        {
            for (const auto &[name, symbol] : scope->getSymbols())
            {
                const std::string symbolLabel =
                        std::format("{} [{}]", name, symbol ? symbol->getSymbolTypeName() : "null");
                if (ImGui::TreeNode(symbolLabel.c_str()))
                {
                    if (symbol)
                    {
                        ImGui::Text("Id: %zu", symbol->getId());
                        ImGui::Text("Kind: %s", symbol->getSymbolTypeName());
                        ImGui::Text("Data type: %s", symbol->getSymbolDataTypeName().data());
                        if (symbol->getDefiningNode())
                        {
                            ImGui::Separator();
                            ImGui::Text("Defining node: %s", symbol->getDefiningNode()->getAstNodeName());
                            ImGui::TextWrapped(
                                    "Definition source: %s",
                                    formatSourceReference(symbol->getDefiningNode()->getSourceRef()).c_str());
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }

        if (includeParents && scope->getParent())
        {
            renderScopeTree(scope->getParent(), "Parent Scope", true);
        }
        ImGui::TreePop();
    }
}

std::string Editor::formatSeverity(ErrorSeverity severity) const
{
    switch (severity)
    {
        case ErrorSeverity::NoError:
            return "Info";
        case ErrorSeverity::Warning:
            return "Warning";
        case ErrorSeverity::Soft:
            return "Soft";
        case ErrorSeverity::Fatal:
            return "Fatal";
        default:
            return "Unknown";
    }
}

std::string Editor::formatSourceReference(const SourceReference &sourceRef) const
{
    if (!sourceRef.m_valid || !m_frontend->getSourceManager())
    {
        return "<no source>";
    }

    return std::format("{}:{}:{}",
                       m_frontend->getSourceManager()->getSourceName(sourceRef.m_sourceFileId),
                       sourceRef.m_line,
                       sourceRef.m_col);
}

std::string Editor::formatOperand(const MirOperand &operand) const
{
    switch (operand.getType())
    {
        case MirOperandType::BigInteger:
        {
            const MirBigInteger *value = operand.getBigInteger();
            return std::format("bigint[id={}, size={}]",
                               value ? value->m_constantId : 0ull,
                               value ? value->m_size : 0ull);
        }
        case MirOperandType::Double:
        {
            const MirDouble *value = operand.getDouble();
            return std::format("double({})", value ? value->m_value : 0.0);
        }
        case MirOperandType::Integer:
        {
            const MirInteger *value = operand.getInteger();
            return std::format("int({}, {}B)", value ? value->m_value : 0ll, value ? value->m_size : 0ull);
        }
        case MirOperandType::Memory:
        {
            const MirMemory *value = operand.getMemory();
            return std::format("mem(base=r{}, index=r{}, scale={}, offset={}, size={})",
                               value ? value->m_baseRegId : 0ull,
                               value ? value->m_indexRegId : 0ull,
                               value ? value->m_scale : 0,
                               value ? value->m_offset : 0ll,
                               value ? value->m_size : 0ull);
        }
        case MirOperandType::Reference:
        {
            const MirReference *value = operand.getReference();
            return std::format("ref({})", value ? value->m_refId : 0ull);
        }
        case MirOperandType::Register:
        {
            const MirRegister *value = operand.getRegister();
            return std::format("r{}:{}B", value ? value->m_id : 0ull, value ? value->m_size : 0ull);
        }
        default:
            return "<unknown>";
    }
}
