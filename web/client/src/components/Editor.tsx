import React, { useEffect, useRef } from 'react';
import MonacoEditor, { Monaco, OnMount } from '@monaco-editor/react';
import { useCompilerStore } from '../store/compilerStore';
import { registerMirLanguage, EZPACKER_MIR_LANGUAGE_ID } from '../grammar/mirLang';
import { Code2, AlertCircle } from 'lucide-react';

export const Editor: React.FC = () => {
  const {
    code,
    setCode,
    compile,
    result,
    jumpToLocation,
    clearJumpLocation,
    inspectSymbol,
  } = useCompilerStore();

  const editorRef = useRef<any>(null);
  const monacoRef = useRef<Monaco | null>(null);

  const handleBeforeMount = (monaco: Monaco) => {
    registerMirLanguage(monaco);

    // Register Rich Symbol & Doxygen Documentation Hover Provider
    monaco.languages.registerHoverProvider(EZPACKER_MIR_LANGUAGE_ID, {
      provideHover: (model: any, position: any) => {
        const word = model.getWordAtPosition(position);
        if (!word) return null;

        const lineContent = model.getLineContent(position.lineNumber);
        const charBefore = lineContent.charAt(word.startColumn - 2);
        const isSymbolToken = charBefore === '@' || charBefore === '%';
        const cleanName = word.word;

        const { result: currentResult } = useCompilerStore.getState();
        const doc = currentResult?.matchedSymbols?.[cleanName] || currentResult?.matchedSymbols?.[`@${cleanName}`];
        const funcAnalysis = currentResult?.functions?.[cleanName];

        if (!doc && !funcAnalysis && !isSymbolToken) return null;

        const contents: any[] = [];

        if (doc) {
          contents.push({ value: `### ${doc.qualifiedName || doc.name}\n*${doc.kind.toUpperCase()}*` });
          if (doc.brief) contents.push({ value: `${doc.brief}` });
          if (doc.parameters && doc.parameters.length > 0) {
            const pText = doc.parameters.map(p => `- \`${p.type} ${p.name}\``).join('\n');
            contents.push({ value: `**Parameters:**\n${pText}` });
          }
          if (doc.returnType) {
            contents.push({ value: `**Returns:** \`${doc.returnType}\`` });
          }
          if (doc.headerFile) {
            contents.push({ value: `*Defined in: \`${doc.headerFile}:${doc.line || 1}\`*` });
          }
        } else if (funcAnalysis) {
          contents.push({ value: `### Function \`@${cleanName}\`` });
          const blkCount = Object.keys(funcAnalysis.blocks || {}).length;
          contents.push({
            value: `Basic Blocks: **${blkCount}** | Frame Size: **${funcAnalysis.frameLayout?.totalSize ?? 0} bytes** (Callee-saved: **${funcAnalysis.frameLayout?.calleeSaveSize ?? 0} bytes**)`
          });
        } else {
          return null;
        }

        contents.push({ value: `[Alt+Click] or switch to *Symbols & Docs* tab to inspect ABI layout & documentation.` });

        return {
          range: new monaco.Range(position.lineNumber, word.startColumn, position.lineNumber, word.endColumn),
          contents,
        };
      },
    });
  };

  const handleEditorDidMount: OnMount = (editor, monaco) => {
    editorRef.current = editor;
    monacoRef.current = monaco;

    // Ctrl+Enter or Cmd+Enter to compile
    editor.addCommand(monaco.KeyMod.CtrlCmd | monaco.KeyCode.Enter, () => {
      compile();
    });

    // Alt+Click on symbol to inspect documentation
    editor.onMouseDown((e) => {
      if (e.target.position && e.event.altKey) {
        const model = editor.getModel();
        if (!model) return;
        const word = model.getWordAtPosition(e.target.position);
        if (word) {
          inspectSymbol(word.word);
        }
      }
    });
  };

  // Update diagnostic markers in the editor when compilation results change
  useEffect(() => {
    if (!editorRef.current || !monacoRef.current) return;
    const monaco = monacoRef.current;
    const model = editorRef.current.getModel();
    if (!model) return;

    if (!result || !result.diagnostics) {
      monaco.editor.setModelMarkers(model, 'ezc', []);
      return;
    }

    const markers = result.diagnostics
      .filter(d => d.line !== undefined && d.line > 0)
      .map(d => {
        let severity = monaco.MarkerSeverity.Info;
        if (d.type === 'error') severity = monaco.MarkerSeverity.Error;
        else if (d.type === 'warning') severity = monaco.MarkerSeverity.Warning;

        return {
          startLineNumber: d.line!,
          startColumn: d.col || 1,
          endLineNumber: d.line!,
          endColumn: (d.col ? d.col + 5 : 100),
          message: `[${d.component}] ${d.message}`,
          severity,
        };
      });

    monaco.editor.setModelMarkers(model, 'ezc', markers);
  }, [result]);

  // Jump to specific error/warning location when clicked from diagnostics drawer
  useEffect(() => {
    if (!editorRef.current || !jumpToLocation) return;
    const editor = editorRef.current;

    editor.setPosition({
      lineNumber: jumpToLocation.line,
      column: jumpToLocation.col,
    });
    editor.revealPositionInCenter(
      { lineNumber: jumpToLocation.line, column: jumpToLocation.col },
      0 // immediate
    );
    editor.focus();

    clearJumpLocation();
  }, [jumpToLocation, clearJumpLocation]);

  return (
    <div className="h-full flex flex-col bg-zinc-950 border-r border-zinc-800/80">
      {/* Editor sub-header */}
      <div className="h-9 px-3 bg-zinc-900/60 border-b border-zinc-800/80 flex items-center justify-between text-xs text-zinc-400 select-none">
        <div className="flex items-center space-x-2">
          <Code2 className="w-3.5 h-3.5 text-zinc-400" />
          <span className="font-medium text-zinc-300">source.mir</span>
          <span className="text-[10px] text-zinc-400 font-mono">EzPacker SSA MIR</span>
        </div>
        <div className="flex items-center space-x-2 text-[11px]">
          {result && !result.success && (
            <span className="flex items-center space-x-1 text-rose-400 font-mono">
              <AlertCircle className="w-3 h-3" />
              <span>Compilation Error</span>
            </span>
          )}
        </div>
      </div>

      {/* Monaco Container */}
      <div className="flex-1 min-h-0">
        <MonacoEditor
          height="100%"
          language={EZPACKER_MIR_LANGUAGE_ID}
          theme="ezpacker-dark"
          value={code}
          onChange={(val) => setCode(val || '')}
          beforeMount={handleBeforeMount}
          onMount={handleEditorDidMount}
          options={{
            fontSize: 13,
            lineHeight: 20,
            fontFamily: 'Fira Code, JetBrains Mono, monospace',
            fontLigatures: true,
            minimap: { enabled: true, maxColumn: 80 },
            scrollBeyondLastLine: false,
            automaticLayout: true,
            tabSize: 4,
            insertSpaces: true,
            padding: { top: 8, bottom: 8 },
            renderLineHighlight: 'line',
            cursorBlinking: 'smooth',
            cursorSmoothCaretAnimation: 'on',
            bracketPairColorization: { enabled: true },
          }}
        />
      </div>
    </div>
  );
};
