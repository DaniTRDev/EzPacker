import React from 'react';
import { DiffEditor } from '@monaco-editor/react';
import { useCompilerStore } from '../store/compilerStore';
import { EZPACKER_MIR_LANGUAGE_ID } from '../grammar/mirLang';
import { stripAnsi } from '../utils/ansi';
import { GitCompare, ArrowRight } from 'lucide-react';

export const DiffViewer: React.FC = () => {
  const {
    result,
    diffStageA,
    diffStageB,
    setDiffStages,
  } = useCompilerStore();

  const stages: Array<{ id: 'mir' | 'legalizedMir' | 'loweredMir' | 'asm'; label: string }> = [
    { id: 'mir', label: '1. Generic SSA MIR' },
    { id: 'legalizedMir', label: '2. Legalized MIR' },
    { id: 'loweredMir', label: '3. Lowered Machine MIR' },
    { id: 'asm', label: '4. Target Assembly' },
  ];

  const getStageContent = (stageId: 'mir' | 'legalizedMir' | 'loweredMir' | 'asm') => {
    if (!result) return '';
    switch (stageId) {
      case 'mir':
        return result.mir || '// Generic MIR not available';
      case 'legalizedMir':
        return result.legalizedMir || '// Legalized MIR not available';
      case 'loweredMir':
        return result.loweredMir || '// Lowered MIR not available';
      case 'asm':
        return result.asm || '// Assembly not available';
    }
  };

  const originalContent = stripAnsi(getStageContent(diffStageA));
  const modifiedContent = stripAnsi(getStageContent(diffStageB));

  return (
    <div className="h-full flex flex-col bg-zinc-950 overflow-hidden">
      {/* Diff Stage Selectors */}
      <div className="h-10 px-3 bg-zinc-900/60 border-b border-zinc-800/80 flex items-center justify-between text-xs select-none">
        <div className="flex items-center space-x-2">
          <GitCompare className="w-4 h-4 text-emerald-400" />
          <span className="font-semibold text-zinc-200">IR Stage Comparison</span>
        </div>

        <div className="flex items-center space-x-2">
          {/* Left Stage */}
          <select
            value={diffStageA}
            onChange={(e) => setDiffStages(e.target.value as any, diffStageB)}
            className="h-7 px-2 text-xs font-medium bg-zinc-950/80 border border-zinc-800 rounded text-zinc-300 focus:outline-none focus:ring-1 focus:ring-emerald-500/50 cursor-pointer"
          >
            {stages.map((s) => (
              <option key={`left-${s.id}`} value={s.id}>
                {s.label}
              </option>
            ))}
          </select>

          <ArrowRight className="w-3.5 h-3.5 text-zinc-400" />

          {/* Right Stage */}
          <select
            value={diffStageB}
            onChange={(e) => setDiffStages(diffStageA, e.target.value as any)}
            className="h-7 px-2 text-xs font-medium bg-zinc-950/80 border border-zinc-800 rounded text-zinc-300 focus:outline-none focus:ring-1 focus:ring-emerald-500/50 cursor-pointer"
          >
            {stages.map((s) => (
              <option key={`right-${s.id}`} value={s.id}>
                {s.label}
              </option>
            ))}
          </select>
        </div>
      </div>

      {/* Monaco Diff Editor */}
      <div className="flex-1 min-h-0">
        <DiffEditor
          height="100%"
          language={EZPACKER_MIR_LANGUAGE_ID}
          theme="ezpacker-dark"
          original={originalContent}
          modified={modifiedContent}
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
      </div>
    </div>
  );
};
