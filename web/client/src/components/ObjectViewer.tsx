import React, { useState, useMemo } from 'react';
import { useCompilerStore } from '../store/compilerStore';
import { Binary, Layers, Search, Copy, Check, Hash, Cpu, ArrowUpRight, Shield, Database } from 'lucide-react';

export const ObjectViewer: React.FC = () => {
  const { result, inspectSymbol } = useCompilerStore();
  const [activeSubTab, setActiveSubTab] = useState<'overview' | 'symbols' | 'relocs' | 'hexdump'>('overview');
  const [symbolFilter, setSymbolFilter] = useState('');
  const [copied, setCopied] = useState(false);

  const obj = result?.objectInfo;

  const handleCopy = (text: string) => {
    navigator.clipboard.writeText(text);
    setCopied(true);
    setTimeout(() => setCopied(false), 1500);
  };

  const filteredSymbols = useMemo(() => {
    if (!obj || !obj.symbols) return [];
    if (!symbolFilter.trim()) return obj.symbols;
    const q = symbolFilter.toLowerCase();
    return obj.symbols.filter(
      s => s.name.toLowerCase().includes(q) || s.section.toLowerCase().includes(q)
    );
  }, [obj, symbolFilter]);

  if (!obj || obj.sections.length === 0) {
    return (
      <div className="h-full flex flex-col items-center justify-center text-zinc-400 text-xs p-6 text-center">
        <Binary className="w-8 h-8 text-zinc-500 mb-2 stroke-[1.5]" />
        <p>No compiled object (.o / .obj) binary available.</p>
        <p className="text-[11px] text-zinc-400 mt-1">Compile code to inspect binary sections, symbols, and relocations.</p>
      </div>
    );
  }

  return (
    <div className="h-full flex flex-col bg-zinc-950 overflow-hidden text-xs">
      {/* Header Bar */}
      <div className="h-10 px-3 bg-zinc-900/60 border-b border-zinc-800/80 flex items-center justify-between shrink-0">
        <div className="flex items-center space-x-2.5">
          <Binary className="w-4 h-4 text-emerald-400" />
          <span className="font-semibold text-zinc-200">Object Binary ({obj.format})</span>
          <span className="text-[10px] font-mono text-zinc-300 bg-zinc-800 px-2 py-0.5 rounded border border-zinc-700/60">
            {obj.fileSize} bytes
          </span>
          <span className="text-[10px] font-mono text-zinc-400 bg-zinc-800/50 px-2 py-0.5 rounded border border-zinc-700/40">
            {obj.sections.length} sections
          </span>
          <span className="text-[10px] font-mono text-zinc-400 bg-zinc-800/50 px-2 py-0.5 rounded border border-zinc-700/40">
            {obj.symbols.length} symbols
          </span>
        </div>

        {/* Sub-tab Navigation */}
        <div className="flex items-center space-x-2">
          <div className="flex items-center bg-zinc-950 rounded p-0.5 border border-zinc-800 text-[11px]">
            <button
              onClick={() => setActiveSubTab('overview')}
              className={`px-2.5 py-0.5 rounded font-medium transition-colors ${
                activeSubTab === 'overview'
                  ? 'bg-zinc-800 text-emerald-300 font-semibold'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              Sections ({obj.sections.length})
            </button>
            <button
              onClick={() => setActiveSubTab('symbols')}
              className={`px-2.5 py-0.5 rounded font-medium transition-colors ${
                activeSubTab === 'symbols'
                  ? 'bg-zinc-800 text-emerald-300 font-semibold'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              Symbols ({obj.symbols.length})
            </button>
            <button
              onClick={() => setActiveSubTab('relocs')}
              className={`px-2.5 py-0.5 rounded font-medium transition-colors ${
                activeSubTab === 'relocs'
                  ? 'bg-zinc-800 text-emerald-300 font-semibold'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              Relocations ({obj.relocations.length})
            </button>
            <button
              onClick={() => setActiveSubTab('hexdump')}
              className={`px-2.5 py-0.5 rounded font-medium transition-colors ${
                activeSubTab === 'hexdump'
                  ? 'bg-zinc-800 text-emerald-300 font-semibold'
                  : 'text-zinc-400 hover:text-zinc-200'
              }`}
            >
              Hex Dump
            </button>
          </div>

          <button
            onClick={() => handleCopy(obj.hexDump || '')}
            className="p-1.5 text-zinc-400 hover:text-zinc-200 rounded hover:bg-zinc-800 transition-colors"
            title="Copy Hex Dump"
          >
            {copied ? <Check className="w-3.5 h-3.5 text-emerald-400" /> : <Copy className="w-3.5 h-3.5" />}
          </button>
        </div>
      </div>

      {/* Main Content Area */}
      <div className="flex-1 overflow-y-auto p-4 custom-scrollbar">
        {/* View 1: Sections Overview */}
        {activeSubTab === 'overview' && (
          <div className="space-y-3.5">
            <div className="text-[11px] text-zinc-400 font-medium">
              Layout of binary sections emitted for target ABI ({obj.format}):
            </div>
            <div className="grid grid-cols-1 md:grid-cols-2 gap-3">
              {obj.sections.map((sec, idx) => (
                <div
                  key={`${sec.name}-${idx}`}
                  className="p-3.5 rounded-lg bg-zinc-900/60 border border-zinc-800 hover:border-zinc-700 transition-all font-mono"
                >
                  <div className="flex items-center justify-between pb-2 border-b border-zinc-800/70 mb-2.5">
                    <div className="flex items-center space-x-2">
                      <span className="font-bold text-sm text-emerald-400">{sec.name}</span>
                      <span className="text-[10px] px-1.5 py-0.2 rounded bg-zinc-800 text-zinc-300">
                        {sec.size} bytes
                      </span>
                    </div>
                    <span className="text-[10px] text-zinc-500">Offset: 0x{sec.offset.toString(16)}</span>
                  </div>

                  <div className="grid grid-cols-2 gap-2 text-[11px] mb-2.5 text-zinc-400">
                    <div>
                      <span className="text-zinc-500">Virtual Addr: </span>
                      <span className="text-zinc-300">0x{sec.virtualAddress.toString(16)}</span>
                    </div>
                    <div>
                      <span className="text-zinc-500">Flags: </span>
                      <span className="text-zinc-300">{sec.characteristics}</span>
                    </div>
                  </div>

                  {sec.hexPreview && (
                    <div className="pt-2 border-t border-zinc-800/50">
                      <div className="text-[10px] text-zinc-500 mb-1">Byte Stream Preview:</div>
                      <div className="text-[10px] text-zinc-400 bg-zinc-950 p-2 rounded border border-zinc-850 break-all select-all font-mono leading-relaxed">
                        {sec.hexPreview}
                      </div>
                    </div>
                  )}
                </div>
              ))}
            </div>
          </div>
        )}

        {/* View 2: Symbol Table */}
        {activeSubTab === 'symbols' && (
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <div className="text-[11px] text-zinc-400 font-medium">
                Object symbols linked against C ABI:
              </div>
              <div className="relative">
                <Search className="w-3.5 h-3.5 text-zinc-500 absolute left-2 top-2 pointer-events-none" />
                <input
                  type="text"
                  placeholder="Filter symbols..."
                  value={symbolFilter}
                  onChange={(e) => setSymbolFilter(e.target.value)}
                  className="h-7 pl-7 pr-3 bg-zinc-900 border border-zinc-800 rounded font-mono text-xs text-zinc-200 placeholder-zinc-500 focus:outline-none focus:ring-1 focus:ring-emerald-500/50 w-52"
                />
              </div>
            </div>

            <div className="rounded-lg border border-zinc-800 overflow-hidden font-mono text-xs">
              <table className="w-full text-left">
                <thead className="bg-zinc-900/80 border-b border-zinc-800 text-[11px] text-zinc-400 select-none">
                  <tr>
                    <th className="py-2 px-3">Symbol Name</th>
                    <th className="py-2 px-3">Section</th>
                    <th className="py-2 px-3">Offset / Value</th>
                    <th className="py-2 px-3">Binding</th>
                    <th className="py-2 px-3">Type</th>
                    <th className="py-2 px-3 text-right">Inspect</th>
                  </tr>
                </thead>
                <tbody className="divide-y divide-zinc-800/60 bg-zinc-950">
                  {filteredSymbols.map((sym, idx) => (
                    <tr
                      key={`${sym.name}-${idx}`}
                      className="hover:bg-zinc-900/50 transition-colors group cursor-pointer"
                      onClick={() => inspectSymbol(sym.name)}
                    >
                      <td className="py-2 px-3 font-semibold text-emerald-400 flex items-center space-x-1.5">
                        <span>{sym.name}</span>
                      </td>
                      <td className="py-2 px-3 text-zinc-300">
                        <span className={`px-1.5 py-0.5 rounded text-[10px] ${
                          sym.section === '.text'
                            ? 'bg-emerald-950/60 text-emerald-300 border border-emerald-800/40'
                            : 'bg-zinc-800/80 text-zinc-300'
                        }`}>
                          {sym.section}
                        </span>
                      </td>
                      <td className="py-2 px-3 text-zinc-400">
                        0x{sym.value.toString(16).padStart(4, '0')}
                      </td>
                      <td className="py-2 px-3">
                        <span className={`text-[10px] px-1.5 py-0.2 rounded font-medium ${
                          sym.binding === 'global'
                            ? 'bg-blue-950/60 text-blue-300 border border-blue-800/50'
                            : sym.binding === 'weak'
                            ? 'bg-amber-950/60 text-amber-300 border border-amber-800/50'
                            : 'bg-zinc-800 text-zinc-400'
                        }`}>
                          {sym.binding}
                        </span>
                      </td>
                      <td className="py-2 px-3 text-zinc-400">
                        {sym.type}
                      </td>
                      <td className="py-2 px-3 text-right">
                        <button
                          onClick={(e) => {
                            e.stopPropagation();
                            inspectSymbol(sym.name);
                          }}
                          className="text-[11px] px-2 py-0.5 rounded bg-zinc-800 hover:bg-emerald-600 hover:text-white text-zinc-300 transition-colors flex items-center space-x-1 ml-auto"
                        >
                          <span>Docs</span>
                          <ArrowUpRight className="w-3 h-3" />
                        </button>
                      </td>
                    </tr>
                  ))}
                  {filteredSymbols.length === 0 && (
                    <tr>
                      <td colSpan={6} className="py-4 text-center text-zinc-500">
                        No symbols match "{symbolFilter}"
                      </td>
                    </tr>
                  )}
                </tbody>
              </table>
            </div>
          </div>
        )}

        {/* View 3: Relocation Table */}
        {activeSubTab === 'relocs' && (
          <div className="space-y-3 font-mono text-xs">
            <div className="text-[11px] text-zinc-400 font-medium">
              Relocations requiring linker resolution at ABI call sites:
            </div>
            {obj.relocations.length === 0 ? (
              <div className="p-6 text-center text-zinc-500 border border-zinc-800/80 rounded-lg">
                No external relocations required for this compilation.
              </div>
            ) : (
              <div className="rounded-lg border border-zinc-800 overflow-hidden">
                <table className="w-full text-left">
                  <thead className="bg-zinc-900/80 border-b border-zinc-800 text-[11px] text-zinc-400 select-none">
                    <tr>
                      <th className="py-2 px-3">Section</th>
                      <th className="py-2 px-3">Offset</th>
                      <th className="py-2 px-3">Target Symbol</th>
                      <th className="py-2 px-3">Fixup Type</th>
                      {obj.relocations.some(r => r.addend !== undefined) && (
                        <th className="py-2 px-3">Addend</th>
                      )}
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-zinc-800/60 bg-zinc-950">
                    {obj.relocations.map((rel, idx) => (
                      <tr key={`rel-${idx}`} className="hover:bg-zinc-900/40">
                        <td className="py-2 px-3 text-emerald-400 font-semibold">{rel.section}</td>
                        <td className="py-2 px-3 text-zinc-400">0x{rel.offset.toString(16)}</td>
                        <td className="py-2 px-3 text-zinc-200">
                          <button
                            onClick={() => inspectSymbol(rel.symbolName)}
                            className="hover:text-emerald-400 underline decoration-zinc-700 underline-offset-2 flex items-center space-x-1"
                          >
                            <span>{rel.symbolName}</span>
                            <ArrowUpRight className="w-2.5 h-2.5 opacity-60" />
                          </button>
                        </td>
                        <td className="py-2 px-3 text-zinc-300 font-medium">{rel.type}</td>
                        {obj.relocations.some(r => r.addend !== undefined) && (
                          <td className="py-2 px-3 text-zinc-400">{rel.addend ?? 0}</td>
                        )}
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            )}
          </div>
        )}

        {/* View 4: Hex Dump */}
        {activeSubTab === 'hexdump' && (
          <div className="space-y-2">
            <div className="text-[11px] text-zinc-400 font-medium font-mono">
              Raw Byte Stream & Memory Layout (0x0000 - 0x{obj.fileSize.toString(16)}):
            </div>
            <pre className="p-3 rounded-lg bg-zinc-950 border border-zinc-800/80 font-mono text-[11px] text-zinc-300 overflow-x-auto select-all custom-scrollbar leading-relaxed">
              {obj.hexDump || '// No hex dump available'}
            </pre>
          </div>
        )}
      </div>
    </div>
  );
};
