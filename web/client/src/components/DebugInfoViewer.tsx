import React, { useMemo, useState } from 'react';
import { useCompilerStore } from '../store/compilerStore';
import {
  Bug,
  FileCode2,
  ListOrdered,
  Box,
  FunctionSquare,
  Search,
  ChevronRight,
  ChevronDown,
  Layers,
} from 'lucide-react';

type DebugSubTab = 'overview' | 'lines' | 'types' | 'functions';

export const DebugInfoViewer: React.FC = () => {
  const { result } = useCompilerStore();
  const [activeSubTab, setActiveSubTab] = useState<DebugSubTab>('overview');
  const [lineFilter, setLineFilter] = useState('');
  const [typeFilter, setTypeFilter] = useState('');
  const [expandedTypes, setExpandedTypes] = useState<Record<string, boolean>>({});

  const debugInfo = result?.debugInfo;
  const obj = result?.objectInfo;

  const debugSections = useMemo(() => {
    const objSections = (obj?.sections || []).filter(
      (s) => s.isDebug || s.name.startsWith('.debug')
    );
    const summaryMap = new Map(
      (obj?.debugSummary?.sections || []).map((s) => [s.name, s])
    );
    const relocCounts = new Map<string, number>();
    for (const rel of obj?.relocations || []) {
      relocCounts.set(rel.section, (relocCounts.get(rel.section) || 0) + 1);
    }

    if (objSections.length > 0) {
      return objSections.map((sec) => {
        const extra = summaryMap.get(sec.name);
        return {
          name: sec.name,
          size: sec.size,
          offset: sec.offset,
          relocCount: relocCounts.get(sec.name) || 0,
          subsectionCount: extra?.subsectionCount,
          recordCount: extra?.recordCount,
        };
      });
    }

    return (debugInfo?.emittedSections || []).map((name) => ({
      name,
      size: summaryMap.get(name)?.size ?? 0,
      offset: 0,
      relocCount: relocCounts.get(name) || 0,
      subsectionCount: summaryMap.get(name)?.subsectionCount,
      recordCount: summaryMap.get(name)?.recordCount,
    }));
  }, [obj, debugInfo]);

  const sourceFiles = useMemo(() => {
    if (debugInfo?.files && debugInfo.files.length > 0) {
      return debugInfo.files;
    }
    const mainFile = debugInfo?.mainFile || result?.pipeline?.inputFile;
    if (mainFile && (debugInfo?.fileCount ?? 0) > 0) {
      const parts = mainFile.replace(/\\/g, '/').split('/');
      const filename = parts.pop() || mainFile;
      const directory = parts.join('/') || '.';
      return [{ fileId: 1, filename, directory }];
    }
    return [];
  }, [debugInfo, result?.pipeline?.inputFile]);

  const filteredLines = useMemo(() => {
    const lines = debugInfo?.lines || [];
    if (!lineFilter.trim()) return lines;
    const q = lineFilter.toLowerCase();
    return lines.filter(
      (l) =>
        (l.functionName || '').toLowerCase().includes(q) ||
        l.file.toLowerCase().includes(q) ||
        String(l.line).includes(q) ||
        `0x${l.codeOffset.toString(16)}`.includes(q)
    );
  }, [debugInfo?.lines, lineFilter]);

  const filteredTypes = useMemo(() => {
    const types = debugInfo?.types || [];
    if (!typeFilter.trim()) return types;
    const q = typeFilter.toLowerCase();
    return types.filter(
      (t) =>
        t.name.toLowerCase().includes(q) ||
        t.kind.toLowerCase().includes(q) ||
        String(t.typeId).toLowerCase().includes(q)
    );
  }, [debugInfo?.types, typeFilter]);

  const toggleTypeExpand = (key: string) => {
    setExpandedTypes((prev) => ({ ...prev, [key]: !prev[key] }));
  };

  if (!debugInfo || !debugInfo.enabled || debugInfo.level === 'None') {
    return (
      <div className="h-full flex flex-col items-center justify-center text-zinc-400 text-xs p-6 text-center bg-zinc-950">
        <Bug className="w-8 h-8 text-zinc-600 mb-2.5 stroke-[1.5]" />
        <p className="font-medium text-zinc-300">No debug information emitted</p>
        <p className="text-[11px] text-zinc-500 mt-1 max-w-sm">
          Enable <span className="font-mono text-emerald-400">-g</span> (
          <span className="font-mono text-zinc-300">-g1</span> or{' '}
          <span className="font-mono text-zinc-300">-g2</span>) in the Flags popover to generate DWARF v5 or CodeView debug sections.
        </p>
      </div>
    );
  }

  const fmtLower = (debugInfo.format || '').toLowerCase();
  const formatLabel =
    fmtLower === 'dwarf5'
      ? 'DWARF v5'
      : fmtLower === 'codeview'
      ? 'CodeView CV8'
      : debugInfo.format || 'Unknown';

  return (
    <div className="h-full flex flex-col bg-zinc-950 overflow-hidden text-xs">
      {/* Header Bar */}
      <div className="h-10 px-3 bg-zinc-900/60 border-b border-zinc-800/80 flex items-center justify-between shrink-0">
        <div className="flex items-center space-x-2.5">
          <Bug className="w-4 h-4 text-emerald-400" />
          <span className="font-semibold text-zinc-200">Debug Info Inspector</span>
          <span className="text-[10px] font-mono text-emerald-300 bg-emerald-950/60 px-2 py-0.5 rounded border border-emerald-800/50">
            {formatLabel}
          </span>
          <span className="text-[10px] font-mono text-zinc-300 bg-zinc-800 px-2 py-0.5 rounded border border-zinc-700/60">
            {debugInfo.level}
          </span>
        </div>

        {/* Sub-tabs */}
        <div className="flex items-center bg-zinc-950 rounded p-0.5 border border-zinc-800 text-[11px]">
          <button
            onClick={() => setActiveSubTab('overview')}
            className={`px-2.5 py-0.5 rounded font-medium flex items-center space-x-1 transition-colors ${
              activeSubTab === 'overview'
                ? 'bg-zinc-800 text-emerald-300 font-semibold'
                : 'text-zinc-400 hover:text-zinc-200'
            }`}
          >
            <Layers className="w-3 h-3" />
            <span>Overview</span>
          </button>
          <button
            onClick={() => setActiveSubTab('lines')}
            className={`px-2.5 py-0.5 rounded font-medium flex items-center space-x-1 transition-colors ${
              activeSubTab === 'lines'
                ? 'bg-zinc-800 text-emerald-300 font-semibold'
                : 'text-zinc-400 hover:text-zinc-200'
            }`}
          >
            <ListOrdered className="w-3 h-3" />
            <span>Line Table ({debugInfo.lineEntryCount})</span>
          </button>
          <button
            onClick={() => setActiveSubTab('types')}
            className={`px-2.5 py-0.5 rounded font-medium flex items-center space-x-1 transition-colors ${
              activeSubTab === 'types'
                ? 'bg-zinc-800 text-emerald-300 font-semibold'
                : 'text-zinc-400 hover:text-zinc-200'
            }`}
          >
            <Box className="w-3 h-3" />
            <span>Types ({debugInfo.typeCount})</span>
          </button>
          <button
            onClick={() => setActiveSubTab('functions')}
            className={`px-2.5 py-0.5 rounded font-medium flex items-center space-x-1 transition-colors ${
              activeSubTab === 'functions'
                ? 'bg-zinc-800 text-emerald-300 font-semibold'
                : 'text-zinc-400 hover:text-zinc-200'
            }`}
          >
            <FunctionSquare className="w-3 h-3" />
            <span>Functions & Vars ({debugInfo.functionCount})</span>
          </button>
        </div>
      </div>

      {/* Main Content */}
      <div className="flex-1 overflow-y-auto p-4 custom-scrollbar">
        {/* 1. Overview */}
        {activeSubTab === 'overview' && (
          <div className="space-y-4">
            {/* Metadata Grid */}
            <div className="grid grid-cols-2 md:grid-cols-3 gap-3">
              <div className="p-3 rounded-lg bg-zinc-900/60 border border-zinc-800">
                <div className="text-[10px] uppercase font-mono text-zinc-500">Format</div>
                <div className="text-sm font-mono font-semibold text-emerald-400 mt-0.5">{formatLabel}</div>
              </div>
              <div className="p-3 rounded-lg bg-zinc-900/60 border border-zinc-800">
                <div className="text-[10px] uppercase font-mono text-zinc-500">Debug Level</div>
                <div className="text-sm font-mono font-semibold text-zinc-200 mt-0.5">{debugInfo.level}</div>
              </div>
              <div className="p-3 rounded-lg bg-zinc-900/60 border border-zinc-800">
                <div className="text-[10px] uppercase font-mono text-zinc-500">Language</div>
                <div className="text-sm font-mono font-semibold text-zinc-200 mt-0.5">
                  {debugInfo.language || (result?.pipeline?.inputFile?.endsWith('.c') ? 'C11' : 'EzPacker MIR')}
                </div>
              </div>
              <div className="p-3 rounded-lg bg-zinc-900/60 border border-zinc-800">
                <div className="text-[10px] uppercase font-mono text-zinc-500">Producer</div>
                <div className="text-xs font-mono text-zinc-300 mt-1 truncate" title={debugInfo.producer || 'EzPacker ezc'}>
                  {debugInfo.producer || 'EzPacker ezc'}
                </div>
              </div>
              <div className="p-3 rounded-lg bg-zinc-900/60 border border-zinc-800">
                <div className="text-[10px] uppercase font-mono text-zinc-500">CompDir</div>
                <div className="text-xs font-mono text-zinc-300 mt-1 truncate" title={debugInfo.compDir || '.'}>
                  {debugInfo.compDir || '.'}
                </div>
              </div>
              <div className="p-3 rounded-lg bg-zinc-900/60 border border-zinc-800">
                <div className="text-[10px] uppercase font-mono text-zinc-500">Main File</div>
                <div className="text-xs font-mono text-zinc-300 mt-1 truncate" title={debugInfo.mainFile || result?.pipeline?.inputFile || 'source'}>
                  {debugInfo.mainFile || result?.pipeline?.inputFile || 'source'}
                </div>
              </div>
            </div>

            {/* Telemetry Counts */}
            <div className="grid grid-cols-5 gap-2.5 font-mono">
              <div className="p-2.5 rounded bg-zinc-900/40 border border-zinc-800/80 text-center">
                <div className="text-lg font-bold text-emerald-400">{debugInfo.fileCount}</div>
                <div className="text-[10px] text-zinc-400">Files</div>
              </div>
              <div className="p-2.5 rounded bg-zinc-900/40 border border-zinc-800/80 text-center">
                <div className="text-lg font-bold text-emerald-400">{debugInfo.typeCount}</div>
                <div className="text-[10px] text-zinc-400">Types</div>
              </div>
              <div className="p-2.5 rounded bg-zinc-900/40 border border-zinc-800/80 text-center">
                <div className="text-lg font-bold text-emerald-400">{debugInfo.functionCount}</div>
                <div className="text-[10px] text-zinc-400">Functions</div>
              </div>
              <div className="p-2.5 rounded bg-zinc-900/40 border border-zinc-800/80 text-center">
                <div className="text-lg font-bold text-emerald-400">{debugInfo.globalCount}</div>
                <div className="text-[10px] text-zinc-400">Globals</div>
              </div>
              <div className="p-2.5 rounded bg-zinc-900/40 border border-zinc-800/80 text-center">
                <div className="text-lg font-bold text-emerald-400">{debugInfo.lineEntryCount}</div>
                <div className="text-[10px] text-zinc-400">Line Entries</div>
              </div>
            </div>

            {/* Source Files Table */}
            <div className="space-y-2">
              <div className="text-[11px] font-semibold text-zinc-300 flex items-center space-x-1.5">
                <FileCode2 className="w-3.5 h-3.5 text-emerald-400" />
                <span>Source Files ({debugInfo.fileCount})</span>
              </div>
              <div className="rounded-lg border border-zinc-800 overflow-hidden font-mono text-xs">
                <table className="w-full text-left">
                  <thead className="bg-zinc-900/80 border-b border-zinc-800 text-[11px] text-zinc-400">
                    <tr>
                      <th className="py-2 px-3 w-20">fileId</th>
                      <th className="py-2 px-3">filename</th>
                      <th className="py-2 px-3">directory</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-zinc-800/60 bg-zinc-950">
                    {sourceFiles.length > 0 ? (
                      sourceFiles.map((f) => (
                        <tr key={f.fileId} className="hover:bg-zinc-900/40">
                          <td className="py-2 px-3 text-emerald-400 font-semibold">{f.fileId}</td>
                          <td className="py-2 px-3 text-zinc-200">{f.filename}</td>
                          <td className="py-2 px-3 text-zinc-400">{f.directory || '.'}</td>
                        </tr>
                      ))
                    ) : (
                      <tr>
                        <td colSpan={3} className="py-3 text-center text-zinc-500">
                          {debugInfo.fileCount} source file(s) registered
                        </td>
                      </tr>
                    )}
                  </tbody>
                </table>
              </div>
            </div>

            {/* Emitted Binary Debug Sections */}
            <div className="space-y-2">
              <div className="text-[11px] font-semibold text-zinc-300 flex items-center space-x-1.5">
                <Bug className="w-3.5 h-3.5 text-emerald-400" />
                <span>Emitted Binary Debug Sections ({debugSections.length})</span>
              </div>
              {debugSections.length === 0 ? (
                <div className="p-4 text-center text-zinc-500 border border-zinc-800 rounded-lg">
                  No binary debug sections found in object output.
                </div>
              ) : (
                <div className="rounded-lg border border-zinc-800 overflow-hidden font-mono text-xs">
                  <table className="w-full text-left">
                    <thead className="bg-zinc-900/80 border-b border-zinc-800 text-[11px] text-zinc-400">
                      <tr>
                        <th className="py-2 px-3">Section</th>
                        <th className="py-2 px-3">Size</th>
                        <th className="py-2 px-3">Relocations</th>
                        <th className="py-2 px-3">Details</th>
                      </tr>
                    </thead>
                    <tbody className="divide-y divide-zinc-800/60 bg-zinc-950">
                      {debugSections.map((sec) => (
                        <tr key={sec.name} className="hover:bg-zinc-900/40">
                          <td className="py-2 px-3 text-emerald-400 font-semibold">{sec.name}</td>
                          <td className="py-2 px-3 text-zinc-300">{sec.size} bytes</td>
                          <td className="py-2 px-3 text-zinc-400">{sec.relocCount}</td>
                          <td className="py-2 px-3 text-zinc-400">
                            {sec.subsectionCount !== undefined && (
                              <span className="px-1.5 py-0.5 rounded bg-zinc-800 text-[10px] text-zinc-300 mr-1.5">
                                {sec.subsectionCount} C13 subsections
                              </span>
                            )}
                            {sec.recordCount !== undefined && (
                              <span className="px-1.5 py-0.5 rounded bg-zinc-800 text-[10px] text-zinc-300">
                                {sec.recordCount} type records
                              </span>
                            )}
                            {sec.subsectionCount === undefined && sec.recordCount === undefined && (
                              <span className="text-zinc-500">Offset 0x{sec.offset.toString(16)}</span>
                            )}
                          </td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              )}
            </div>
          </div>
        )}

        {/* 2. Line Table */}
        {activeSubTab === 'lines' && (
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <div className="text-[11px] text-zinc-400">
                Address-to-source line mappings ({debugInfo.lineEntryCount} entries):
              </div>
              <div className="relative">
                <Search className="w-3.5 h-3.5 text-zinc-500 absolute left-2 top-2 pointer-events-none" />
                <input
                  type="text"
                  placeholder="Filter lines..."
                  value={lineFilter}
                  onChange={(e) => setLineFilter(e.target.value)}
                  className="h-7 pl-7 pr-3 bg-zinc-900 border border-zinc-800 rounded font-mono text-xs text-zinc-200 placeholder-zinc-500 focus:outline-none focus:ring-1 focus:ring-emerald-500/50 w-52"
                />
              </div>
            </div>

            {debugInfo.lines && debugInfo.lines.length > 0 ? (
              <div className="rounded-lg border border-zinc-800 overflow-hidden font-mono text-xs">
                <table className="w-full text-left">
                  <thead className="bg-zinc-900/80 border-b border-zinc-800 text-[11px] text-zinc-400">
                    <tr>
                      <th className="py-2 px-3">Function</th>
                      <th className="py-2 px-3">Code Offset</th>
                      <th className="py-2 px-3">Location (file:line:col)</th>
                      <th className="py-2 px-3">Flags</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-zinc-800/60 bg-zinc-950">
                    {filteredLines.map((entry, idx) => (
                      <tr key={`line-${idx}`} className="hover:bg-zinc-900/40">
                        <td className="py-2 px-3 text-emerald-400 font-semibold">
                          {entry.functionName || '—'}
                        </td>
                        <td className="py-2 px-3 text-zinc-300">
                          0x{entry.codeOffset.toString(16).padStart(4, '0')}
                        </td>
                        <td className="py-2 px-3 text-zinc-200">
                          {entry.file}:{entry.line}:{entry.col ?? 0}
                        </td>
                        <td className="py-2 px-3 space-x-1">
                          {entry.isStmt && (
                            <span className="px-1.5 py-0.5 rounded bg-emerald-950/60 text-emerald-300 border border-emerald-800/40 text-[10px]">
                              isStmt
                            </span>
                          )}
                          {entry.prologueEnd && (
                            <span className="px-1.5 py-0.5 rounded bg-blue-950/60 text-blue-300 border border-blue-800/40 text-[10px]">
                              prologueEnd
                            </span>
                          )}
                          {entry.epilogueBegin && (
                            <span className="px-1.5 py-0.5 rounded bg-amber-950/60 text-amber-300 border border-amber-800/40 text-[10px]">
                              epilogueBegin
                            </span>
                          )}
                        </td>
                      </tr>
                    ))}
                  </tbody>
                </table>
              </div>
            ) : (
              <div className="p-6 rounded-lg border border-zinc-800 bg-zinc-900/30 text-center space-y-1.5">
                <div className="font-mono text-emerald-400 font-semibold">
                  {debugInfo.lineEntryCount} line table entries emitted
                </div>
                <p className="text-[11px] text-zinc-400 max-w-md mx-auto">
                  Binary line number program encoded in{' '}
                  <span className="font-mono text-zinc-300">
                    {debugInfo.format === 'CodeView' ? '.debug$S' : '.debug_line'}
                  </span>
                  . Inspect the raw section bytes and relocations in the Overview or Object tabs.
                </p>
              </div>
            )}
          </div>
        )}

        {/* 3. Types */}
        {activeSubTab === 'types' && (
          <div className="space-y-3">
            <div className="flex items-center justify-between">
              <div className="text-[11px] text-zinc-400">
                Registered debug types ({debugInfo.typeCount}):
              </div>
              <div className="relative">
                <Search className="w-3.5 h-3.5 text-zinc-500 absolute left-2 top-2 pointer-events-none" />
                <input
                  type="text"
                  placeholder="Filter types..."
                  value={typeFilter}
                  onChange={(e) => setTypeFilter(e.target.value)}
                  className="h-7 pl-7 pr-3 bg-zinc-900 border border-zinc-800 rounded font-mono text-xs text-zinc-200 placeholder-zinc-500 focus:outline-none focus:ring-1 focus:ring-emerald-500/50 w-52"
                />
              </div>
            </div>

            {debugInfo.types && debugInfo.types.length > 0 ? (
              <div className="rounded-lg border border-zinc-800 overflow-hidden font-mono text-xs">
                <table className="w-full text-left">
                  <thead className="bg-zinc-900/80 border-b border-zinc-800 text-[11px] text-zinc-400">
                    <tr>
                      <th className="py-2 px-3">Type ID</th>
                      <th className="py-2 px-3">Name</th>
                      <th className="py-2 px-3">Kind</th>
                      <th className="py-2 px-3">Size</th>
                      <th className="py-2 px-3">Align</th>
                      <th className="py-2 px-3">Members / Params</th>
                    </tr>
                  </thead>
                  <tbody className="divide-y divide-zinc-800/60 bg-zinc-950">
                    {filteredTypes.map((t) => {
                      const key = String(t.typeId);
                      const hasNested = (t.members && t.members.length > 0) || (t.params && t.params.length > 0);
                      const isExpanded = !!expandedTypes[key];
                      const hexId =
                        typeof t.typeId === 'number' ? `0x${t.typeId.toString(16)}` : String(t.typeId);
                      return (
                        <React.Fragment key={key}>
                          <tr
                            className={`hover:bg-zinc-900/40 ${hasNested ? 'cursor-pointer' : ''}`}
                            onClick={() => hasNested && toggleTypeExpand(key)}
                          >
                            <td className="py-2 px-3 text-zinc-400">{hexId}</td>
                            <td className="py-2 px-3 text-emerald-400 font-semibold">{t.name}</td>
                            <td className="py-2 px-3 text-zinc-300">{t.kind}</td>
                            <td className="py-2 px-3 text-zinc-300">{t.sizeInBytes} B</td>
                            <td className="py-2 px-3 text-zinc-400">{t.alignment ?? '—'}</td>
                            <td className="py-2 px-3 text-zinc-400">
                              {hasNested ? (
                                <span className="inline-flex items-center space-x-1 text-emerald-300">
                                  {isExpanded ? <ChevronDown className="w-3 h-3" /> : <ChevronRight className="w-3 h-3" />}
                                  <span>
                                    {t.members?.length ? `${t.members.length} members` : `${t.params?.length} params`}
                                  </span>
                                </span>
                              ) : (
                                '—'
                              )}
                            </td>
                          </tr>
                          {isExpanded && hasNested && (
                            <tr className="bg-zinc-900/30">
                              <td colSpan={6} className="px-6 py-2 text-[11px] text-zinc-300 space-y-1">
                                {t.members?.map((m, mi) => (
                                  <div key={mi} className="flex items-center space-x-3">
                                    <span className="text-emerald-400">{m.name}</span>
                                    <span className="text-zinc-400">{m.typeName}</span>
                                    {m.offsetBytes !== undefined && (
                                      <span className="text-zinc-500">+{m.offsetBytes}B</span>
                                    )}
                                  </div>
                                ))}
                                {t.params && (
                                  <div>Params: ({t.params.join(', ')})</div>
                                )}
                              </td>
                            </tr>
                          )}
                        </React.Fragment>
                      );
                    })}
                  </tbody>
                </table>
              </div>
            ) : (
              <div className="p-6 rounded-lg border border-zinc-800 bg-zinc-900/30 text-center space-y-1.5">
                <div className="font-mono text-emerald-400 font-semibold">
                  {debugInfo.typeCount} debug type entries emitted
                </div>
                <p className="text-[11px] text-zinc-400 max-w-md mx-auto">
                  Type records are encoded in{' '}
                  <span className="font-mono text-zinc-300">
                    {debugInfo.format === 'CodeView' ? '.debug$T' : '.debug_info'}
                  </span>
                  .
                </p>
              </div>
            )}
          </div>
        )}

        {/* 4. Functions & Variables */}
        {activeSubTab === 'functions' && (
          <div className="space-y-4">
            {debugInfo.functions && debugInfo.functions.length > 0 ? (
              <div className="space-y-3">
                {debugInfo.functions.map((fn) => (
                  <div
                    key={fn.name}
                    className="p-3.5 rounded-lg bg-zinc-900/60 border border-zinc-800 space-y-2.5 font-mono"
                  >
                    <div className="flex items-center justify-between border-b border-zinc-800/70 pb-2">
                      <span className="font-bold text-sm text-emerald-400">{fn.name}</span>
                      <div className="flex items-center space-x-2 text-[10px] text-zinc-400">
                        {fn.sectionOffset !== undefined && (
                          <span>Offset: 0x{fn.sectionOffset.toString(16)}</span>
                        )}
                        {fn.codeSize !== undefined && <span>Size: {fn.codeSize}B</span>}
                        {fn.prologueSize !== undefined && <span>Prologue: {fn.prologueSize}B</span>}
                        {fn.totalFrameSize !== undefined && <span>Frame: {fn.totalFrameSize}B</span>}
                        {fn.hasFramePointer !== undefined && (
                          <span className="px-1.5 py-0.2 rounded bg-zinc-800 text-zinc-300">
                            FP: {fn.hasFramePointer ? 'yes' : 'no'}
                          </span>
                        )}
                      </div>
                    </div>

                    {fn.parameters && fn.parameters.length > 0 && (
                      <div className="space-y-1">
                        <div className="text-[10px] uppercase text-zinc-500">Parameters</div>
                        {fn.parameters.map((p, idx) => (
                          <div key={idx} className="flex items-center justify-between text-[11px] bg-zinc-950 px-2.5 py-1 rounded border border-zinc-800/60">
                            <span className="text-emerald-300">{p.name}</span>
                            <span className="text-zinc-400">{p.typeName || ''}</span>
                            <span className="text-zinc-300">{p.location || p.reg || '—'}</span>
                            <span className="text-zinc-500">{p.declLoc || ''}</span>
                          </div>
                        ))}
                      </div>
                    )}

                    {fn.locals && fn.locals.length > 0 && (
                      <div className="space-y-1">
                        <div className="text-[10px] uppercase text-zinc-500">Locals</div>
                        {fn.locals.map((v, idx) => (
                          <div key={idx} className="flex items-center justify-between text-[11px] bg-zinc-950 px-2.5 py-1 rounded border border-zinc-800/60">
                            <span className="text-zinc-200">{v.name}</span>
                            <span className="text-zinc-400">{v.typeName || ''}</span>
                            <span className="text-zinc-300">{v.location || v.reg || '—'}</span>
                            <span className="text-zinc-500">{v.declLoc || ''}</span>
                          </div>
                        ))}
                      </div>
                    )}
                  </div>
                ))}
              </div>
            ) : (
              <div className="space-y-3">
                <div className="text-[11px] text-zinc-400">
                  Compiled functions with debug records ({debugInfo.functionCount} functions, {debugInfo.globalCount} globals):
                </div>
                <div className="grid grid-cols-1 md:grid-cols-2 gap-3 font-mono">
                  {Object.values(result?.functions || {}).map((fn) => (
                    <div
                      key={fn.name}
                      className="p-3 rounded-lg bg-zinc-900/60 border border-zinc-800 space-y-1.5"
                    >
                      <div className="flex items-center justify-between">
                        <span className="font-bold text-emerald-400">{fn.name}</span>
                        <span className="text-[10px] text-zinc-400">
                          {Object.keys(fn.blocks || {}).length} blocks
                        </span>
                      </div>
                      <div className="text-[11px] text-zinc-400 flex items-center space-x-3">
                        <span>Frame: {fn.frameLayout?.totalSize ?? 0}B</span>
                        <span>Callee-Save: {fn.frameLayout?.calleeSaveSize ?? 0}B</span>
                      </div>
                    </div>
                  ))}
                </div>
              </div>
            )}

            {/* Globals */}
            <div className="space-y-2 pt-2 border-t border-zinc-800/60">
              <div className="text-[11px] font-semibold text-zinc-300">
                Globals ({debugInfo.globalCount})
              </div>
              {debugInfo.globals && debugInfo.globals.length > 0 ? (
                <div className="rounded-lg border border-zinc-800 overflow-hidden font-mono text-xs">
                  <table className="w-full text-left">
                    <thead className="bg-zinc-900/80 border-b border-zinc-800 text-[11px] text-zinc-400">
                      <tr>
                        <th className="py-2 px-3">Name</th>
                        <th className="py-2 px-3">Type</th>
                        <th className="py-2 px-3">Section</th>
                        <th className="py-2 px-3">Decl</th>
                      </tr>
                    </thead>
                    <tbody className="divide-y divide-zinc-800/60 bg-zinc-950">
                      {debugInfo.globals.map((g, idx) => (
                        <tr key={idx} className="hover:bg-zinc-900/40">
                          <td className="py-2 px-3 text-emerald-400 font-semibold">{g.name}</td>
                          <td className="py-2 px-3 text-zinc-300">{g.typeName || '—'}</td>
                          <td className="py-2 px-3 text-zinc-400">{g.section || '—'}</td>
                          <td className="py-2 px-3 text-zinc-500">{g.declLoc || '—'}</td>
                        </tr>
                      ))}
                    </tbody>
                  </table>
                </div>
              ) : (
                <div className="text-[11px] text-zinc-500 font-mono">
                  {debugInfo.globalCount === 0
                    ? 'No global variables with debug info in this module.'
                    : `${debugInfo.globalCount} global variable(s) emitted in debug info.`}
                </div>
              )}
            </div>
          </div>
        )}
      </div>
    </div>
  );
};
