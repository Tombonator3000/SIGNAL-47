// The lines that explain the controls (the first drive, the flashlight, the field camera) are
// said once on a device (todo.md: "Hint bare første gang"), not every night and not after every
// reload; the notebook's Controls tab has them all (ui/UI.ts). Which have been said is kept in
// localStorage under s47.hints (every key of ours has the s47. prefix: the Pages sites share one
// origin). Storage can be blocked (private mode); then each is said once per page.

const KEY = 's47.hints';
let said: Set<string> | null = null;

function load() {
  if (said) return said;
  said = new Set();
  try {
    const v = JSON.parse(localStorage.getItem(KEY) ?? '[]');
    if (Array.isArray(v)) for (const id of v) if (typeof id === 'string') said.add(id);
  } catch { /* none kept */ }
  return said;
}

/** True the first time on this device a hint is asked for (and it is marked said), false after. */
export function firstTime(id: string) {
  const s = load();
  if (s.has(id)) return false;
  s.add(id);
  try { localStorage.setItem(KEY, JSON.stringify([...s])); } catch { /* not kept */ }
  return true;
}

/** Whether a hint has been said on this device (the tests). */
export function hintSaid(id: string) { return load().has(id); }
