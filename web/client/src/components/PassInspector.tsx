import React, { useState } from 'react';
import MonacoEditor, { DiffEditor } from '@monaco-editor/react';
import { useCompilerStore, PassInspectorMode } from '../store/compilerStore';
import { CfgViewer } from './CfgViewer';
import { LivenessViewer } from './LivenessViewer';
import { EZPACKER_MIR_LANGUAGE_ID, registerMirLanguage } from '../grammar/mirLang';
import { stripAnsi } from '../utils/ansi';
import {
  Code2,
  GitCompare,
  Network,
  Activity,
  ChevronLeft,
  ChevronRight,
  Copy,
  Check,
  Sparkles,
  Layers,
  ArrowRight,
  Info,
  CheckCircle2,
  Clock,
  Box,
} from 'lucide-react';

export const PassInspector: React.FC = () => {
  const {
    result,
    selectedPassIndex,
    setSelectedPassIndex,
    passInspectorMode,
    setPassInspectorMode,
  } = useCompilerStore();

  const [copied, setCopied] = useState(false);

  const passes = result?.pipeline?.passes || [];
  const currentPass = passes[selectedPassIndex];

  if (!result || !result.pipeline || passes.length === 0 || !currentPass) {
    return (
      <div className="h-full flex flex-col items-center justify-center text-zinc-500 text-xs p-8 text-center bg-zinc-950">
        <Layers className="w-10 h-10 text-zinc-600 mb-3 stroke-[1.5]" />
        <p className="text-zinc-300 font-medium text-sm">No pass telemetry available</p>
        <p className="text-zinc-500 text-[11px] mt-1 max-w-sm">
          Run compilation with valid MIR or C code to inspect each pass snapshot, instruction transformations, and diffs.
        </p>
      </div>
    );
  }

  const prevPass = selectedPassIndex > 0 ? passes[selectedPassIndex - 1] : null;
  const initialMir = result.pipeline.initialMir || '';

  const handleCopy = (text: string) => {
    navigator.clipboard.writeText(text);
    setCopied(true);
    setTimeout(() => setCopied(false), 1500);
  };

  const handleBeforeMount = (monaco: any) => {
    registerMirLanguage(monaco);
  };

  const originalDiffContent =
    passInspectorMode === 'diff_prev'
      ? prevPass ? prevPass.mirDump : initialMir
      : initialMir;

  const modifiedDiffContent = currentPass.mirDump;

  return (
    <div className="h-full flex flex-col bg-zinc-950 overflow-hidden text-xs select-none">
      {/* Pass Header & Details Bar */}
      <div className="p-2.5 bg-zinc-900/90 border-b border-zinc-800 flex flex-wrap items-center justify-between gap-2 shrink-0 backdrop-blur-sm">
        {/* Left: Pass Identification & Metrics */}
        <div className="flex items-center space-x-2 min-w-0">
          <div className="flex items-center space-x-1.5">
            <span className="font-mono text-zinc-400 font-bold text-xs bg-zinc-800 px-1.5 py-0.5 rounded border border-zinc-700/60">
              #{selectedPassIndex + 1}
            </span>
            <span className="font-mono font-semibold text-zinc-100 text-xs truncate max-w-[200px] sm:max-w-[320px]">
              {currentPass.name}
            </span>
          </div>

          {/* Stage Badge */}
          <span className="text-[10px] font-mono uppercase px-1.5 py-0.5 rounded bg-zinc-800/80 text-zinc-300 border border-zinc-700/60">
            {currentPass.stage}
          </span>

          {/* Target Function Tag */}
          {currentPass.function && (
            <span className="text-[10px] font-mono px-1.5 py-0.5 rounded bg-emerald-950/60 text-emerald-300 border border-emerald-800/60">
              @{currentPass.function}
            </span>
          )}

          {/* Mutation Status */}
          {currentPass.modifiedMir ? (
            <span className="text-[10px] font-mono font-bold px-1.5 py-0.5 rounded bg-emerald-950/80 text-emerald-400 border border-emerald-600/70 flex items-center space-x-1">
              <Sparkles className="w-2.5 h-2.5" />
              <span>Δ Modifies MIR</span>
            </span>
          ) : (
            <span className="text-[10px] font-mono text-zinc-500 px-1.5 py-0.5 rounded bg-zinc-900 border border-zinc-800">
              Preserves MIR
            </span>
          )}

          {/* Metrics Pill */}
          <div className="hidden lg:flex items-center space-x-2 text-[10px] font-mono text-zinc-400 border-l border-zinc-800 pl-2">
            <span>{currentPass.durationMs.toFixed(3)} ms</span>
            <span>•</span>
            <span>{currentPass.instructionCount} instrs</span>
            <span>•</span>
            <span>{currentPass.blockCount} blocks</span>
          </div>
        </div>

        {/* Right: Inspection Modes & Pass Stepping Controls */}
        <div className="flex items-center space-x-2">
          {/* Inspection Mode Tabs */}
          <div className="flex items-center bg-zinc-950 rounded-md p-0.5 border border-zinc-800 text-[11px] font-mono">
            <button
              onClick={() => setPassInspectorMode('snapshot')}
              className={`px-2 py-0.5 rounded flex items-center space-x-1 transition-colors ${
                passInspectorMode === 'snapshot'
                  ? 'bg-zinc-800 text-emerald-300 font-semibold'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
              title="Inspect full MIR snapshot at this pass"
            >
              <Code2 className="w-3 h-3" />
              <span>Snapshot</span>
            </button>

            <button
              onClick={() => setPassInspectorMode('diff_prev')}
              className={`px-2 py-0.5 rounded flex items-center space-x-1 transition-colors ${
                passInspectorMode === 'diff_prev'
                  ? 'bg-zinc-800 text-emerald-300 font-semibold'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
              title="Diff against previous pass result"
            >
              <GitCompare className="w-3 h-3" />
              <span>Diff vs Prev (Δ)</span>
            </button>

            <button
              onClick={() => setPassInspectorMode('diff_initial')}
              className={`px-2 py-0.5 rounded flex items-center space-x-1 transition-colors ${
                passInspectorMode === 'diff_initial'
                  ? 'bg-zinc-800 text-emerald-300 font-semibold'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
              title="Diff against initial frontend MIR"
            >
              <GitCompare className="w-3 h-3" />
              <span>Diff vs Initial</span>
            </button>

            <button
              onClick={() => setPassInspectorMode('cfg')}
              className={`px-2 py-0.5 rounded flex items-center space-x-1 transition-colors ${
                passInspectorMode === 'cfg'
                  ? 'bg-zinc-800 text-emerald-300 font-semibold'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
              title="Inspect CFG topology"
            >
              <Network className="w-3 h-3" />
              <span>CFG</span>
            </button>

            <button
              onClick={() => setPassInspectorMode('liveness')}
              className={`px-2 py-0.5 rounded flex items-center space-x-1 transition-colors ${
                passInspectorMode === 'liveness'
                  ? 'bg-zinc-800 text-emerald-300 font-semibold'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
              title="Inspect Liveness Analysis"
            >
              <Activity className="w-3 h-3" />
              <span>Liveness</span>
            </button>
          </div>

          {/* Stepper Buttons: Prev / Next */}
          <div className="flex items-center space-x-1 bg-zinc-950 p-0.5 rounded-md border border-zinc-800">
            <button
              onClick={() => setSelectedPassIndex(Math.max(0, selectedPassIndex - 1))}
              disabled={selectedPassIndex === 0}
              className="p-1 rounded text-zinc-400 hover:text-zinc-100 disabled:opacity-30 disabled:hover:text-zinc-400 transition-colors"
              title="Previous Pass"
            >
              <ChevronLeft className="w-3.5 h-3.5" />
            </button>
            <button
              onClick={() => setSelectedPassIndex(Math.min(passes.length - 1, selectedPassIndex + 1))}
              disabled={selectedPassIndex === passes.length - 1}
              className="p-1 rounded text-zinc-400 hover:text-zinc-100 disabled:opacity-30 disabled:hover:text-zinc-400 transition-colors"
              title="Next Pass"
            >
              <ChevronRight className="w-3.5 h-3.5" />
            </button>
          </div>

          {/* Copy Current Snapshot */}
          <button
            onClick={() => handleCopy(currentPass.mirDump)}
            className="p-1.5 text-zinc-400 hover:text-zinc-200 rounded hover:bg-zinc-800 transition-colors border border-zinc-800"
            title="Copy pass MIR snapshot"
          >
            {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
          </button>
        </div>
      </div>

      {/* Diff Sub-Header Legend */}
      {(passInspectorMode === 'diff_prev' || passInspectorMode === 'diff_initial') && (
        <div className="h-7 px-3 bg-zinc-900/60 border-b border-zinc-800/80 flex items-center justify-between font-mono text-[11px] text-zinc-400">
          <div className="flex items-center space-x-2">
            <span className="text-zinc-500 font-medium">Original (Left):</span>
            <span className="text-zinc-300 font-semibold">
              {passInspectorMode === 'diff_prev'
                ? prevPass
                  ? `Pass #${selectedPassIndex} (${prevPass.name})`
                  : 'Initial MIR'
                : 'Initial Input MIR'}
            </span>
          </div>

          <ArrowRight className="w-3 h-3 text-zinc-600" />

          <div className="flex items-center space-x-2">
            <span className="text-zinc-500 font-medium">Modified (Right):</span>
            <span className="text-emerald-400 font-semibold">
              Pass #{selectedPassIndex + 1} ({currentPass.name})
            </span>
          </div>
        </div>
      )}

      {/* Inspector Workspace Content */}
      <div className="flex-1 min-h-0 relative">
        {/* 1. Snapshot Mode */}
        {passInspectorMode === 'snapshot' && (
          <MonacoEditor
            height="100%"
            language={EZPACKER_MIR_LANGUAGE_ID}
            theme="ezpacker-dark"
            value={stripAnsi(currentPass.mirDump)}
            beforeMount={handleBeforeMount}
            options={{
              readOnly: true,
              fontSize: 13,
              lineHeight: 20,
              fontFamily: 'Fira Code, JetBrains Mono, monospace',
              minimap: { enabled: true, maxColumn: 80 },
              scrollBeyondLastLine: false,
              automaticLayout: true,
              padding: { top: 8, bottom: 8 },
            }}
          />
        )}

        {/* 2 & 3. Diff Modes */}
        {(passInspectorMode === 'diff_prev' || passInspectorMode === 'diff_initial') && (
          <DiffEditor
            height="100%"
            language={EZPACKER_MIR_LANGUAGE_ID}
            theme="ezpacker-dark"
            original={stripAnsi(originalDiffContent)}
            modified={stripAnsi(modifiedDiffContent)}
            beforeMount={handleBeforeMount}
            options={{
              readOnly: true,
              fontSize: 12,
              lineHeight: 19,
              fontFamily: 'Fira Code, JetBrains Mono, monospace',
              minimap: { enabled: false },
              renderSideBySide: true,
              scrollBeyondLastLine: false,
              automaticLayout: true,
              renderIndicators: true,
            }}
          />
        )}

        {/* 4. CFG Graph Mode */}
        {passInspectorMode === 'cfg' && <CfgViewer />}

        {/* 5. Liveness Analysis Mode */}
        {passInspectorMode === 'liveness' && <LivenessViewer />}
      </div>
    </div>
  );
};

