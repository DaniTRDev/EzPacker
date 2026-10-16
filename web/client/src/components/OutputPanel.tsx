import React, { useState } from 'react';
import MonacoEditor from '@monaco-editor/react';
import { useCompilerStore } from '../store/compilerStore';
import { PassViewer } from './PassViewer';
import { DiffViewer } from './DiffViewer';
import { CfgViewer } from './CfgViewer';
import { LivenessViewer } from './LivenessViewer';
import { StackViewer } from './StackViewer';
import { ObjectViewer } from './ObjectViewer';
import { SymbolInspector } from './SymbolInspector';
import { EZPACKER_MIR_LANGUAGE_ID } from '../grammar/mirLang';
import { stripAnsi } from '../utils/ansi';
import {
  Layers,
  Activity,
  BookOpen,
  GitCompare,
  Copy,
  Check,
  ChevronRight,
  Cpu,
  Binary,
  Code2,
  Network,
  Server,
  Clock,
} from 'lucide-react';

export const OutputPanel: React.FC = () => {
  const {
    viewMode,
    setViewMode,
    pipelineStage,
    setPipelineStage,
    analysisTab,
    setAnalysisTab,
    result,
    isCompiling,
  } = useCompilerStore();

  const [isDiffMode, setIsDiffMode] = useState(false);
  const [copied, setCopied] = useState(false);

  const handleCopy = (text: string) => {
    navigator.clipboard.writeText(text);
    setCopied(true);
    setTimeout(() => setCopied(false), 1500);
  };

  const getActivePipelineCode = () => {
    if (!result) return '';
    switch (pipelineStage) {
      case 'mir':
        return result.mir || '// Generic MIR not available';
      case 'legalizedMir':
        return result.legalizedMir || '// Legalized MIR not available';
      case 'loweredMir':
        return result.loweredMir || '// Lowered Machine MIR not available';
      case 'asm':
        return result.asm || '// Assembly output not available';
      case 'object':
        return '';
    }
  };

  return (
    <div className="h-full flex flex-col bg-zinc-950 overflow-hidden select-none">
      {/* Primary Workspace Navigation Bar (De-bloated 3-Mode Architecture) */}
      <div className="h-10 px-3 bg-zinc-900/90 border-b border-zinc-800 flex items-center justify-between text-xs shrink-0 backdrop-blur-sm">
        {/* Left: 3 Primary Workspace Modes */}
        <div className="flex items-center space-x-1">
          {/* 1. Pipeline Mode */}
          <button
            onClick={() => setViewMode('pipeline')}
            className={`px-3 py-1.5 font-medium rounded flex items-center space-x-1.5 transition-all ${
              viewMode === 'pipeline'
                ? 'bg-zinc-800 text-emerald-300 font-semibold shadow-sm'
                : 'text-zinc-400 hover:text-zinc-200 hover:bg-zinc-850'
            }`}
          >
            <Layers className="w-3.5 h-3.5" />
            <span>Pipeline</span>
          </button>

          {/* 2. Deep Analysis Mode */}
          <button
            onClick={() => setViewMode('analysis')}
            className={`px-3 py-1.5 font-medium rounded flex items-center space-x-1.5 transition-all ${
              viewMode === 'analysis'
                ? 'bg-zinc-800 text-emerald-300 font-semibold shadow-sm'
                : 'text-zinc-400 hover:text-zinc-200 hover:bg-zinc-850'
            }`}
          >
            <Activity className="w-3.5 h-3.5" />
            <span>Analysis</span>
            {result?.functions && (
              <span className="text-[10px] px-1 py-0.2 rounded bg-zinc-950 text-zinc-400 font-mono">
                {Object.keys(result.functions).length} fn
              </span>
            )}
          </button>

          {/* 3. Symbol & ABI Inspector Mode */}
          <button
            onClick={() => setViewMode('symbols')}
            className={`px-3 py-1.5 font-medium rounded flex items-center space-x-1.5 transition-all ${
              viewMode === 'symbols'
                ? 'bg-zinc-800 text-emerald-300 font-semibold shadow-sm'
                : 'text-zinc-400 hover:text-zinc-200 hover:bg-zinc-850'
            }`}
          >
            <BookOpen className="w-3.5 h-3.5" />
            <span>Symbols & Docs</span>
            {result?.objectInfo && (
              <span className="text-[10px] px-1 py-0.2 rounded bg-zinc-950 text-zinc-400 font-mono">
                {result.objectInfo.symbols.length}
              </span>
            )}
          </button>
        </div>

        {/* Right: Mode-Specific Sub-Controls */}
        <div className="flex items-center space-x-2">
          {/* Pipeline Mode Stage Stepper */}
          {viewMode === 'pipeline' && (
            <div className="flex items-center space-x-1.5">
              <div className="flex items-center bg-zinc-950 rounded-md p-0.5 border border-zinc-800 text-[11px] font-mono">
                <button
                  onClick={() => { setIsDiffMode(false); setPipelineStage('mir'); }}
                  className={`px-2 py-0.5 rounded transition-colors ${
                    !isDiffMode && pipelineStage === 'mir'
                      ? 'bg-zinc-800 text-emerald-300 font-semibold'
                      : 'text-zinc-400 hover:text-zinc-200'
                  }`}
                  title="Frontend SSA Intermediate Representation"
                >
                  MIR
                </button>
                <ChevronRight className="w-3 h-3 text-zinc-600 pointer-events-none" />
                <button
                  onClick={() => { setIsDiffMode(false); setPipelineStage('legalizedMir'); }}
                  className={`px-2 py-0.5 rounded transition-colors ${
                    !isDiffMode && pipelineStage === 'legalizedMir'
                      ? 'bg-zinc-800 text-emerald-300 font-semibold'
                      : 'text-zinc-400 hover:text-zinc-200'
                  }`}
                  title="Type & Opcode Legalized MIR"
                >
                  Legalized
                </button>
                <ChevronRight className="w-3 h-3 text-zinc-600 pointer-events-none" />
                <button
                  onClick={() => { setIsDiffMode(false); setPipelineStage('loweredMir'); }}
                  className={`px-2 py-0.5 rounded transition-colors ${
                    !isDiffMode && pipelineStage === 'loweredMir'
                      ? 'bg-zinc-800 text-emerald-300 font-semibold'
                      : 'text-zinc-400 hover:text-zinc-200'
                  }`}
                  title="Machine-Lowered Register Allocated MIR"
                >
                  Lowered
                </button>
                <ChevronRight className="w-3 h-3 text-zinc-600 pointer-events-none" />
                <button
                  onClick={() => { setIsDiffMode(false); setPipelineStage('asm'); }}
                  className={`px-2 py-0.5 rounded transition-colors ${
                    !isDiffMode && pipelineStage === 'asm'
                      ? 'bg-zinc-800 text-emerald-300 font-semibold'
                      : 'text-zinc-400 hover:text-zinc-200'
                  }`}
                  title="Final x86-64 Assembly Text (-S)"
                >
                  Assembly
                </button>
                <ChevronRight className="w-3 h-3 text-zinc-600 pointer-events-none" />
                <button
                  onClick={() => { setIsDiffMode(false); setPipelineStage('object'); }}
                  className={`px-2 py-0.5 rounded transition-colors flex items-center space-x-1 ${
                    !isDiffMode && pipelineStage === 'object'
                      ? 'bg-zinc-800 text-emerald-300 font-semibold'
                      : 'text-zinc-400 hover:text-zinc-200'
                  }`}
                  title="Compiled Relocatable Object File (.o/.obj)"
                >
                  <span>Object (.obj)</span>
                </button>
              </div>

              {/* Diff Toggle Button */}
              <button
                onClick={() => setIsDiffMode(!isDiffMode)}
                className={`p-1.5 rounded transition-colors border ${
                  isDiffMode
                    ? 'bg-emerald-950/60 text-emerald-300 border-emerald-700/60'
                    : 'bg-zinc-950 text-zinc-400 border-zinc-800 hover:text-zinc-200'
                }`}
                title="Toggle Side-by-Side IR Diff Viewer"
              >
                <GitCompare className="w-3.5 h-3.5" />
              </button>

              {/* Copy Code Button */}
              {!isDiffMode && pipelineStage !== 'object' && (
                <button
                  onClick={() => handleCopy(getActivePipelineCode())}
                  className="p-1.5 text-zinc-400 hover:text-zinc-200 rounded hover:bg-zinc-800 transition-colors"
                  title="Copy current stage output"
                >
                  {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
                </button>
              )}
            </div>
          )}

          {/* Analysis Mode Sub-Tabs */}
          {viewMode === 'analysis' && (
            <div className="flex items-center bg-zinc-950 rounded-md p-0.5 border border-zinc-800 text-[11px]">
              <button
                onClick={() => setAnalysisTab('cfg')}
                className={`px-2 py-0.5 rounded flex items-center space-x-1 transition-colors ${
                  analysisTab === 'cfg'
                    ? 'bg-zinc-800 text-emerald-300 font-semibold'
                    : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                <Network className="w-3 h-3" />
                <span>CFG Graph</span>
              </button>
              <button
                onClick={() => setAnalysisTab('liveness')}
                className={`px-2 py-0.5 rounded flex items-center space-x-1 transition-colors ${
                  analysisTab === 'liveness'
                    ? 'bg-zinc-800 text-emerald-300 font-semibold'
                    : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                <Activity className="w-3 h-3" />
                <span>Liveness</span>
              </button>
              <button
                onClick={() => setAnalysisTab('stack')}
                className={`px-2 py-0.5 rounded flex items-center space-x-1 transition-colors ${
                  analysisTab === 'stack'
                    ? 'bg-zinc-800 text-emerald-300 font-semibold'
                    : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                <Server className="w-3 h-3" />
                <span>Stack Frame</span>
              </button>
              <button
                onClick={() => setAnalysisTab('passes')}
                className={`px-2 py-0.5 rounded flex items-center space-x-1 transition-colors ${
                  analysisTab === 'passes'
                    ? 'bg-zinc-800 text-emerald-300 font-semibold'
                    : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                <Clock className="w-3 h-3" />
                <span>Pass Timing</span>
              </button>
            </div>
          )}
        </div>
      </div>

      {/* Main Workspace Body */}
      <div className="flex-1 min-h-0 relative">
        {/* ============================================================== */}
        {/* Mode 1: Compilation Pipeline View                              */}
        {/* ============================================================== */}
        {viewMode === 'pipeline' && (
          isDiffMode ? (
            <DiffViewer />
          ) : pipelineStage === 'object' ? (
            <ObjectViewer />
          ) : (
            <MonacoEditor
              height="100%"
              language={pipelineStage === 'asm' ? 'shell' : EZPACKER_MIR_LANGUAGE_ID}
              theme="ezpacker-dark"
              value={stripAnsi(getActivePipelineCode()) || (isCompiling ? '// Lowering compiler passes...' : '// Stage output not available')}
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
          )
        )}

        {/* ============================================================== */}
        {/* Mode 2: Deep Analysis View                                     */}
        {/* ============================================================== */}
        {viewMode === 'analysis' && (
          <>
            {analysisTab === 'cfg' && <CfgViewer />}
            {analysisTab === 'liveness' && <LivenessViewer />}
            {analysisTab === 'stack' && <StackViewer />}
            {analysisTab === 'passes' && <PassViewer />}
          </>
        )}

        {/* ============================================================== */}
        {/* Mode 3: Interactive Symbol & ABI Inspector                     */}
        {/* ============================================================== */}
        {viewMode === 'symbols' && <SymbolInspector />}
      </div>
    </div>
  );
};
