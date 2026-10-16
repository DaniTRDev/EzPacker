import React, { useEffect } from 'react';
import { PanelGroup, Panel, PanelResizeHandle } from 'react-resizable-panels';
import { useCompilerStore } from './store/compilerStore';
import { Header } from './components/Header';
import { Editor } from './components/Editor';
import { OutputPanel } from './components/OutputPanel';
import { DiagnosticsDrawer } from './components/DiagnosticsDrawer';

export const App: React.FC = () => {
  const { loadInitialData } = useCompilerStore();

  useEffect(() => {
    loadInitialData();
  }, [loadInitialData]);

  return (
    <div className="flex flex-col h-screen w-screen overflow-hidden bg-zinc-950 font-sans text-zinc-100">
      {/* Top Application Header */}
      <Header />

      {/* Main Resizable Workspace */}
      <div className="flex-1 min-h-0 relative">
        <PanelGroup direction="horizontal">
          {/* Left: Code Editor */}
          <Panel defaultSize={50} minSize={25} className="h-full">
            <Editor />
          </Panel>

          {/* Vertical Resize Handle */}
          <PanelResizeHandle className="w-1.5 bg-zinc-900 hover:bg-emerald-500/50 active:bg-emerald-500 transition-colors cursor-col-resize relative group">
            <div className="absolute inset-y-0 -left-1 -right-1" />
          </PanelResizeHandle>

          {/* Right: Output Tabs */}
          <Panel defaultSize={50} minSize={25} className="h-full">
            <OutputPanel />
          </Panel>
        </PanelGroup>
      </div>

      {/* Bottom Collapsible Diagnostics Drawer */}
      <DiagnosticsDrawer />
    </div>
  );
};

export default App;
