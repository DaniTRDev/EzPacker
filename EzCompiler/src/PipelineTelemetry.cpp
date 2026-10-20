#include "PipelineTelemetry.h"
#include "Diagnostics/DiagnosticMessage.h"
#include "SourceManager/GenericSourceManager.h"
#include <fstream>
#include <sstream>

namespace EzCompiler
{

namespace
{

std::string escapeJson(std::string_view s)
{
    std::string out;
    out.reserve(s.size() + 16);
    for (char c : s)
    {
        switch (c)
        {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\f': out += "\\f"; break;
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            if (static_cast<unsigned char>(c) < 0x20)
            {
                out += std::format("\\u{:04x}", static_cast<unsigned char>(c));
            }
            else
            {
                out += c;
            }
            break;
        }
    }
    return out;
}

std::string severityToString(DiagnosticMessageType type)
{
    if (type & Diag_Error) return "error";
    if (type & Diag_Warning) return "warning";
    if (type & Diag_Trace) return "trace";
    if (type & Diag_Debug) return "debug";
    return "info";
}

} // namespace

PipelineTelemetry::PipelineTelemetry() = default;

void PipelineTelemetry::recordPass(PassTelemetryRecord record)
{
    m_passes.push_back(std::move(record));
}

void PipelineTelemetry::updateFunctionAnalysis(const std::string &fnName, FunctionTelemetry telemetry)
{
    auto it = m_functions.find(fnName);
    if (it == m_functions.end())
    {
        m_functions[fnName] = std::move(telemetry);
        return;
    }

    FunctionTelemetry &existing = it->second;
    existing.name = telemetry.name;

    if (!telemetry.blocks.empty())
    {
        existing.blocks = std::move(telemetry.blocks);
    }
    if (!telemetry.liveness.empty())
    {
        existing.liveness = std::move(telemetry.liveness);
    }
    if (telemetry.frameLayout.isComputed)
    {
        existing.frameLayout = std::move(telemetry.frameLayout);
    }
}

void PipelineTelemetry::onDiag(const DiagnosticMessage &msg)
{
    DiagnosticTelemetryRecord rec;
    rec.severity = severityToString(msg.getType());
    rec.component = std::string(msg.getSender());
    rec.message = std::string(msg.getMainMsg());

    SourceReference *ref = msg.getPrimarySourceRef();
    if (ref && m_srcMgr)
    {
        const SourceLineRange *lineRange = m_srcMgr->getReferenceLine(ref);
        if (lineRange)
        {
            rec.line = lineRange->m_lineNumber;
            if (ref->m_beginOffset >= lineRange->m_beginOffset)
            {
                rec.col = (ref->m_beginOffset - lineRange->m_beginOffset) + 1;
            }
        }
    }

    m_diagnostics.push_back(std::move(rec));
}

std::string PipelineTelemetry::toJson() const
{
    std::ostringstream ss;
    ss << "{\n";
    ss << "  \"target\": \"" << escapeJson(m_target) << "\",\n";
    ss << "  \"optLevel\": \"" << escapeJson(m_optLevel) << "\",\n";
    ss << "  \"inputFile\": \"" << escapeJson(m_inputFile) << "\",\n";
    ss << "  \"initialMir\": \"" << escapeJson(m_initialMir) << "\",\n";
    ss << "  \"assembly\": \"" << escapeJson(m_assembly) << "\",\n";

    // Passes
    ss << "  \"passes\": [\n";
    for (size_t i = 0; i < m_passes.size(); ++i)
    {
        const auto &p = m_passes[i];
        ss << "    {\n";
        ss << "      \"index\": " << p.index << ",\n";
        ss << "      \"name\": \"" << escapeJson(p.passName) << "\",\n";
        ss << "      \"stage\": \"" << escapeJson(p.stage) << "\",\n";
        ss << "      \"function\": \"" << escapeJson(p.functionName) << "\",\n";
        ss << "      \"type\": \"" << escapeJson(p.passType) << "\",\n";
        ss << "      \"durationMs\": " << p.durationMs << ",\n";
        ss << "      \"modifiedMir\": " << (p.modifiedMir ? "true" : "false") << ",\n";
        ss << "      \"instructionCount\": " << p.instructionCount << ",\n";
        ss << "      \"blockCount\": " << p.blockCount << ",\n";
        ss << "      \"mirDump\": \"" << escapeJson(p.mirDump) << "\"\n";
        ss << "    }" << (i + 1 < m_passes.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    // Diagnostics
    ss << "  \"diagnostics\": [\n";
    for (size_t i = 0; i < m_diagnostics.size(); ++i)
    {
        const auto &d = m_diagnostics[i];
        ss << "    {\n";
        ss << "      \"severity\": \"" << escapeJson(d.severity) << "\",\n";
        ss << "      \"component\": \"" << escapeJson(d.component) << "\",\n";
        ss << "      \"message\": \"" << escapeJson(d.message) << "\",\n";
        ss << "      \"file\": \"" << escapeJson(d.file) << "\",\n";
        ss << "      \"line\": " << d.line << ",\n";
        ss << "      \"col\": " << d.col << "\n";
        ss << "    }" << (i + 1 < m_diagnostics.size() ? "," : "") << "\n";
    }
    ss << "  ],\n";

    // Functions analysis
    ss << "  \"functions\": {\n";
    size_t fnIdx = 0;
    for (const auto &[fnName, fnData] : m_functions)
    {
        ss << "    \"" << escapeJson(fnName) << "\": {\n";
        ss << "      \"name\": \"" << escapeJson(fnData.name) << "\",\n";

        // Blocks
        ss << "      \"blocks\": [\n";
        for (size_t bi = 0; bi < fnData.blocks.size(); ++bi)
        {
            const auto &blk = fnData.blocks[bi];
            ss << "        {\n";
            ss << "          \"id\": " << blk.id << ",\n";
            ss << "          \"name\": \"" << escapeJson(blk.name) << "\",\n";
            ss << "          \"instructionCount\": " << blk.instructionCount << ",\n";

            ss << "          \"instructions\": [";
            for (size_t ii = 0; ii < blk.instructions.size(); ++ii)
            {
                ss << "\"" << escapeJson(blk.instructions[ii]) << "\""
                   << (ii + 1 < blk.instructions.size() ? ", " : "");
            }
            ss << "],\n";

            ss << "          \"predecessors\": [";
            for (size_t pi = 0; pi < blk.predecessors.size(); ++pi)
            {
                ss << "\"" << escapeJson(blk.predecessors[pi]) << "\""
                   << (pi + 1 < blk.predecessors.size() ? ", " : "");
            }
            ss << "],\n";

            ss << "          \"successors\": [";
            for (size_t si = 0; si < blk.successors.size(); ++si)
            {
                ss << "\"" << escapeJson(blk.successors[si]) << "\""
                   << (si + 1 < blk.successors.size() ? ", " : "");
            }
            ss << "]\n";
            ss << "        }" << (bi + 1 < fnData.blocks.size() ? "," : "") << "\n";
        }
        ss << "      ],\n";

        // Liveness
        ss << "      \"liveness\": {\n";
        for (size_t li = 0; li < fnData.liveness.size(); ++li)
        {
            const auto &lv = fnData.liveness[li];
            ss << "        \"" << escapeJson(lv.blockName) << "\": {\n";

            auto printRegList = [&](const char *label, const std::vector<std::string> &regs, bool comma) {
                ss << "          \"" << label << "\": [";
                for (size_t ri = 0; ri < regs.size(); ++ri)
                {
                    ss << "\"" << escapeJson(regs[ri]) << "\"" << (ri + 1 < regs.size() ? ", " : "");
                }
                ss << "]" << (comma ? "," : "") << "\n";
            };

            printRegList("def", lv.def, true);
            printRegList("use", lv.use, true);
            printRegList("liveIn", lv.liveIn, true);
            printRegList("liveOut", lv.liveOut, false);

            ss << "        }" << (li + 1 < fnData.liveness.size() ? "," : "") << "\n";
        }
        ss << "      },\n";

        // Frame Layout
        ss << "      \"frameLayout\": {\n";
        ss << "        \"calleeSaveSize\": " << fnData.frameLayout.calleeSaveSize << ",\n";
        ss << "        \"totalSize\": " << fnData.frameLayout.totalSize << ",\n";
        ss << "        \"hasCalls\": " << (fnData.frameLayout.hasCalls ? "true" : "false") << ",\n";
        ss << "        \"hasDynamicAllocs\": " << (fnData.frameLayout.hasDynamicAllocs ? "true" : "false") << ",\n";
        ss << "        \"usedCalleeSavedRegs\": [";
        for (size_t ri = 0; ri < fnData.frameLayout.usedCalleeSavedRegs.size(); ++ri)
        {
            ss << "\"" << escapeJson(fnData.frameLayout.usedCalleeSavedRegs[ri]) << "\""
               << (ri + 1 < fnData.frameLayout.usedCalleeSavedRegs.size() ? ", " : "");
        }
        ss << "]\n";
        ss << "      }\n";

        ss << "    }" << (++fnIdx < m_functions.size() ? "," : "") << "\n";
    }
    ss << "  }\n";
    ss << "}\n";

    return ss.str();
}

bool PipelineTelemetry::writeToFile(const std::filesystem::path &path) const
{
    std::error_code ec;
    std::filesystem::path parent = path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent, ec);
    }

    std::ofstream out(path, std::ios::out | std::ios::trunc);
    if (!out.is_open())
    {
        return false;
    }

    out << toJson();
    return out.good();
}

} // namespace EzCompiler

