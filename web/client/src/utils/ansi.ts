/**
 * Strips all ANSI terminal escape sequences (colors, bold, resets) from text.
 */
export function stripAnsi(str: string | undefined | null): string {
  if (!str) return '';
  return str.replace(
    /[\u001b\x1b]\[[0-9;]*[a-zA-Z]|[\u001b\x1b]\([a-zA-Z]|[\u001b\x1b][=>]/g,
    ''
  );
}
