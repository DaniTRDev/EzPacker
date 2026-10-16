import * as fs from 'fs';
import { ObjectAnalysis, ObjectSectionInfo, ObjectSymbolInfo, ObjectRelocInfo } from './types.js';

/**
 * Parses binary object files (.obj / .o) emitted by EzPacker (COFF or ELF64).
 */
export function parseObjectFile(filePath: string): ObjectAnalysis {
  if (!fs.existsSync(filePath)) {
    throw new Error(`Object file not found: ${filePath}`);
  }

  const buf = fs.readFileSync(filePath);
  return parseObjectBuffer(buf);
}

/**
 * Formats a buffer into a standard hex + ASCII dump string.
 */
export function formatHexDump(buf: Buffer, maxBytes: number = 2048): string {
  const length = Math.min(buf.length, maxBytes);
  const lines: string[] = [];

  for (let i = 0; i < length; i += 16) {
    const slice = buf.subarray(i, Math.min(i + 16, length));
    const offsetHex = i.toString(16).padStart(8, '0');

    let hexPart = '';
    let asciiPart = '';

    for (let j = 0; j < 16; j++) {
      if (j < slice.length) {
        hexPart += slice[j].toString(16).padStart(2, '0') + ' ';
        const byte = slice[j];
        asciiPart += (byte >= 32 && byte <= 126) ? String.fromCharCode(byte) : '.';
      } else {
        hexPart += '   ';
      }
      if (j === 7) hexPart += ' ';
    }

    lines.push(`${offsetHex}  ${hexPart} |${asciiPart}|`);
  }

  if (buf.length > maxBytes) {
    lines.push(`... [${buf.length - maxBytes} more bytes truncated]`);
  }

  return lines.join('\n');
}

/**
 * Decodes a null-terminated ASCII string from a Buffer starting at given offset.
 */
function readNullTerminatedString(buf: Buffer, offset: number): string {
  let end = offset;
  while (end < buf.length && buf[end] !== 0) {
    end++;
  }
  return buf.toString('utf8', offset, end);
}

export function parseObjectBuffer(buf: Buffer): ObjectAnalysis {
  if (buf.length < 20) {
    return {
      format: 'Unknown',
      fileSize: buf.length,
      sections: [],
      symbols: [],
      relocations: [],
      hexDump: formatHexDump(buf),
    };
  }

  // Check for ELF64: 0x7F 'E' 'L' 'F'
  if (buf[0] === 0x7f && buf[1] === 0x45 && buf[2] === 0x4c && buf[3] === 0x46) {
    return parseElf64(buf);
  }

  // Check for COFF AMD64: machine is 0x8664
  const machine = buf.readUInt16LE(0);
  if (machine === 0x8664) {
    return parseCoff(buf);
  }

  return {
    format: 'Unknown',
    fileSize: buf.length,
    sections: [],
    symbols: [],
    relocations: [],
    hexDump: formatHexDump(buf),
  };
}

/**
 * Parses PE/COFF AMD64 (.obj) object file buffer.
 */
function parseCoff(buf: Buffer): ObjectAnalysis {
  const numberOfSections = buf.readUInt16LE(2);
  const ptrToSymbolTable = buf.readUInt32LE(8);
  const numberOfSymbols = buf.readUInt32LE(12);
  const sizeOfOptionalHeader = buf.readUInt16LE(16);

  const sections: ObjectSectionInfo[] = [];
  const sectionHeadersOffset = 20 + sizeOfOptionalHeader;

  // 1. Parse Section Headers
  for (let i = 0; i < numberOfSections; i++) {
    const sOffset = sectionHeadersOffset + i * 40;
    if (sOffset + 40 > buf.length) break;

    // Read 8-byte name
    let secName = '';
    const rawName = buf.subarray(sOffset, sOffset + 8);
    if (rawName[0] === 0x2f) { // '/' followed by decimal offset into string table
      const strOffset = parseInt(rawName.toString('ascii').slice(1).trim(), 10);
      const strTableStart = ptrToSymbolTable + numberOfSymbols * 18;
      secName = readNullTerminatedString(buf, strTableStart + strOffset);
    } else {
      let nullIdx = rawName.indexOf(0);
      if (nullIdx === -1) nullIdx = 8;
      secName = rawName.toString('utf8', 0, nullIdx).trim();
    }

    const virtualSize = buf.readUInt32LE(sOffset + 8);
    const virtualAddress = buf.readUInt32LE(sOffset + 12);
    const sizeOfRawData = buf.readUInt32LE(sOffset + 16);
    const pointerToRawData = buf.readUInt32LE(sOffset + 20);
    const characteristics = buf.readUInt32LE(sOffset + 36);

    let charDesc = [];
    if (characteristics & 0x00000020) charDesc.push('Code');
    if (characteristics & 0x00000040) charDesc.push('InitData');
    if (characteristics & 0x00000080) charDesc.push('UninitData');
    if (characteristics & 0x20000000) charDesc.push('Execute');
    if (characteristics & 0x40000000) charDesc.push('Read');
    if (characteristics & 0x80000000) charDesc.push('Write');

    let hexPreview = '';
    if (pointerToRawData > 0 && sizeOfRawData > 0 && pointerToRawData + sizeOfRawData <= buf.length) {
      const secBytes = buf.subarray(pointerToRawData, pointerToRawData + Math.min(sizeOfRawData, 64));
      hexPreview = Array.from(secBytes).map(b => b.toString(16).padStart(2, '0')).join(' ');
    }

    sections.push({
      name: secName,
      size: sizeOfRawData || virtualSize,
      offset: pointerToRawData,
      virtualAddress,
      characteristics: charDesc.join(' | ') || `0x${characteristics.toString(16)}`,
      hexPreview: hexPreview || undefined,
    });
  }

  // 2. Parse String Table & Symbols
  const symbols: ObjectSymbolInfo[] = [];
  const stringTableOffset = ptrToSymbolTable + numberOfSymbols * 18;

  let symIdx = 0;
  while (symIdx < numberOfSymbols) {
    const entryOffset = ptrToSymbolTable + symIdx * 18;
    if (entryOffset + 18 > buf.length) break;

    let symName = '';
    const zeroes = buf.readUInt32LE(entryOffset);
    if (zeroes === 0) {
      const strOffset = buf.readUInt32LE(entryOffset + 4);
      symName = readNullTerminatedString(buf, stringTableOffset + strOffset);
    } else {
      const rawName = buf.subarray(entryOffset, entryOffset + 8);
      let nullIdx = rawName.indexOf(0);
      if (nullIdx === -1) nullIdx = 8;
      symName = rawName.toString('utf8', 0, nullIdx).trim();
    }

    const value = buf.readUInt32LE(entryOffset + 8);
    const sectionNumber = buf.readInt16LE(entryOffset + 12);
    const type = buf.readUInt16LE(entryOffset + 14);
    const storageClass = buf.readUInt8(entryOffset + 16);
    const numAux = buf.readUInt8(entryOffset + 17);

    let secName = 'UNDEF';
    if (sectionNumber > 0 && sectionNumber <= sections.length) {
      secName = sections[sectionNumber - 1].name;
    } else if (sectionNumber === -1) {
      secName = 'ABS';
    } else if (sectionNumber === -2) {
      secName = 'DEBUG';
    }

    const isFunction = (type === 0x20) || (secName === '.text');
    const binding: 'global' | 'local' | 'weak' = storageClass === 2 ? 'global' : 'local';

    if (symName && !symName.startsWith('.')) {
      symbols.push({
        name: symName,
        section: secName,
        value,
        size: 0, // COFF symbols do not store size directly
        binding,
        type: isFunction ? 'function' : 'object',
      });
    }

    symIdx += 1 + numAux;
  }

  // 3. Parse Relocations
  const relocations: ObjectRelocInfo[] = [];
  for (let i = 0; i < numberOfSections; i++) {
    const sOffset = sectionHeadersOffset + i * 40;
    const pointerToRelocs = buf.readUInt32LE(sOffset + 24);
    const numberOfRelocs = buf.readUInt16LE(sOffset + 32);
    const secName = sections[i]?.name || `.sec${i + 1}`;

    if (pointerToRelocs > 0 && numberOfRelocs > 0) {
      for (let r = 0; r < numberOfRelocs; r++) {
        const rOffset = pointerToRelocs + r * 10;
        if (rOffset + 10 > buf.length) break;

        const virtualAddress = buf.readUInt32LE(rOffset);
        const symbolTableIndex = buf.readUInt32LE(rOffset + 4);
        const relocType = buf.readUInt16LE(rOffset + 8);

        // Find symbol name
        let targetSym = `sym_${symbolTableIndex}`;
        const targetOffset = ptrToSymbolTable + symbolTableIndex * 18;
        if (targetOffset + 18 <= buf.length) {
          const z = buf.readUInt32LE(targetOffset);
          if (z === 0) {
            const strOffset = buf.readUInt32LE(targetOffset + 4);
            targetSym = readNullTerminatedString(buf, stringTableOffset + strOffset);
          } else {
            const raw = buf.subarray(targetOffset, targetOffset + 8);
            let n = raw.indexOf(0);
            if (n === -1) n = 8;
            targetSym = raw.toString('utf8', 0, n).trim();
          }
        }

        let typeName = `0x${relocType.toString(16)}`;
        if (relocType === 1) typeName = 'IMAGE_REL_AMD64_ADDR64';
        else if (relocType === 2) typeName = 'IMAGE_REL_AMD64_ADDR32';
        else if (relocType === 4) typeName = 'IMAGE_REL_AMD64_REL32';

        relocations.push({
          section: secName,
          offset: virtualAddress,
          symbolName: targetSym,
          type: typeName,
        });
      }
    }
  }

  return {
    format: 'COFF',
    fileSize: buf.length,
    sections,
    symbols,
    relocations,
    hexDump: formatHexDump(buf),
  };
}

/**
 * Parses ELF64 (.o) object file buffer.
 */
function parseElf64(buf: Buffer): ObjectAnalysis {
  const shoff = Number(buf.readBigUInt64LE(40));
  const shentsize = buf.readUInt16LE(58);
  const shnum = buf.readUInt16LE(60);
  const shstrndx = buf.readUInt16LE(62);

  // Read section header string table (.shstrtab)
  let shstrtabBuf: Buffer = Buffer.alloc(0);
  if (shstrndx < shnum) {
    const strShdrOffset = shoff + shstrndx * shentsize;
    const strOffset = Number(buf.readBigUInt64LE(strShdrOffset + 24));
    const strSize = Number(buf.readBigUInt64LE(strShdrOffset + 32));
    if (strOffset + strSize <= buf.length) {
      shstrtabBuf = Buffer.from(buf.subarray(strOffset, strOffset + strSize));
    }
  }

  const sections: ObjectSectionInfo[] = [];
  let symtabOffset = 0;
  let symtabSize = 0;
  let symtabEntSize = 24;
  let symtabLink = 0;

  const relaSections: Array<{ name: string; offset: number; size: number; link: number; info: number }> = [];

  for (let i = 0; i < shnum; i++) {
    const sOffset = shoff + i * shentsize;
    if (sOffset + shentsize > buf.length) break;

    const sh_name = buf.readUInt32LE(sOffset);
    const sh_type = buf.readUInt32LE(sOffset + 4);
    const sh_flags = Number(buf.readBigUInt64LE(sOffset + 8));
    const sh_addr = Number(buf.readBigUInt64LE(sOffset + 16));
    const sh_offset = Number(buf.readBigUInt64LE(sOffset + 24));
    const sh_size = Number(buf.readBigUInt64LE(sOffset + 32));
    const sh_link = buf.readUInt32LE(sOffset + 40);
    const sh_info = buf.readUInt32LE(sOffset + 44);
    const sh_entsize = Number(buf.readBigUInt64LE(sOffset + 56));

    const name = readNullTerminatedString(shstrtabBuf, sh_name);

    if (sh_type === 2) { // SHT_SYMTAB
      symtabOffset = sh_offset;
      symtabSize = sh_size;
      symtabEntSize = sh_entsize || 24;
      symtabLink = sh_link;
    } else if (sh_type === 4) { // SHT_RELA
      relaSections.push({ name, offset: sh_offset, size: sh_size, link: sh_link, info: sh_info });
    }

    if (name) {
      let flagDesc = [];
      if (sh_flags & 0x1) flagDesc.push('Write');
      if (sh_flags & 0x2) flagDesc.push('Alloc');
      if (sh_flags & 0x4) flagDesc.push('Exec');

      let hexPreview = '';
      if (sh_type !== 8 && sh_offset > 0 && sh_size > 0 && sh_offset + sh_size <= buf.length) {
        const secBytes = buf.subarray(sh_offset, sh_offset + Math.min(sh_size, 64));
        hexPreview = Array.from(secBytes).map(b => b.toString(16).padStart(2, '0')).join(' ');
      }

      sections.push({
        name,
        size: sh_size,
        offset: sh_offset,
        virtualAddress: sh_addr,
        characteristics: flagDesc.join(' | ') || `0x${sh_flags.toString(16)}`,
        hexPreview: hexPreview || undefined,
      });
    }
  }

  // Parse ELF symbol table
  const symbols: ObjectSymbolInfo[] = [];
  const symbolNames: string[] = [];

  if (symtabOffset > 0 && symtabSize > 0 && symtabLink < shnum) {
    // Read string table for symbols
    const strShdrOffset = shoff + symtabLink * shentsize;
    const strOffset = Number(buf.readBigUInt64LE(strShdrOffset + 24));
    const strSize = Number(buf.readBigUInt64LE(strShdrOffset + 32));
    const strBuf = Buffer.from(buf.subarray(strOffset, strOffset + strSize));

    const count = Math.floor(symtabSize / symtabEntSize);
    for (let i = 0; i < count; i++) {
      const entry = symtabOffset + i * symtabEntSize;
      const st_name = buf.readUInt32LE(entry);
      const st_info = buf.readUInt8(entry + 4);
      const st_shndx = buf.readUInt16LE(entry + 6);
      const st_value = Number(buf.readBigUInt64LE(entry + 8));
      const st_size = Number(buf.readBigUInt64LE(entry + 16));

      const symName = readNullTerminatedString(strBuf, st_name);
      symbolNames.push(symName);

      const bindVal = st_info >> 4;
      const typeVal = st_info & 0x0f;

      const binding: 'global' | 'local' | 'weak' =
        bindVal === 1 ? 'global' : (bindVal === 2 ? 'weak' : 'local');
      const type: 'function' | 'object' | 'notype' =
        typeVal === 2 ? 'function' : (typeVal === 1 ? 'object' : 'notype');

      let secName = 'UNDEF';
      if (st_shndx > 0 && st_shndx < sections.length) {
        secName = sections[st_shndx - 1]?.name || `.sec${st_shndx}`;
      } else if (st_shndx === 0xfff1) {
        secName = 'ABS';
      }

      if (symName && !symName.startsWith('$') && !symName.startsWith('.')) {
        symbols.push({
          name: symName,
          section: secName,
          value: st_value,
          size: st_size,
          binding,
          type,
        });
      }
    }
  }

  // Parse Relocations
  const relocations: ObjectRelocInfo[] = [];
  for (const rSec of relaSections) {
    const targetSecName = rSec.name.replace(/^\.rela\./, '.');
    const count = Math.floor(rSec.size / 24);

    for (let i = 0; i < count; i++) {
      const rOffset = rSec.offset + i * 24;
      if (rOffset + 24 > buf.length) break;

      const offset = Number(buf.readBigUInt64LE(rOffset));
      const info = buf.readBigUInt64LE(rOffset + 8);
      const addend = Number(buf.readBigInt64LE(rOffset + 16));

      const symIdx = Number(info >> 32n);
      const typeVal = Number(info & 0xffffffffn);
      const symName = symbolNames[symIdx] || `sym_${symIdx}`;

      let typeName = `R_X86_64_${typeVal}`;
      if (typeVal === 1) typeName = 'R_X86_64_64';
      else if (typeVal === 2) typeName = 'R_X86_64_PC32';
      else if (typeVal === 4) typeName = 'R_X86_64_PLT32';

      relocations.push({
        section: targetSecName,
        offset,
        symbolName: symName,
        type: typeName,
        addend: addend !== 0 ? addend : undefined,
      });
    }
  }

  return {
    format: 'ELF64',
    fileSize: buf.length,
    sections,
    symbols,
    relocations,
    hexDump: formatHexDump(buf),
  };
}
