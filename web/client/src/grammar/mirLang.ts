import type { Monaco } from '@monaco-editor/react';

export const EZPACKER_MIR_LANGUAGE_ID = 'ezpacker-mir';

export function registerMirLanguage(monaco: Monaco) {
  // Check if already registered
  const languages = monaco.languages.getLanguages();
  if (languages.some((lang: any) => lang.id === EZPACKER_MIR_LANGUAGE_ID)) {
    return;
  }

  monaco.languages.register({ id: EZPACKER_MIR_LANGUAGE_ID });

  // Language configuration (brackets, comments, auto-closing)
  monaco.languages.setLanguageConfiguration(EZPACKER_MIR_LANGUAGE_ID, {
    comments: {
      lineComment: '//',
    },
    brackets: [
      ['{', '}'],
      ['[', ']'],
      ['(', ')'],
    ],
    autoClosingPairs: [
      { open: '{', close: '}' },
      { open: '[', close: ']' },
      { open: '(', close: ')' },
      { open: '"', close: '"' },
    ],
    surroundingPairs: [
      { open: '{', close: '}' },
      { open: '[', close: ']' },
      { open: '(', close: ')' },
      { open: '"', close: '"' },
    ],
  });

  // Monarch Tokenizer definition
  monaco.languages.setMonarchTokensProvider(EZPACKER_MIR_LANGUAGE_ID, {
    defaultToken: '',
    tokenPostfix: '.mir',

    keywords: [
      'fn', 'entry', 'return', 'extern', 'global',
    ],

    typeKeywords: [
      'i8', 'i16', 'i32', 'i64',
      'u8', 'u16', 'u32', 'u64',
      'f32', 'f64', '_void*', 'bool',
      'void',
    ],

    opcodes: [
      'MOV', 'ADD', 'SUB', 'IMUL', 'IDIV', 'UDIV', 'REM',
      'AND', 'OR', 'XOR', 'SHL', 'SHR', 'SAR', 'NOT', 'NEG',
      'RET', 'CALL', 'PUSH', 'POP',
      'JMP', 'JE', 'JNE', 'JL', 'JLE', 'JG', 'JGE', 'JZ', 'JNZ',
      'CMP', 'SETLE', 'SETGE', 'SETL', 'SETG', 'SETE', 'SETNE',
      'LOAD', 'STORE', 'PHI', 'ALLOCA', 'LEA',
      'CAST', 'TRUNC', 'ZEXT', 'SEXT', 'BITCAST',
      'HL', 'TL', 'INT', // high-level, target-level, internal tags
    ],

    tokenizer: {
      root: [
        // Comments
        [/\/\/.*$/, 'comment'],

        // Function declarations: fn @name
        [/\bfn\b/, 'keyword'],
        [/@[a-zA-Z_][a-zA-Z0-9_]*/, 'function.name'],

        // Labels (e.g. entry:, base_case:)
        [/^[ \t]*[a-zA-Z_][a-zA-Z0-9_]*:/, 'type.identifier'],
        [/\.[a-zA-Z_][a-zA-Z0-9_]*:/, 'type.identifier'],

        // Registers (e.g. %arg1, %v24, %p0)
        [/%\.?[a-zA-Z_][a-zA-Z0-9_.]*/, 'variable.parameter'],

        // Arrows ->
        [/->/, 'delimiter'],

        // Hex numbers
        [/0[xX][0-9a-fA-F]+/, 'number.hex'],

        // Decimal numbers
        [/-?\d+/, 'number'],

        // Instruction opcodes
        [
          /[A-Za-z0-9_]+/,
          {
            cases: {
              '@typeKeywords': 'type',
              '@opcodes': 'keyword.control',
              '@keywords': 'keyword',
              '@default': 'identifier',
            },
          },
        ],

        // Delimiters
        [/[{}()\[\]]/, '@brackets'],
        [/[,;]/, 'delimiter'],
      ],
    },
  });

  // Custom Dark Theme for Monaco Editor
  monaco.editor.defineTheme('ezpacker-dark', {
    base: 'vs-dark',
    inherit: true,
    rules: [
      { token: 'comment', foreground: '6b7280', fontStyle: 'italic' },
      { token: 'keyword', foreground: '38bdf8', fontStyle: 'bold' }, // sky blue
      { token: 'keyword.control', foreground: 'c084fc', fontStyle: 'bold' }, // purple/violet opcodes
      { token: 'type', foreground: '34d399' }, // emerald types (i32, i64)
      { token: 'function.name', foreground: '60a5fa', fontStyle: 'bold' }, // blue function @symbol
      { token: 'variable.parameter', foreground: 'fbbf24' }, // amber registers %reg
      { token: 'type.identifier', foreground: 'f472b6', fontStyle: 'bold' }, // pink labels
      { token: 'number', foreground: 'fb923c' }, // orange numbers
      { token: 'number.hex', foreground: 'f97316' },
      { token: 'delimiter', foreground: '94a3b8' },
    ],
    colors: {
      'editor.background': '#09090b', // zinc-950
      'editor.foreground': '#f4f4f5',
      'editor.lineHighlightBackground': '#18181b', // zinc-900
      'editorCursor.foreground': '#22c55e', // emerald cursor
      'editorWhitespace.foreground': '#27272a',
      'editorLineNumber.foreground': '#52525b',
      'editorLineNumber.activeForeground': '#a1a1aa',
      'editor.selectionBackground': '#22c55e33',
      'editorGutter.background': '#09090b',
    },
  });
}
