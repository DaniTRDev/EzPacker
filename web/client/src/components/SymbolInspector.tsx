import React, { useState, useMemo, useEffect } from 'react';
import { useCompilerStore } from '../store/compilerStore';
import { DoxygenDocEntity } from '../types/compiler';
import {
  BookOpen,
  Search,
  Cpu,
  Layers,
  FileCode,
  Shield,
  ArrowRight,
  ExternalLink,
  Sparkles,
  Info,
  Server,
  Activity,
  Code2,
  RefreshCw,
} from 'lucide-react';

export const SymbolInspector: React.FC = () => {
  const {
    result,
    doxygenStatus,
    generateDocs,
    isGeneratingDocs,
    selectedSymbolName,
    selectedDocEntity,
    inspectSymbol,
    setPipelineStage,
    setSelectedAnalysisFunc,
    setAnalysisTab,
  } = useCompilerStore();

  const [query, setQuery] = useState('');
  const [kindFilter, setKindFilter] = useState<'all' | 'function' | 'struct' | 'runtime'>('all');
  const [searchResults, setSearchResults] = useState<DoxygenDocEntity[]>([]);
  const [isSearching, setIsSearching] = useState(false);

  // Collect symbols from compiled module
  const compiledFunctions = useMemo(() => {
    return Object.keys(result?.functions || {});
  }, [result]);

  const objectSymbols = useMemo(() => {
    return result?.objectInfo?.symbols || [];
  }, [result]);

  // Query Doxygen search endpoint when query changes
  useEffect(() => {
    let active = true;
    const fetchSymbols = async () => {
      setIsSearching(true);
      try {
        const res = await fetch(`/api/doxygen/symbols?q=${encodeURIComponent(query)}&limit=30`);
        if (res.ok && active) {
          const list: DoxygenDocEntity[] = await res.json();
          setSearchResults(list);
        }
      } catch {
        // Search error
      } finally {
        if (active) setIsSearching(false);
      }
    };

    const timer = setTimeout(fetchSymbols, 200);
    return () => {
      active = false;
      clearTimeout(timer);
    };
  }, [query]);

  // Combined candidates
  const allCandidateSymbols = useMemo(() => {
    const map = new Map<string, { name: string; kind: string; isCompiled: boolean; doc?: DoxygenDocEntity }>();

    // 1. Add current module functions
    for (const fn of compiledFunctions) {
      map.set(fn, {
        name: fn,
        kind: 'function',
        isCompiled: true,
        doc: result?.matchedSymbols?.[fn],
      });
    }

    // 2. Add object symbols
    for (const sym of objectSymbols) {
      if (!map.has(sym.name) && sym.name) {
        map.set(sym.name, {
          name: sym.name,
          kind: sym.type === 'function' ? 'function' : 'struct',
          isCompiled: true,
          doc: result?.matchedSymbols?.[sym.name],
        });
      }
    }

    // 3. Add search results from Doxygen
    for (const entity of searchResults) {
      if (!map.has(entity.name)) {
        map.set(entity.name, {
          name: entity.name,
          kind: entity.name.startsWith('__ez_') ? 'runtime' : entity.kind,
          isCompiled: false,
          doc: entity,
        });
      } else {
        const existing = map.get(entity.name)!;
        existing.doc = entity;
        if (entity.name.startsWith('__ez_')) existing.kind = 'runtime';
      }
    }

    // Convert to list and filter by kind & search
    let list = Array.from(map.values());
    if (kindFilter !== 'all') {
      list = list.filter(item => {
        if (kindFilter === 'runtime') return item.name.startsWith('__ez_') || item.kind === 'runtime';
        if (kindFilter === 'function') return item.kind === 'function';
        if (kindFilter === 'struct') return item.kind === 'struct' || item.kind === 'class';
        return true;
      });
    }

    return list;
  }, [compiledFunctions, objectSymbols, searchResults, kindFilter, result]);

  // Selected symbol metadata
  const activeName = selectedSymbolName || allCandidateSymbols[0]?.name;
  const activeEntity = selectedDocEntity || result?.matchedSymbols?.[activeName];
  const activeFuncAnalysis = activeName ? result?.functions?.[activeName] : undefined;
  const activeObjSym = activeName ? objectSymbols.find(s => s.name === activeName) : undefined;

  return (
    <div className="h-full flex flex-col bg-zinc-950 overflow-hidden text-xs">
      {/* Top Controls Bar */}
      <div className="h-10 px-3 bg-zinc-900/60 border-b border-zinc-800/80 flex items-center justify-between shrink-0">
        <div className="flex items-center space-x-2">
          <BookOpen className="w-4 h-4 text-emerald-400" />
          <span className="font-semibold text-zinc-200">Symbol & ABI Inspector</span>
          {doxygenStatus && (
            <span className={`text-[10px] font-mono px-2 py-0.5 rounded border ${
              doxygenStatus.available
                ? 'bg-emerald-950/40 text-emerald-300 border-emerald-800/40'
                : 'bg-amber-950/40 text-amber-300 border-amber-800/40'
            }`}>
              {doxygenStatus.available ? `Docs Ready (${doxygenStatus.entityCount})` : 'Docs Offline'}
            </span>
          )}
        </div>

        <div className="flex items-center space-x-2">
          {doxygenStatus?.available && (
            <button
              onClick={() => generateDocs()}
              disabled={isGeneratingDocs}
              className="flex items-center space-x-1.5 px-2.5 py-1 rounded bg-zinc-900 hover:bg-zinc-800 text-zinc-300 border border-zinc-700/60 text-[11px] transition-colors"
              title="Re-run Doxygen to refresh documentation index"
            >
              <RefreshCw className={`w-3 h-3 ${isGeneratingDocs ? 'animate-spin text-emerald-400' : 'text-zinc-400'}`} />
              <span>{isGeneratingDocs ? 'Indexing...' : 'Refresh Docs'}</span>
            </button>
          )}
        </div>
      </div>

      {/* Two Column Layout: Symbol List on Left, Detail Card on Right */}
      <div className="flex-1 flex overflow-hidden">
        {/* Left: Search & Symbol List */}
        <div className="w-64 border-r border-zinc-800/80 flex flex-col bg-zinc-950/50 shrink-0">
          {/* Search Box */}
          <div className="p-2 border-b border-zinc-800/70">
            <div className="relative">
              <Search className="w-3.5 h-3.5 text-zinc-500 absolute left-2 top-2 pointer-events-none" />
              <input
                type="text"
                placeholder="Search symbols..."
                value={query}
                onChange={(e) => setQuery(e.target.value)}
                className="w-full h-7 pl-7 pr-2 bg-zinc-900 border border-zinc-800 rounded font-mono text-xs text-zinc-200 placeholder-zinc-500 focus:outline-none focus:ring-1 focus:ring-emerald-500/50"
              />
            </div>

            {/* Filter Pills */}
            <div className="flex items-center space-x-1 mt-2 text-[10px]">
              <button
                onClick={() => setKindFilter('all')}
                className={`px-1.5 py-0.5 rounded transition-colors ${
                  kindFilter === 'all' ? 'bg-emerald-500/20 text-emerald-300 font-medium' : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                All
              </button>
              <button
                onClick={() => setKindFilter('function')}
                className={`px-1.5 py-0.5 rounded transition-colors ${
                  kindFilter === 'function' ? 'bg-emerald-500/20 text-emerald-300 font-medium' : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                Fn
              </button>
              <button
                onClick={() => setKindFilter('struct')}
                className={`px-1.5 py-0.5 rounded transition-colors ${
                  kindFilter === 'struct' ? 'bg-emerald-500/20 text-emerald-300 font-medium' : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                Types
              </button>
              <button
                onClick={() => setKindFilter('runtime')}
                className={`px-1.5 py-0.5 rounded transition-colors ${
                  kindFilter === 'runtime' ? 'bg-emerald-500/20 text-emerald-300 font-medium' : 'text-zinc-400 hover:text-zinc-200'
                }`}
              >
                C ABI
              </button>
            </div>
          </div>

          {/* List */}
          <div className="flex-1 overflow-y-auto divide-y divide-zinc-900 custom-scrollbar">
            {allCandidateSymbols.map((item) => {
              const isSelected = item.name === activeName;
              return (
                <div
                  key={item.name}
                  onClick={() => inspectSymbol(item.name)}
                  className={`p-2.5 cursor-pointer font-mono text-xs transition-colors ${
                    isSelected
                      ? 'bg-emerald-950/40 text-emerald-300 border-l-2 border-emerald-500'
                      : 'hover:bg-zinc-900/60 text-zinc-300'
                  }`}
                >
                  <div className="flex items-center justify-between">
                    <span className="font-medium truncate">{item.name}</span>
                    <span className={`text-[9px] px-1 py-0.2 rounded font-sans uppercase ${
                      item.name.startsWith('__ez_')
                        ? 'bg-purple-950/60 text-purple-300 border border-purple-800/40'
                        : item.isCompiled
                        ? 'bg-emerald-950/60 text-emerald-300 border border-emerald-800/40'
                        : 'bg-zinc-800 text-zinc-400'
                    }`}>
                      {item.name.startsWith('__ez_') ? 'ABI' : item.kind}
                    </span>
                  </div>
                  {item.doc?.brief && (
                    <div className="text-[10px] text-zinc-500 truncate mt-0.5 font-sans">
                      {item.doc.brief}
                    </div>
                  )}
                </div>
              );
            })}

            {allCandidateSymbols.length === 0 && (
              <div className="p-4 text-center text-zinc-500 text-[11px]">
                {isSearching ? 'Searching...' : 'No matching symbols found'}
              </div>
            )}
          </div>
        </div>

        {/* Right: Selected Symbol Detailed Card */}
        <div className="flex-1 overflow-y-auto p-5 custom-scrollbar bg-zinc-950">
          {activeName ? (
            <div className="max-w-3xl space-y-5">
              {/* Entity Header Banner */}
              <div className="p-4 rounded-xl bg-zinc-900/60 border border-zinc-800 space-y-2.5">
                <div className="flex items-start justify-between">
                  <div>
                    <div className="flex items-center space-x-2">
                      <span className="text-base font-bold font-mono text-zinc-100">
                        {activeEntity?.qualifiedName || activeName}
                      </span>
                      <span className={`text-[10px] px-2 py-0.5 rounded font-mono ${
                        activeName.startsWith('__ez_')
                          ? 'bg-purple-950 text-purple-300 border border-purple-800/50'
                          : 'bg-emerald-950 text-emerald-300 border border-emerald-800/50'
                      }`}>
                        {activeName.startsWith('__ez_') ? 'C ABI Runtime' : activeEntity?.kind || 'Function'}
                      </span>
                    </div>

                    {activeEntity?.headerFile && (
                      <div className="text-[11px] font-mono text-zinc-500 mt-1 flex items-center space-x-1">
                        <FileCode className="w-3 h-3 text-zinc-500" />
                        <span>Defined in: {activeEntity.headerFile}:{activeEntity.line || 1}</span>
                      </div>
                    )}
                  </div>

                  {/* Contextual Jump Actions */}
                  <div className="flex items-center space-x-2">
                    {activeFuncAnalysis && (
                      <>
                        <button
                          onClick={() => {
                            setSelectedAnalysisFunc(activeName);
                            setAnalysisTab('stack');
                          }}
                          className="px-2.5 py-1 rounded bg-zinc-800 hover:bg-zinc-700 text-zinc-200 text-[11px] flex items-center space-x-1 transition-colors"
                        >
                          <Server className="w-3 h-3 text-emerald-400" />
                          <span>Stack Layout</span>
                        </button>
                        <button
                          onClick={() => {
                            setSelectedAnalysisFunc(activeName);
                            setAnalysisTab('cfg');
                          }}
                          className="px-2.5 py-1 rounded bg-zinc-800 hover:bg-zinc-700 text-zinc-200 text-[11px] flex items-center space-x-1 transition-colors"
                        >
                          <Layers className="w-3 h-3 text-emerald-400" />
                          <span>CFG</span>
                        </button>
                      </>
                    )}
                    <button
                      onClick={() => setPipelineStage('asm')}
                      className="px-2.5 py-1 rounded bg-zinc-800 hover:bg-zinc-700 text-zinc-200 text-[11px] flex items-center space-x-1 transition-colors"
                    >
                      <Cpu className="w-3 h-3 text-emerald-400" />
                      <span>Assembly (-S)</span>
                    </button>
                  </div>
                </div>

                {/* Brief description */}
                {activeEntity?.brief && (
                  <p className="text-zinc-300 text-xs leading-relaxed font-sans pt-1">
                    {activeEntity.brief}
                  </p>
                )}
              </div>

              {/* ABI & Stack Frame Metrics (if compiled function) */}
              {(activeFuncAnalysis || activeObjSym) && (
                <div className="p-4 rounded-xl bg-zinc-900/40 border border-zinc-800/80 space-y-3">
                  <div className="flex items-center space-x-2 text-xs font-semibold text-zinc-200">
                    <Shield className="w-3.5 h-3.5 text-emerald-400" />
                    <span>ABI & Stack Frame Specifications</span>
                  </div>

                  <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 font-mono text-xs">
                    <div className="p-2.5 rounded bg-zinc-950 border border-zinc-850">
                      <div className="text-[10px] text-zinc-500 uppercase tracking-wider">Total Frame</div>
                      <div className="text-sm font-bold text-zinc-100 mt-0.5">
                        {activeFuncAnalysis?.frameLayout?.totalSize ?? 0} bytes
                      </div>
                    </div>
                    <div className="p-2.5 rounded bg-zinc-950 border border-zinc-850">
                      <div className="text-[10px] text-zinc-500 uppercase tracking-wider">Callee-Saved</div>
                      <div className="text-sm font-bold text-zinc-100 mt-0.5">
                        {activeFuncAnalysis?.frameLayout?.calleeSaveSize ?? 0} bytes
                      </div>
                    </div>
                    <div className="p-2.5 rounded bg-zinc-950 border border-zinc-850">
                      <div className="text-[10px] text-zinc-500 uppercase tracking-wider">Section</div>
                      <div className="text-sm font-bold text-emerald-400 mt-0.5">
                        {activeObjSym?.section || '.text'}
                      </div>
                    </div>
                    <div className="p-2.5 rounded bg-zinc-950 border border-zinc-850">
                      <div className="text-[10px] text-zinc-500 uppercase tracking-wider">Binding</div>
                      <div className="text-sm font-bold text-zinc-100 mt-0.5 capitalize">
                        {activeObjSym?.binding || 'Global'}
                      </div>
                    </div>
                  </div>
                </div>
              )}

              {/* Doxygen Detailed Docstrings & Parameters */}
              {activeEntity && (
                <div className="p-4 rounded-xl bg-zinc-900/40 border border-zinc-800/80 space-y-4">
                  <div className="flex items-center space-x-2 text-xs font-semibold text-zinc-200">
                    <BookOpen className="w-3.5 h-3.5 text-emerald-400" />
                    <span>Doxygen Documentation</span>
                  </div>

                  {activeEntity.detailedDescription && (
                    <div className="text-zinc-300 text-xs leading-relaxed space-y-2">
                      <div className="font-semibold text-zinc-400 text-[11px] uppercase tracking-wider">Description</div>
                      <p className="p-3 rounded bg-zinc-950 border border-zinc-850 font-sans">
                        {activeEntity.detailedDescription}
                      </p>
                    </div>
                  )}

                  {/* Parameters Table */}
                  {activeEntity.parameters && activeEntity.parameters.length > 0 && (
                    <div className="space-y-2">
                      <div className="font-semibold text-zinc-400 text-[11px] uppercase tracking-wider">Parameters</div>
                      <div className="rounded border border-zinc-800 overflow-hidden font-mono text-xs">
                        <table className="w-full text-left">
                          <thead className="bg-zinc-950 text-zinc-500 text-[10px] uppercase border-b border-zinc-800">
                            <tr>
                              <th className="py-1.5 px-3">Name</th>
                              <th className="py-1.5 px-3">Type</th>
                              <th className="py-1.5 px-3">Description</th>
                            </tr>
                          </thead>
                          <tbody className="divide-y divide-zinc-850 bg-zinc-950/60 font-sans">
                            {activeEntity.parameters.map((p, idx) => (
                              <tr key={idx}>
                                <td className="py-2 px-3 font-mono font-semibold text-emerald-400">{p.name}</td>
                                <td className="py-2 px-3 font-mono text-zinc-400">{p.type}</td>
                                <td className="py-2 px-3 text-zinc-300">{p.description || '—'}</td>
                              </tr>
                            ))}
                          </tbody>
                        </table>
                      </div>
                    </div>
                  )}

                  {/* Return Value */}
                  {activeEntity.returnType && (
                    <div className="space-y-1 font-mono text-xs">
                      <div className="font-semibold text-zinc-400 text-[11px] uppercase tracking-wider font-sans">Return Type</div>
                      <div className="p-2.5 rounded bg-zinc-950 border border-zinc-850 text-emerald-300">
                        {activeEntity.returnType}
                      </div>
                    </div>
                  )}
                </div>
              )}

              {!activeEntity && (
                <div className="p-8 text-center text-zinc-500 border border-dashed border-zinc-800 rounded-xl space-y-2">
                  <Info className="w-8 h-8 text-zinc-600 mx-auto" />
                  <p>No external Doxygen docstrings found for symbol "@{activeName}".</p>
                  <p className="text-[11px] text-zinc-600">
                    This symbol is defined locally inside the active MIR module.
                  </p>
                </div>
              )}
            </div>
          ) : (
            <div className="h-full flex flex-col items-center justify-center text-zinc-500 text-xs">
              <BookOpen className="w-10 h-10 text-zinc-600 mb-2 stroke-[1.5]" />
              <p>Select a symbol from the list or click a symbol in the editor to inspect it.</p>
            </div>
          )}
        </div>
      </div>
    </div>
  );
};
