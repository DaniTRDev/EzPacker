import * as fs from 'fs';
import * as path from 'path';
import { fileURLToPath } from 'url';
import { ExampleFile } from './types.js';

const __filename = fileURLToPath(import.meta.url);
const __dirname = path.dirname(__filename);
const examplesDir = path.resolve(__dirname, '../../..', 'examples');

export function loadExamples(): ExampleFile[] {
  if (!fs.existsSync(examplesDir)) {
    console.warn(`[Examples] Directory not found at: ${examplesDir}`);
    return [];
  }

  const files = fs.readdirSync(examplesDir).filter(f => f.endsWith('.mir'));
  const examples: ExampleFile[] = [];

  for (const file of files) {
    const fullPath = path.join(examplesDir, file);
    const content = fs.readFileSync(fullPath, 'utf-8');

    // Extract title and description from leading comments if present
    const lines = content.split('\n');
    let title = file.replace('.mir', '').replace(/_/g, ' ');
    title = title.charAt(0).toUpperCase() + title.slice(1);

    const descLines: string[] = [];
    for (const l of lines) {
      const trimmed = l.trim();
      if (trimmed.startsWith('//')) {
        const text = trimmed.replace(/^\/\/\s*/, '').trim();
        if (text && !text.toLowerCase().includes('ezpacker custom mir module:')) {
          descLines.push(text);
        }
      } else if (trimmed.length > 0) {
        break;
      }
    }

    const description = descLines.join(' ') || `Demonstrates ${title.toLowerCase()} in EzPacker MIR.`;

    examples.push({
      id: file.replace('.mir', ''),
      filename: file,
      title,
      description,
      code: content,
    });
  }

  return examples;
}
