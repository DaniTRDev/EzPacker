import React, { useState } from 'react';
import { useCompilerStore } from '../store/compilerStore';
import { Play, Sparkles, Sliders, Check, ChevronDown, Layers, Terminal, Cpu, BookOpen, RefreshCw, AlertCircle, CheckCircle2 } from 'lucide-react';

export const Header: React.FC = () => {
  const {
    targetTriple,
    setTargetTriple,
    optLevel,
    setOptLevel,
    features,
    toggleFeature,
    isPositionIndependent,
    setIsPositionIndependent,
    enableRtti,
    setEnableRtti,
    autoCompile,
    setAutoCompile,
    isCompiling,
    compile,
    examples,
    selectedExampleId,
    selectExample,
    targetInfo,
    result,
    doxygenStatus,
    generateDocs,
    isGeneratingDocs,
  } = useCompilerStore();

  const [featuresOpen, setFeaturesOpen] = useState(false);

  const optLevels: Array<'O0' | 'O1' | 'O2' | 'Os'> = ['O0', 'O1', 'O2', 'Os'];

  return (
    <header className="h-14 bg-zinc-900/90 border-b border-zinc-800 flex items-center justify-between px-4 select-none shrink-0 z-30 backdrop-blur-sm">
      {/* Brand & Examples */}
      <div className="flex items-center space-x-4">
        <div className="flex items-center space-x-2">
          <div className="w-7 h-7 rounded-lg bg-emerald-500/10 border border-emerald-500/30 flex items-center justify-center text-emerald-400 font-bold font-mono text-sm shadow-[0_0_12px_rgba(34,197,94,0.15)]">
            EZ
          </div>
          <div className="flex flex-col">
            <span className="font-semibold text-sm tracking-tight text-zinc-100 flex items-center space-x-1.5">
              <span>EzPacker</span>
              <span className="text-[10px] uppercase font-mono px-1.5 py-0.5 rounded bg-zinc-800 text-zinc-400 border border-zinc-700/60 font-medium">
                Explorer
              </span>
            </span>
          </div>
        </div>

        {/* Example Selector */}
        <div className="relative">
          <select
            value={selectedExampleId || ''}
            onChange={(e) => selectExample(e.target.value)}
            className="h-8 pl-3 pr-8 text-xs font-medium bg-zinc-950/70 border border-zinc-800 rounded-md text-zinc-200 hover:border-zinc-700 focus:outline-none focus:ring-1 focus:ring-emerald-500/50 appearance-none cursor-pointer transition-colors"
          >
            <option value="" disabled>Load Example...</option>
            {examples.map((ex) => (
              <option key={ex.id} value={ex.id}>
                {ex.title} ({ex.filename})
              </option>
            ))}
          </select>
          <ChevronDown className="w-3.5 h-3.5 text-zinc-400 absolute right-2.5 top-2.5 pointer-events-none" />
        </div>
      </div>

      {/* Target & Options */}
      <div className="flex items-center space-x-3">
        {/* Target Triple */}
        <div className="flex items-center space-x-1.5 text-xs text-zinc-400">
          <Cpu className="w-3.5 h-3.5 text-zinc-500" />
          <select
            value={targetTriple}
            onChange={(e) => setTargetTriple(e.target.value)}
            className="h-8 pl-2 pr-7 text-xs font-mono bg-zinc-950/70 border border-zinc-800 rounded-md text-zinc-200 hover:border-zinc-700 focus:outline-none focus:ring-1 focus:ring-emerald-500/50 appearance-none cursor-pointer transition-colors"
          >
            {targetInfo?.availableTriples.map((t) => (
              <option key={t.id} value={t.id}>
                {t.label}
              </option>
            )) || <option value="x86_64-pc-windows-msvc">x86_64 Windows (MSVC)</option>}
          </select>
        </div>

        {/* Optimization Level Segmented Buttons */}
        <div className="flex items-center bg-zinc-950/70 p-0.5 rounded-md border border-zinc-800">
          {optLevels.map((lvl) => (
            <button
              key={lvl}
              onClick={() => setOptLevel(lvl)}
              className={`px-2.5 py-1 text-xs font-mono font-medium rounded transition-all ${
                optLevel === lvl
                  ? 'bg-emerald-500/20 text-emerald-300 border border-emerald-500/40 shadow-sm'
                  : 'text-zinc-400 hover:text-zinc-200 hover:bg-zinc-800/40'
              }`}
            >
              -{lvl}
            </button>
          ))}
        </div>

        {/* Features Popover */}
        <div className="relative">
          <button
            onClick={() => setFeaturesOpen(!featuresOpen)}
            className={`h-8 px-2.5 text-xs font-medium rounded-md border flex items-center space-x-1.5 transition-colors ${
              features.length > 0 || isPositionIndependent
                ? 'bg-zinc-800/80 text-emerald-300 border-zinc-700'
                : 'bg-zinc-950/70 text-zinc-400 border-zinc-800 hover:border-zinc-700 hover:text-zinc-200'
            }`}
          >
            <Sliders className="w-3.5 h-3.5" />
            <span>Flags ({features.length + (isPositionIndependent ? 1 : 0)})</span>
            <ChevronDown className="w-3 h-3 ml-0.5 text-zinc-400" />
          </button>

          {featuresOpen && (
            <div className="absolute right-0 top-10 w-64 bg-zinc-900 border border-zinc-800 rounded-lg shadow-2xl p-3 z-50 text-xs text-zinc-300 space-y-2.5 backdrop-blur-md">
              <div className="font-semibold text-zinc-100 flex items-center justify-between pb-1 border-b border-zinc-800">
                <span>Compiler Flags</span>
                <button
                  onClick={() => setFeaturesOpen(false)}
                  className="text-zinc-500 hover:text-zinc-300"
                >
                  ✕
                </button>
              </div>

              {/* Code Generation Options */}
              <div className="space-y-1.5">
                <div className="text-[10px] uppercase font-mono tracking-wider text-zinc-400 font-medium">Code Generation</div>
                <label className="flex items-center space-x-2 cursor-pointer hover:text-zinc-100">
                  <input
                    type="checkbox"
                    checked={isPositionIndependent}
                    onChange={(e) => setIsPositionIndependent(e.target.checked)}
                    className="rounded border-zinc-700 bg-zinc-950 text-emerald-500 focus:ring-emerald-500/30"
                  />
                  <span>-fPIC (Position Independent Code)</span>
                </label>
                <label className="flex items-center space-x-2 cursor-pointer hover:text-zinc-100">
                  <input
                    type="checkbox"
                    checked={enableRtti}
                    onChange={(e) => setEnableRtti(e.target.checked)}
                    className="rounded border-zinc-700 bg-zinc-950 text-emerald-500 focus:ring-emerald-500/30"
                  />
                  <span>--rtti (Generate Runtime Type Info)</span>
                </label>
              </div>

              {/* CPU Feature Extensions */}
              <div className="space-y-1.5 pt-1 border-t border-zinc-800/60">
                <div className="text-[10px] uppercase font-mono tracking-wider text-zinc-400 font-medium">Target Features</div>
                {targetInfo?.features.map((feat) => {
                  const active = features.includes(feat.id);
                  return (
                    <label
                      key={feat.id}
                      className="flex items-center justify-between cursor-pointer hover:text-zinc-100 py-0.5"
                    >
                      <div className="flex items-center space-x-2">
                        <input
                          type="checkbox"
                          checked={active}
                          onChange={() => toggleFeature(feat.id)}
                          className="rounded border-zinc-700 bg-zinc-950 text-emerald-500 focus:ring-emerald-500/30"
                        />
                        <span className="font-mono">{feat.id}</span>
                      </div>
                      <span className="text-[10px] text-zinc-400">{feat.label}</span>
                    </label>
                  );
                })}
              </div>
            </div>
          )}
        </div>
      </div>

      {/* Compile Action & Status */}
      <div className="flex items-center space-x-3">
        {/* Doxygen Documentation Status Badge */}
        {doxygenStatus && (
          <button
            onClick={() => doxygenStatus.available && generateDocs()}
            disabled={!doxygenStatus.available || isGeneratingDocs}
            title={
              doxygenStatus.available
                ? `Doxygen documentation active (${doxygenStatus.entityCount} entities indexed). Click to re-index.`
                : doxygenStatus.errorMessage || 'Doxygen is not installed on host system.'
            }
            className={`hidden md:flex items-center space-x-1.5 px-2.5 py-1 rounded-md border text-[11px] font-mono transition-colors ${
              isGeneratingDocs
                ? 'bg-zinc-950 text-emerald-400 border-emerald-500/50'
                : doxygenStatus.available
                ? 'bg-zinc-950/70 hover:bg-zinc-900 text-zinc-300 border-zinc-800/80 hover:border-zinc-700 cursor-pointer'
                : 'bg-zinc-950/40 text-amber-400/80 border-amber-900/50 cursor-not-allowed'
            }`}
          >
            {isGeneratingDocs ? (
              <RefreshCw className="w-3 h-3 text-emerald-400 animate-spin" />
            ) : doxygenStatus.available ? (
              <span className="w-1.5 h-1.5 rounded-full bg-emerald-400" />
            ) : (
              <span className="w-1.5 h-1.5 rounded-full bg-amber-400" />
            )}
            <span>
              {isGeneratingDocs
                ? 'Indexing Docs...'
                : doxygenStatus.available
                ? `Docs (${doxygenStatus.entityCount})`
                : 'Docs Offline'}
            </span>
          </button>
        )}

        {/* Execution Time badge */}
        {result && (
          <div className="hidden sm:flex items-center space-x-1.5 px-2.5 py-1 rounded-md bg-zinc-950/70 border border-zinc-800/80 text-[11px] font-mono text-zinc-400">
            <span className="w-1.5 h-1.5 rounded-full bg-emerald-400 animate-pulse"></span>
            <span>{result.executionDurationMs}ms</span>
          </div>
        )}

        {/* Auto Compile Toggle */}
        <label className="flex items-center space-x-1.5 text-xs text-zinc-400 cursor-pointer select-none">
          <input
            type="checkbox"
            checked={autoCompile}
            onChange={(e) => setAutoCompile(e.target.checked)}
            className="rounded border-zinc-700 bg-zinc-950 text-emerald-500 focus:ring-emerald-500/30"
          />
          <span className="hidden md:inline">Auto-compile</span>
        </label>

        {/* Compile Button */}
        <button
          onClick={() => compile()}
          disabled={isCompiling}
          title="Compile (Ctrl+Enter)"
          className={`h-8 px-3.5 rounded-md font-medium text-xs flex items-center space-x-1.5 transition-all shadow-sm ${
            isCompiling
              ? 'bg-emerald-600/50 text-emerald-200 cursor-not-allowed'
              : 'bg-emerald-600 hover:bg-emerald-500 text-white active:scale-95 shadow-[0_0_15px_rgba(16,185,129,0.2)]'
          }`}
        >
          <Play className={`w-3.5 h-3.5 fill-current ${isCompiling ? 'animate-spin' : ''}`} />
          <span>{isCompiling ? 'Compiling...' : 'Compile'}</span>
          <kbd className="hidden lg:inline ml-1 text-[10px] opacity-75 font-mono px-1 py-0.2 bg-emerald-700/60 rounded">
            ^↵
          </kbd>
        </button>
      </div>
    </header>
  );
};
