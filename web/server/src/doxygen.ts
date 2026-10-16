import { spawn, execSync } from 'child_process';
import * as fs from 'fs';
import * as path from 'path';
import { fileURLToPath } from 'url';
import { XMLParser } from 'fast-xml-parser';
import { DoxygenDocEntity, DoxygenParamInfo, DoxygenStatus } from './types.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const projectRoot = path.resolve(__dirname, '../../../');

export class DoxygenRunner {
  private doxygenBin: string | null = null;
  private version: string | null = null;
  private status: DoxygenStatus;
  private symbolIndex: Map<string, DoxygenDocEntity> = new Map();
  private generationPromise: Promise<DoxygenStatus> | null = null;
  private xmlParser: XMLParser;

  constructor() {
    this.xmlParser = new XMLParser({
      ignoreAttributes: false,
      attributeNamePrefix: '@_',
      textNodeName: '#text',
      trimValues: true,
    });

    this.doxygenBin = this.locateDoxygenExecutable();
    this.status = {
      available: this.doxygenBin !== null,
      generated: false,
      doxygenPath: this.doxygenBin || undefined,
      version: this.version || undefined,
      entityCount: 0,
      status: this.doxygenBin ? 'idle' : 'not_installed',
      errorMessage: this.doxygenBin ? undefined : 'Doxygen executable was not found on PATH or standard locations.',
    };
  }

  /**
   * Discovers the Doxygen executable on the host system.
   */
  private locateDoxygenExecutable(): string | null {
    if (process.env.DOXYGEN_BIN) {
      if (fs.existsSync(process.env.DOXYGEN_BIN)) {
        this.detectVersion(process.env.DOXYGEN_BIN);
        return path.resolve(process.env.DOXYGEN_BIN);
      }
      return null;
    }

    const candidates = [
      'E:\\Tools\\doxygen\\bin\\doxygen.exe',
      'C:\\Program Files\\doxygen\\bin\\doxygen.exe',
      'C:\\Program Files (x86)\\doxygen\\bin\\doxygen.exe',
      '/usr/bin/doxygen',
      '/usr/local/bin/doxygen',
    ];

    for (const cand of candidates) {
      if (fs.existsSync(cand)) {
        this.detectVersion(cand);
        return cand;
      }
    }

    try {
      const probe = process.platform === 'win32' ? 'where.exe doxygen' : 'which doxygen';
      const output = execSync(probe, { encoding: 'utf8', stdio: ['ignore', 'pipe', 'ignore'] }).trim();
      const firstLine = output.split(/\r?\n/)[0];
      if (firstLine && fs.existsSync(firstLine)) {
        this.detectVersion(firstLine);
        return firstLine;
      }
    } catch {
      // Not on PATH
    }

    return null;
  }

  private detectVersion(binPath: string): void {
    try {
      const ver = execSync(`"${binPath}" -v`, { encoding: 'utf8', timeout: 5000 }).trim();
      this.version = ver.split('\n')[0].trim();
    } catch {
      this.version = null;
    }
  }

  public getStatus(): DoxygenStatus {
    return { ...this.status };
  }

  public getSymbol(name: string): DoxygenDocEntity | null {
    const cleanName = name.replace(/^@/, '').trim();
    return this.symbolIndex.get(cleanName) || this.symbolIndex.get(name) || null;
  }

  public searchSymbols(query: string, limit: number = 25): DoxygenDocEntity[] {
    const q = query.toLowerCase().trim();
    if (!q) {
      return Array.from(this.symbolIndex.values()).slice(0, limit);
    }

    const results: DoxygenDocEntity[] = [];
    for (const [key, entity] of this.symbolIndex.entries()) {
      if (
        key.toLowerCase().includes(q) ||
        entity.name.toLowerCase().includes(q) ||
        entity.brief.toLowerCase().includes(q)
      ) {
        results.push(entity);
        if (results.length >= limit) break;
      }
    }
    return results;
  }

  /**
   * Correlates symbols found in source MIR or compiled functions with Doxygen docstrings.
   */
  public matchSymbols(names: string[]): Record<string, DoxygenDocEntity> {
    const matched: Record<string, DoxygenDocEntity> = {};
    for (const n of names) {
      const sym = this.getSymbol(n);
      if (sym) {
        matched[n] = sym;
      }
    }
    return matched;
  }

  /**
   * Generates documentation via Doxygen and parses the XML output into the memory index.
   */
  public async generateDocumentation(force: boolean = false): Promise<DoxygenStatus> {
    if (!this.doxygenBin) {
      this.status = {
        available: false,
        generated: false,
        entityCount: 0,
        status: 'not_installed',
        errorMessage: 'Cannot generate documentation: Doxygen executable is not installed or not in PATH.',
      };
      return this.status;
    }

    if (!force && this.status.generated && this.symbolIndex.size > 0) {
      return this.status;
    }

    if (this.generationPromise) {
      return this.generationPromise;
    }

    this.status.status = 'generating';
    this.status.errorMessage = undefined;

    this.generationPromise = (async () => {
      const xmlOutputDir = path.join(projectRoot, 'docs', 'xml_web');
      const tempDoxyfile = path.join(projectRoot, 'docs', '.Doxyfile.web');

      try {
        const baseDoxyfile = path.join(projectRoot, 'docs', 'Doxyfile');
        if (!fs.existsSync(baseDoxyfile)) {
          throw new Error(`Base Doxyfile not found at ${baseDoxyfile}`);
        }

        const configContent = [
          `@INCLUDE = "${baseDoxyfile.replace(/\\/g, '/')}"`,
          `OUTPUT_DIRECTORY = "${path.join(projectRoot, 'docs').replace(/\\/g, '/')}"`,
          `GENERATE_XML = YES`,
          `XML_OUTPUT = xml_web`,
          `GENERATE_HTML = NO`,
          `QUIET = YES`,
          `WARN_IF_DOC_ERROR = NO`,
          `WARN_IF_UNDOCUMENTED = NO`,
        ].join('\n');

        fs.writeFileSync(tempDoxyfile, configContent, 'utf8');

        // Execute doxygen with a 30s timeout
        const execResult = await this.spawnDoxygen(tempDoxyfile, 35000);

        if (execResult.exitCode !== 0) {
          throw new Error(
            `Doxygen failed with exit code ${execResult.exitCode}.\nStderr: ${execResult.stderr.slice(0, 500)}`
          );
        }

        const indexPath = path.join(xmlOutputDir, 'index.xml');
        if (!fs.existsSync(indexPath)) {
          throw new Error(`Doxygen completed successfully, but XML index was not found at: ${indexPath}`);
        }

        // Parse XML output into memory index
        await this.parseDoxygenXmlOutput(xmlOutputDir);

        this.status = {
          available: true,
          generated: true,
          doxygenPath: this.doxygenBin!,
          version: this.version || undefined,
          entityCount: this.symbolIndex.size,
          lastGenerated: new Date().toISOString(),
          status: 'ready',
        };

        return this.status;
      } catch (err: any) {
        console.error('[DoxygenRunner] Error during documentation generation:', err.message);
        this.status = {
          available: true,
          generated: false,
          doxygenPath: this.doxygenBin!,
          version: this.version || undefined,
          entityCount: 0,
          status: 'error',
          errorMessage: err.message || 'Unknown error occurred during Doxygen generation',
        };
        return this.status;
      } finally {
        this.generationPromise = null;
        try {
          if (fs.existsSync(tempDoxyfile)) {
            fs.unlinkSync(tempDoxyfile);
          }
        } catch {
          // Ignore temp cleanup error
        }
      }
    })();

    return this.generationPromise;
  }

  private spawnDoxygen(configFile: string, timeoutMs: number): Promise<{ exitCode: number; stdout: string; stderr: string }> {
    return new Promise((resolve) => {
      let stdout = '';
      let stderr = '';
      let timedOut = false;

      const proc = spawn(this.doxygenBin!, [configFile], {
        cwd: projectRoot,
      });

      const timer = setTimeout(() => {
        timedOut = true;
        proc.kill('SIGTERM');
      }, timeoutMs);

      proc.stdout?.on('data', (c) => { stdout += c.toString(); });
      proc.stderr?.on('data', (c) => { stderr += c.toString(); });

      proc.on('close', (code) => {
        clearTimeout(timer);
        if (timedOut) {
          resolve({
            exitCode: 124,
            stdout,
            stderr: `${stderr}\nDoxygen execution timed out after ${timeoutMs}ms.`,
          });
        } else {
          resolve({
            exitCode: code ?? 0,
            stdout,
            stderr,
          });
        }
      });

      proc.on('error', (err) => {
        clearTimeout(timer);
        resolve({
          exitCode: 1,
          stdout,
          stderr: `Failed to spawn doxygen: ${err.message}`,
        });
      });
    });
  }

  /**
   * Recursively extracts plain text from mixed XML AST objects.
   */
  private extractText(node: any): string {
    if (!node) return '';
    if (typeof node === 'string') return node;
    if (typeof node === 'number') return String(node);
    if (Array.isArray(node)) {
      return node.map((n) => this.extractText(n)).join(' ').trim();
    }
    if (typeof node === 'object') {
      let result = '';
      for (const key of Object.keys(node)) {
        if (key.startsWith('@_')) continue;
        const sub = this.extractText(node[key]);
        if (sub) {
          result = result ? `${result} ${sub}` : sub;
        }
      }
      return result.trim();
    }
    return '';
  }

  /**
   * Parses the generated Doxygen XML compound and index files.
   */
  private async parseDoxygenXmlOutput(xmlDir: string): Promise<void> {
    const indexPath = path.join(xmlDir, 'index.xml');
    const indexXml = fs.readFileSync(indexPath, 'utf8');
    const parsedIndex = this.xmlParser.parse(indexXml);

    const compounds = parsedIndex?.doxygenindex?.compound || [];
    const compoundList = Array.isArray(compounds) ? compounds : [compounds];

    const newIndex = new Map<string, DoxygenDocEntity>();

    for (const comp of compoundList) {
      if (!comp || !comp['@_refid']) continue;
      const refId = comp['@_refid'];
      const compKind = comp['@_kind'];

      // Parse compounds of interest: structs, classes, files, namespaces
      if (['struct', 'class', 'file'].includes(compKind)) {
        const compXmlFile = path.join(xmlDir, `${refId}.xml`);
        if (!fs.existsSync(compXmlFile)) continue;

        try {
          const compXml = fs.readFileSync(compXmlFile, 'utf8');
          const parsedComp = this.xmlParser.parse(compXml);
          const compoundDef = parsedComp?.doxygen?.compounddef;
          if (!compoundDef) continue;

          // If struct/class, index the compound itself
          if (compKind === 'struct' || compKind === 'class') {
            const structName = this.extractText(compoundDef.compoundname) || comp.name;
            const brief = this.extractText(compoundDef.briefdescription);
            const detailed = this.extractText(compoundDef.detaileddescription);
            const loc = compoundDef.location;

            const entity: DoxygenDocEntity = {
              id: refId,
              name: structName.split('::').pop() || structName,
              qualifiedName: structName,
              kind: compKind,
              brief,
              detailedDescription: detailed,
              parameters: [],
              headerFile: loc ? loc['@_file'] : undefined,
              line: loc && loc['@_line'] ? parseInt(loc['@_line'], 10) : undefined,
            };

            newIndex.set(entity.name, entity);
            newIndex.set(entity.qualifiedName, entity);
          }

          // Parse section definitions and memberdefs
          const sectionDefs = compoundDef.sectiondef || [];
          const sectionList = Array.isArray(sectionDefs) ? sectionDefs : [sectionDefs];

          for (const sec of sectionList) {
            const memberDefs = sec?.memberdef || [];
            const memberList = Array.isArray(memberDefs) ? memberDefs : [memberDefs];

            for (const mem of memberList) {
              if (!mem || !mem.name) continue;
              const memKind = mem['@_kind'];
              if (!['function', 'variable', 'typedef', 'enum'].includes(memKind)) continue;

              const memName = this.extractText(mem.name);
              const definition = this.extractText(mem.definition);
              const argsString = this.extractText(mem.argsstring);
              const returnType = this.extractText(mem.type);
              const brief = this.extractText(mem.briefdescription);
              const detailed = this.extractText(mem.detaileddescription);
              const loc = mem.location;

              // Parse parameters
              const parameters: DoxygenParamInfo[] = [];
              const rawParams = mem.param || [];
              const paramList = Array.isArray(rawParams) ? rawParams : [rawParams];

              for (const p of paramList) {
                if (!p) continue;
                const pName = this.extractText(p.declname) || this.extractText(p.defname);
                const pType = this.extractText(p.type);
                parameters.push({
                  name: pName,
                  type: pType,
                  description: '',
                });
              }

              const entity: DoxygenDocEntity = {
                id: mem['@_id'] || `${refId}_${memName}`,
                name: memName,
                qualifiedName: definition ? definition.split('(')[0].trim() : memName,
                kind: memKind as any,
                brief,
                detailedDescription: detailed,
                parameters,
                returnType: returnType || undefined,
                returnDescription: undefined,
                headerFile: loc ? loc['@_file'] : undefined,
                line: loc && loc['@_line'] ? parseInt(loc['@_line'], 10) : undefined,
              };

              newIndex.set(entity.name, entity);
              if (entity.qualifiedName && entity.qualifiedName !== entity.name) {
                newIndex.set(entity.qualifiedName, entity);
              }
            }
          }
        } catch (err: any) {
          // Log and continue parsing other compounds
        }
      }
    }

    this.symbolIndex = newIndex;
  }
}
